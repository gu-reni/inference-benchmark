#!/bin/bash
# 完整性能数据采集脚本 - 用于 README 数据更新
# 使用方法: chmod +x benchmark_full.sh && ./benchmark_full.sh

set -e

# -------------------- 配置 --------------------
CPP_URL="http://localhost:8080/api/inference"
PYTHON_URL="http://localhost:8081/api/inference"
CPP_HEALTH="http://localhost:8080/health"
PYTHON_HEALTH="http://localhost:8081/health"
TEST_IMAGE="${1:-test_dog.jpg}"          # 可传入测试图片路径
REQUESTS=100                             # 延迟统计请求次数
WARMUP=5                                 # 预热请求次数
AB_REQUESTS=1000                         # ab 压测总请求数
AB_CONCURRENCY=4                         # ab 并发数
TIMEOUT=10                               # curl 超时时间

# -------------------- 颜色输出 --------------------
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# -------------------- 依赖检查 --------------------
check_command() {
    command -v "$1" >/dev/null 2>&1 || error "$1 未安装，请先安装 (apt install $2)"
}

check_command curl curl
check_command awk gawk
check_command bc bc
check_command ab apache2-utils

# JSON 解析器选择
if command -v jq >/dev/null 2>&1; then
    PARSE_CMD="jq -r '.inference_time_us'"
else
    info "jq 未安装，将使用 python3 解析 JSON"
    PARSE_CMD="python3 -c \"import sys,json; print(json.load(sys.stdin)['inference_time_us'])\""
fi

# -------------------- 服务健康检查 --------------------
info "检查服务健康状态..."
for url in "$CPP_HEALTH" "$PYTHON_HEALTH"; do
    if ! curl -s -f -o /dev/null --max-time "$TIMEOUT" "$url"; then
        error "服务 $url 不可用，请先启动服务 (docker-compose up -d)"
    fi
done
info "所有服务健康状态正常"

# -------------------- 检查测试图片 --------------------
if [ ! -f "$TEST_IMAGE" ]; then
    warn "测试图片 $TEST_IMAGE 不存在，尝试下载示例图片..."
    wget -q -O test_dog.jpg https://raw.githubusercontent.com/pytorch/hub/master/images/dog.jpg || error "下载失败，请手动准备测试图片"
    TEST_IMAGE="test_dog.jpg"
fi
info "测试图片: $TEST_IMAGE (大小: $(du -h "$TEST_IMAGE" | cut -f1))"

# -------------------- 预热 --------------------
info "预热中 (每个服务 $WARMUP 次)..."
for i in $(seq 1 $WARMUP); do
    curl -s -X POST -H "Content-Type: application/octet-stream" --data-binary "@$TEST_IMAGE" "$CPP_URL" > /dev/null &
    curl -s -X POST -H "Content-Type: application/octet-stream" --data-binary "@$TEST_IMAGE" "$PYTHON_URL" > /dev/null &
    wait
done
info "预热完成"

# -------------------- 延迟数据采集 --------------------
collect_latency() {
    local url=$1
    local output_file=$2
    > "$output_file"
    for i in $(seq 1 $REQUESTS); do
        curl -s -X POST -H "Content-Type: application/octet-stream" --data-binary "@$TEST_IMAGE" "$url" \
            --max-time "$TIMEOUT" | eval "$PARSE_CMD" >> "$output_file"
        printf "\r  采集进度: %3d / %d" $i $REQUESTS
    done
    echo ""
}

info "采集 C++ 延迟数据 ($REQUESTS 次)..."
collect_latency "$CPP_URL" cpp_latency_us.txt

info "采集 Python 延迟数据 ($REQUESTS 次)..."
collect_latency "$PYTHON_URL" python_latency_us.txt

# 转换并计算统计数据（返回 p50 p99 avg 三个值）
calc_stats() {
    local file=$1
    awk '{print $1/1000}' "$file" | sort -n | awk '
    BEGIN { count=0; sum=0 }
    {
        a[++count]=$1;
        sum+=$1;
    }
    END {
        if (count==0) exit;
        p50=a[int(count*0.50)];
        p99=a[int(count*0.99)];
        avg=sum/count;
        printf "%.2f %.2f %.2f", p50, p99, avg;
    }'
}

cpp_stats_raw=$(calc_stats cpp_latency_us.txt)
python_stats_raw=$(calc_stats python_latency_us.txt)

cpp_p50=$(echo "$cpp_stats_raw" | awk '{print $1}')
cpp_p99=$(echo "$cpp_stats_raw" | awk '{print $2}')
cpp_avg=$(echo "$cpp_stats_raw" | awk '{print $3}')
python_p50=$(echo "$python_stats_raw" | awk '{print $1}')
python_p99=$(echo "$python_stats_raw" | awk '{print $2}')
python_avg=$(echo "$python_stats_raw" | awk '{print $3}')

info "C++  延迟统计: P50: ${cpp_p50} ms, P99: ${cpp_p99} ms, Avg: ${cpp_avg} ms"
info "Python延迟统计: P50: ${python_p50} ms, P99: ${python_p99} ms, Avg: ${python_avg} ms"

