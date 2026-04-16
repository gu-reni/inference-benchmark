#!/usr/bin/env python3
import torch
import torchvision

def export_resnet18():
    print("Loading pretrained ResNet18...")
    from torchvision.models import ResNet18_Weights
    # 加载预训练模型
    model = torchvision.models.resnet18(weights=ResNet18_Weights.IMAGENET1K_V1)
    model.eval()
    
    dummy_input = torch.randn(1, 3, 224, 224)
    
    print("Exporting to ONNX...")
    torch.onnx.export(
        model,
        dummy_input,
        "../models/resnet18.onnx",
        opset_version=11,           # 使用兼容性好的算子集版本
        input_names=['input'],
        output_names=['output'],
        dynamic_axes=None
    )
    print("Model exported successfully to models/resnet18.onnx")

if __name__ == "__main__":
    export_resnet18()