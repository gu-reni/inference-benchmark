FROM python:3.9-slim

WORKDIR /app

# 兼容新旧 Debian 源配置：替换 apt 源为阿里云镜像
RUN if [ -f /etc/apt/sources.list ]; then \
        sed -i 's/deb.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list \
        && sed -i 's/security.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list; \
    elif [ -f /etc/apt/sources.list.d/debian.sources ]; then \
        sed -i 's/deb.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list.d/debian.sources \
        && sed -i 's/security.debian.org/mirrors.aliyun.com/g' /etc/apt/sources.list.d/debian.sources; \
    fi \
    && apt-get update \
    && apt-get install -y \
        libgl1 \
        libglx-mesa0 \
        libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

ENV PIP_DEFAULT_TIMEOUT=300 \
    PIP_RETRIES=10

# 安装 Python 依赖
RUN pip install --no-cache-dir \
        -i https://mirrors.aliyun.com/pypi/simple/ \
        flask==2.3.0 \
        opencv-python-headless==4.8.0.74 \
        numpy==1.24.0 \
    && pip install --no-cache-dir \
        -i https://pypi.tuna.tsinghua.edu.cn/simple \
        torch>=2.0.0 \
        torchvision>=0.15.0

COPY app.py .

EXPOSE 8081

CMD ["python", "app.py"]