根据你的实测数据，以下是一份专业、不夸大的 README 文档，直接复制使用即可。

---

# 🚀 C++ 高性能推理服务：ONNX Runtime vs PyTorch 性能对比

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/)
[![ONNX Runtime](https://img.shields.io/badge/ONNX_Runtime-1.15.1-orange)](https://onnxruntime.ai/)
[![PyTorch](https://img.shields.io/badge/PyTorch-2.0-red)](https://pytorch.org/)
[![Vue](https://img.shields.io/badge/Vue-3.3-green)](https://vuejs.org/)
[![Docker](https://img.shields.io/badge/Docker-29.4.0-blue)](https://www.docker.com/)

一个用于量化对比 C++ 与 Python 在 AI 推理场景下性能差异的生产级项目。通过相同的 ResNet18 模型、相同的预处理流程，在同一台服务器上分别运行 C++ + ONNX Runtime 和 Python + PyTorch 推理服务，并由 Vue.js 前端直观展示延迟、吞吐、内存占用等关键指标。

## 📊 实测性能数据

| 指标 | C++ (ONNX Runtime) | Python (PyTorch) | 加速比 / 优化幅度 |
| :--- | :--- | :--- | :--- |
| 平均推理延迟 | **62.5 ms** | **179.4 ms** | **2.87x** |
| P50 延迟 | 62.3 ms | 180.9 ms | 2.90x |
| P99 延迟 | 70.1 ms | 189.3 ms | 2.70x |
| 内存占用 (RSS) | **249.5 MiB** | **441.3 MiB** | **43% ↓** |
| 吞吐量 (QPS) | 11.8 | 4.1 | 2.87x |
| 镜像体积¹ | 436 MB | 11.7 GB | 96% ↓ |

> **测试环境**：Intel Xeon Platinum 8269CY @ 2.50GHz（2 核），3.4GB RAM，Ubuntu 22.04.5 LTS，Docker 29.4.0，ResNet18 模型，输入尺寸 224×224，测试图片为 PyTorch Hub 示例 dog.jpg。  
> **¹ 镜像体积**：`docker images` 显示的虚拟大小。Python 镜像因包含完整 PyTorch 及构建缓存而较大；C++ 镜像通过多阶段构建仅保留运行时必需库。

## 🏛️ 系统架构

```
用户浏览器 (http://47.95.252.95)
                │
                ▼
        ┌───────────────┐
        │  Nginx :80    │  反向代理 + 静态资源缓存
        └───────┬───────┘
                │
   ┌────────────┼────────────┐
   │ /api/cpp   │ /api/python│
   ▼            ▼            │
┌──────────┐ ┌──────────────┐│
│ C++ 容器 │ │ Python 容器  ││   静态资源
│  :8080   │ │   :8081      ││   / → 前端容器
└──────────┘ └──────────────┘│
   │            │             │
   └────────────┼─────────────┘
                ▼
        ┌───────────────┐
        │ Vue 前端 :8082│  Chart.js 可视化
        └───────────────┘
```

## 🚀 快速开始

### 环境要求

- Docker 24.0+
- Docker Compose 2.0+
- 约 5GB 可用磁盘空间（用于构建镜像）

### 一键部署

```bash
git clone https://github.com/yourusername/inference-benchmark.git
cd inference-benchmark
docker-compose up -d --build
```

首次构建约需 10~15 分钟。构建完成后访问：

- **主界面**：`http://<服务器IP>`（默认端口 80）
- **直连前端**：`http://<服务器IP>:8082`

### 手动测试推理接口

```bash
# C++ 服务
curl -X POST -H "Content-Type: application/octet-stream" \
     --data-binary @test_dog.jpg \
     http://localhost:8080/api/inference

# Python 服务
curl -X POST -H "Content-Type: application/octet-stream" \
     --data-binary @test_dog.jpg \
     http://localhost:8081/api/inference
```

## 📁 项目结构

```
inference-benchmark/
├── models/                         # ONNX 模型文件
├── cpp-backend/                    # C++ 推理服务
│   ├── include/                    # 头文件
│   ├── src/                        # 源代码
│   ├── third_party/cpp-httplib/    # HTTP 库
│   ├── CMakeLists.txt
│   └── Dockerfile
├── python-backend/                 # Python 对比服务
│   ├── app.py
│   ├── requirements.txt
│   └── Dockerfile
├── frontend/                       # Vue.js 前端
│   ├── public/                     # 静态资源（含 ImageNet 标签）
│   ├── src/
│   └── Dockerfile
├── nginx/                          # Nginx 配置
│   └── nginx.conf
├── scripts/                        # 辅助脚本
│   ├── export_model.py             # 导出 ONNX 模型
│   └── benchmark_full.sh           # 自动化性能测试
├── docker-compose.yml
└── README.md
```

## 🔧 关键技术实现

### 1. C++ 推理引擎

- **零拷贝**：通过 `Ort::Value::CreateTensor` 直接复用 OpenCV Mat 内存，避免数据复制。
- **预处理对齐**：BGR→RGB、缩放到 [0,1]、ImageNet 标准化（均值 [0.485,0.456,0.406]，标准差 [0.229,0.224,0.225]）。
- **图优化**：启用 ONNX Runtime 全部图优化（算子融合、常量折叠）。
- **线程调优**：`SetIntraOpNumThreads` 设置为物理核心数。

### 2. Python 基准服务

- Flask 提供 HTTP 接口，PyTorch 加载预训练 ResNet18。
- 预热 10 次消除冷启动影响。
- 预处理逻辑与 C++ 完全一致，确保公平对比。

### 3. 容器化与部署

- **C++ 镜像**：多阶段构建，编译阶段包含完整工具链，运行阶段仅保留 `.so` 运行时库，最终体积 436 MB。
- **Python 镜像**：基于 `python:3.9-slim`，安装 PyTorch CPU 版及 OpenCV 等依赖。
- **Nginx**：反向代理 API 请求，静态资源缓存 1 年，统一入口。

### 4. 前端可视化

- Vue 3 + Chart.js 绘制性能对比柱状图。
- 加载标准 ImageNet 中文标签，将模型输出的 Top5 索引转换为可读类别名称。
- 通过 Softmax 计算置信度百分比。

## 📈 性能数据复现

项目提供了自动化测试脚本 `scripts/benchmark_full.sh`，可一键采集所有性能指标：

```bash
cd scripts
chmod +x benchmark_full.sh
./benchmark_full.sh
```

脚本自动完成：
- 服务健康检查
- 预热（5 次）
- 100 次请求统计 P50/P99/平均延迟
- ApacheBench 压测 1000 次计算 QPS
- 采集容器内存占用和镜像体积
- 生成带时间戳的测试报告

## 🛠️ 遇到的工程挑战

| 挑战 | 解决方案 |
| :--- | :--- |
| ONNX IR 版本不兼容 | 导出时显式指定 `ir_version=9`，匹配 ONNX Runtime 1.15.1 |
| Docker 构建时 PyTorch 下载超时 | 配置国内 PyPI 镜像（阿里云/清华源），合并 RUN 层减少磁盘占用 |
| 磁盘空间不足导致镜像导出失败 | 扩容系统盘至 50GB，优化 Dockerfile 层合并 |
| 容器内 OpenCV 动态库缺失 | 在运行阶段补全 `libopencv-dnn4.2` 等依赖 |
| 预测类别错误（白鲸 vs 金毛犬） | 替换为标准顺序的 ImageNet 中文标签文件 |
| 前端图表偶发消失 | 使用 `this.$nextTick` 确保 Canvas 销毁与重建时序 |

## 📝 后续优化方向

- [ ] 支持 GPU 推理（CUDAExecutionProvider）
- [ ] 实现动态批处理提升吞吐
- [ ] 集成 Prometheus + Grafana 监控
- [ ] 评估 INT8 量化对精度和速度的影响

## 📄 开源协议

MIT License

---

**如果这个项目对你有帮助，请给一个 ⭐ Star 鼓励一下！**