# 多阶段构建：编译阶段
FROM ubuntu:20.04 as builder

# 替换为阿里云镜像源（解决国内 apt 超时问题）
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# 下载并安装 ONNX Runtime
WORKDIR /tmp
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v1.15.1/onnxruntime-linux-x64-1.15.1.tgz \
    && tar -xzf onnxruntime-linux-x64-1.15.1.tgz \
    && cp -r onnxruntime-linux-x64-1.15.1/include/* /usr/local/include/ \
    && cp -r onnxruntime-linux-x64-1.15.1/lib/* /usr/local/lib/

# 复制源代码
WORKDIR /app
COPY include/ ./include/
COPY src/ ./src/
COPY third_party/ ./third_party/
COPY CMakeLists.txt .

# 编译
RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j$(nproc)

# 运行阶段
FROM ubuntu:20.04 as runtime

# 同样替换源
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    libopencv-core4.2 \
    libopencv-imgcodecs4.2 \
    libopencv-imgproc4.2 \
    && rm -rf /var/lib/apt/lists/*

    RUN apt-get update && apt-get install -y \
    libopencv-core4.2 \
    libopencv-imgcodecs4.2 \
    libopencv-imgproc4.2 \
    libopencv-dnn4.2 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# 从编译阶段复制必要文件
COPY --from=builder /usr/local/lib/libonnxruntime.so /usr/local/lib/
COPY --from=builder /app/build/bin/inference_server /app/

# 更新动态库缓存
RUN ldconfig

EXPOSE 8080

# 模型文件通过 docker-compose 卷挂载，路径为 /app/models/resnet18_ir9.onnx
CMD ["/app/inference_server", "/app/models/resnet18_ir9.onnx"]