#ifndef INFERENCE_ENGINE_H
#define INFERENCE_ENGINE_H

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <chrono>

class InferenceEngine {
public:
    explicit InferenceEngine(const std::string& model_path);
    ~InferenceEngine() = default;
    std::tuple<std::vector<float>, std::vector<int>, long long> infer(const cv::Mat& img);

private:
    Ort::Env env{nullptr};
    Ort::Session session{nullptr};
    Ort::MemoryInfo memory_info{nullptr};
    
    static constexpr const char* input_name = "input";
    static constexpr const char* output_name = "output";
    
    static constexpr int64_t batch_size = 1;
    static constexpr int64_t channels = 3;
    static constexpr int64_t input_height = 224;
    static constexpr int64_t input_width = 224;
};

#endif