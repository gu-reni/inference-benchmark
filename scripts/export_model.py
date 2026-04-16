#!/usr/bin/env python3
import torch
import torchvision

def export_resnet18():
    print("Loading pretrained ResNet18...")
    from torchvision.models import ResNet18_Weights
    model = torchvision.models.resnet18(weights=ResNet18_Weights.IMAGENET1K_V1)
    model.eval()
    
    dummy_input = torch.randn(1, 3, 224, 224)
    
    print("Exporting to ONNX with IR version 9...")
    torch.onnx.export(
        model,
        dummy_input,
        "../models/resnet18.onnx",
        opset_version=11,           # 算子集版本
        ir_version=9,               # 降低 IR 版本以兼容 ONNX Runtime 1.15
        input_names=['input'],
        output_names=['output'],
        dynamic_axes=None
    )
    print("Model exported successfully to models/resnet18.onnx")

if __name__ == "__main__":
    export_resnet18()