#!/usr/bin/env python3
import torch
import torchvision

def export_resnet18():
    print("Loading pretrained ResNet18...")
    model = torchvision.models.resnet18(pretrained=True)
    model.eval() 
    
    dummy_input = torch.randn(1, 3, 224, 224)
    
    print("Exporting to ONNX...")
    torch.onnx.export(
        model,
        dummy_input,
        "../models/resnet18.onnx",
        opset_version=11,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes=None 
    )
    print("Model exported successfully to models/resnet18.onnx")

if __name__ == "__main__":
    export_resnet18()