# -------------------- 内存占用采集 --------------------
info "采集容器内存占用..."
cpp_mem=$(docker stats --no-stream --format "{{.MemUsage}}" inference-benchmark-cpp-backend-1 2>/dev/null | awk '{print $1}')
python_mem=$(docker stats --no-stream --format "{{.MemUsage}}" inference-benchmark-python-backend-1 2>/dev/null | awk '{print $1}')
[ -z "$cpp_mem" ] && cpp_mem="N/A"
[ -z "$python_mem" ] && python_mem="N/A"
info "C++  内存: $cpp_mem"
info "Python内存: $python_mem"

# -------------------- QPS 吞吐量压测 --------------------
info "使用 ab 压测 C++ 服务 (${AB_REQUESTS}请求, ${AB_CONCURRENCY}并发)..."
cpp_ab_out=$(ab -n $AB_REQUESTS -c $AB_CONCURRENCY -p "$TEST_IMAGE" -T 'application/octet-stream' "$CPP_URL" 2>&1)
cpp_qps=$(echo "$cpp_ab_out" | grep "Requests per second" | awk '{print $4}')
[ -z "$cpp_qps" ] && cpp_qps="N/A"

info "使用 ab 压测 Python 服务 (${AB_REQUESTS}请求, ${AB_CONCURRENCY}并发)..."
python_ab_out=$(ab -n $AB_REQUESTS -c $AB_CONCURRENCY -p "$TEST_IMAGE" -T 'application/octet-stream' "$PYTHON_URL" 2>&1)
python_qps=$(echo "$python_ab_out" | grep "Requests per second" | awk '{print $4}')
[ -z "$python_qps" ] && python_qps="N/A"

info "C++  QPS: $cpp_qps"
info "Python QPS: $python_qps"

# -------------------- 镜像体积 --------------------
info "采集 Docker 镜像体积..."
cpp_size=$(docker images --format "{{.Size}}" inference-benchmark-cpp-backend:latest 2>/dev/null)
python_size=$(docker images --format "{{.Size}}" inference-benchmark-python-backend:latest 2>/dev/null)
[ -z "$cpp_size" ] && cpp_size="N/A"
[ -z "$python_size" ] && python_size="N/A"
info "C++  镜像大小: $cpp_size"
info "Python镜像大小: $python_size"

# -------------------- 环境信息 --------------------
info "采集测试环境信息..."
cpu_model=$(lscpu | grep "Model name" | head -1 | awk -F: '{print $2}' | xargs)
mem_total=$(free -h | grep Mem | awk '{print $2}')
os_version=$(lsb_release -d 2>/dev/null | awk -F: '{print $2}' | xargs || cat /etc/os-release | grep PRETTY_NAME | cut -d '"' -f2)
docker_ver=$(docker --version | awk '{print $3}' | sed 's/,//')

# -------------------- 计算加速比 --------------------
if [ -n "$python_avg" ] && [ -n "$cpp_avg" ] && [ "$(echo "$cpp_avg > 0" | bc)" -eq 1 ]; then
    latency_speedup=$(echo "scale=2; $python_avg / $cpp_avg" | bc)
else
    latency_speedup="N/A"
fi

if [ -n "$cpp_qps" ] && [ -n "$python_qps" ] && [ "$cpp_qps" != "N/A" ] && [ "$python_qps" != "N/A" ] && [ "$(echo "$python_qps > 0" | bc)" -eq 1 ]; then
    qps_speedup=$(echo "scale=2; $cpp_qps / $python_qps" | bc)
else
    qps_speedup="N/A"
fi

# -------------------- 生成报告 --------------------
REPORT_FILE="benchmark_report_$(date +%Y%m%d_%H%M%S).txt"
{
    echo "=============================================="
    echo "        推理服务性能测试报告"
    echo "=============================================="
    echo "测试时间: $(date)"
    echo "测试图片: $TEST_IMAGE"
    echo "请求次数: $REQUESTS (延迟统计), $AB_REQUESTS (压测)"
    echo ""
    echo "----------------- 环境信息 -----------------"
    echo "CPU:      $cpu_model"
    echo "内存:     $mem_total"
    echo "操作系统: $os_version"
    echo "Docker:   $docker_ver"
    echo ""
    echo "----------------- 延迟 (毫秒) -----------------"
    echo "C++  : P50: ${cpp_p50} ms, P99: ${cpp_p99} ms, Avg: ${cpp_avg} ms"
    echo "Python: P50: ${python_p50} ms, P99: ${python_p99} ms, Avg: ${python_avg} ms"
    echo ""
    echo "----------------- 内存占用 -----------------"
    echo "C++  : $cpp_mem"
    echo "Python: $python_mem"
    echo ""
    echo "----------------- 吞吐量 (QPS) -----------------"
    echo "C++  : $cpp_qps"
    echo "Python: $python_qps"
    echo ""
    echo "----------------- 镜像体积 -----------------"
    echo "C++  : $cpp_size"
    echo "Python: $python_size"
    echo ""
    echo "加速比 (延迟): ${latency_speedup} x"
    echo "加速比 (QPS) : ${qps_speedup} x"
    echo "=============================================="
} | tee "$REPORT_FILE"

info "报告已保存至: $REPORT_FILE"

# -------------------- 清理临时文件 --------------------
rm -f cpp_latency_us.txt python_latency_us.txt
