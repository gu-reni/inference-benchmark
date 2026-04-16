# syntax=docker/dockerfile:1

FROM python:3.9-slim

WORKDIR /app

# 升级 pip
RUN pip install --upgrade pip

# 安装通用依赖
RUN --mount=type=cache,target=/root/.cache/pip \
    pip install --no-cache-dir \
    -i https://mirrors.aliyun.com/pypi/simple/ \
    flask==2.3.0 \
    opencv-python==4.8.0.74 \
    numpy==1.24.0

# 安装 PyTorch CPU 版本
RUN --mount=type=cache,target=/root/.cache/pip \
    pip install --no-cache-dir \
    -i https://mirrors.aliyun.com/pypi/simple/ \
    torch==2.0.0 \
    torchvision==0.15.0

COPY app.py .

EXPOSE 8081

CMD ["python", "app.py"]