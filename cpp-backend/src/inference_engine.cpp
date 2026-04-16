#include "inference_engine.h"
#include <iostream>
#include <cassert>
#include <thread>   // 必须包含以使用 std::thread

InferenceEngine::InferenceEngine(const std::string& model_path)
    : env(ORT_LOGGING_LEVEL_WARNING, "InferenceEngine"),
      session(nullptr),
      memory_info(nullptr) {
    
    // 1. 创建 SessionOptions 对象 (注意不是 Session)
    Ort::SessionOptions session_options;
    
    // 2. 设置线程数
    int num_cores = std::thread::hardware_concurrency();
    if (num_cores > 4) num_cores = num_cores / 2;  // 简单处理超线程
    session_options.SetIntraOpNumThreads(num_cores);
    std::cout << "Set IntraOp threads: " << num_cores << std::endl;
    
    // 3. 开启图优化
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    
    // 4. 创建 CPU 内存信息对象
    memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    // 5. 加载模型
    try {
        session = Ort::Session(env, model_path.c_str(), session_options);
        std::cout << "Model loaded successfully: " << model_path << std::endl;
    } catch (const Ort::Exception& e) {  // 注意 const 拼写
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        throw;
    }
}

std::pair<std::vector<float>, long long> InferenceEngine::infer(const cv::Mat& img) {
    // 1. 预处理
    cv::Mat resized, float_img, rgb_img, blob;
    cv::resize(img, resized, cv::Size(input_width, input_height));
    resized.convertTo(float_img, CV_32FC3, 1.0 / 255.0);
    cv::cvtColor(float_img, rgb_img, cv::COLOR_BGR2RGB);
    
    if (!rgb_img.isContinuous()) {
        rgb_img = rgb_img.clone();
    }
    
    blob = cv::dnn::blobFromImage(rgb_img);
    
    // 2. 创建输入 Tensor (零拷贝)
    std::vector<int64_t> input_shape = {batch_size, channels, input_height, input_width};
    size_t input_tensor_size = batch_size * channels * input_height * input_width;
    
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        blob.ptr<float>(),
        input_tensor_size,
        input_shape.data(),
        input_shape.size()
    );
    
    // 3. 推理计时
    std::vector<Ort::Value> output_tensors;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            &input_name, &input_tensor, 1,
            &output_name, 1
        );
    } catch (const Ort::Exception& e) {
        std::cerr << "Inference failed: " << e.what() << std::endl;
        throw;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // 4. 提取结果
    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    size_t output_count = output_shape[1];
    
    std::vector<float> results(output_data, output_data + output_count);
    
    return {results, duration};
}