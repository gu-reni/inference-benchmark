import time
import numpy as np
import cv2
import torch
import torchvision
from flask import Flask, request, jsonify

app = Flask(__name__)

# 加载模型并预热
print("Loading ResNet18 model...")
model = torchvision.models.resnet18(pretrained=True)
model.eval()

# 预热：执行 10 次空推理
print("Warming up...")
dummy = torch.randn(1, 3, 224, 224)
with torch.no_grad():
    for _ in range(10):
        _ = model(dummy)
print("Model ready.")

@app.route('/api/inference', methods=['POST'])
def inference():
    # 读取图片二进制数据
    img_data = request.get_data()
    np_arr = np.frombuffer(img_data, np.uint8)
    img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
    
    if img is None:
        return jsonify({"error": "Invalid image"}), 400
    
    # 预处理：与 C++ 版本保持一致
    resized = cv2.resize(img, (224, 224))
    resized = resized.astype(np.float32) / 255.0
    resized = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    
    # HWC -> CHW
    blob = np.transpose(resized, (2, 0, 1))
    blob = np.expand_dims(blob, axis=0)  # 添加 batch 维度
    
    # 推理计时
    input_tensor = torch.from_numpy(blob)
    start = time.perf_counter()
    with torch.no_grad():
        output = model(input_tensor)
    end = time.perf_counter()
    time_us = int((end - start) * 1_000_000)
    
    # 提取 Top5 结果（logits）
    output_np = output.numpy().flatten()
    top5_indices = np.argsort(output_np)[-5:][::-1]
    top5_values = output_np[top5_indices]
    
    return jsonify({
        "top5": top5_values.tolist(),
        "inference_time_us": time_us
    })

@app.route('/health', methods=['GET'])
def health():
    return jsonify({"status": "ok"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8081, threaded=True)