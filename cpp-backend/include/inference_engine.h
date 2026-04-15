#ifndef INFERENCE_ENGINE_H
#define INFERENCE_ENGINE_H

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <chrono>

class InferenceEngine{
public:
    explicit InferenceEngine(const std::string& model_path);
    ~InferenceEngine()=default;
    std::pair<std::vector<float>,long long> infer(const cv::Mat& img);
private:
    Ort::Env env;
    Ort::Session session;
    Ort::MemoryInfo memory_info;

    static constexpr const char* input_name="input";
    static constexpr const char* output_name="output";

    static constexpr int64_t batch_size=1;
    static constexpr int64_t channels=3;
    static constexpr int64_t input_height=224;
    static constexpr int64_t input_width=224;
};

#endif