FROM docker.m.daocloud.io/library/ubuntu:20.04 AS builder

RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
COPY onnxruntime-linux-x64-1.15.1.tgz .
RUN tar -xzf onnxruntime-linux-x64-1.15.1.tgz \
    && cp -r onnxruntime-linux-x64-1.15.1/include/* /usr/local/include/ \
    && cp -r onnxruntime-linux-x64-1.15.1/lib/* /usr/local/lib/

WORKDIR /app
COPY include/ ./include/
COPY src/ ./src/
COPY third_party/ ./third_party/
COPY CMakeLists.txt .

RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j$(nproc)

FROM docker.m.daocloud.io/library/ubuntu:20.04 AS runtime

RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    libopencv-core4.2 \
    libopencv-imgcodecs4.2 \
    libopencv-imgproc4.2 \
    libopencv-dnn4.2 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /usr/local/lib/libonnxruntime.so /usr/local/lib/
COPY --from=builder /app/build/bin/inference_server /app/

RUN ldconfig

EXPOSE 8080

CMD ["/app/inference_server", "/app/models/resnet18_ir9.onnx"]