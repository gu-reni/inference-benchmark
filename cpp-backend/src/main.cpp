#include "inference_engine.h"
#include <httplib.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

// 辅助函数：将 float 向量转为 JSON 数组字符串
std::string vector_to_json_array(const std::vector<float>& vec, size_t top_k = 5) {
    std::ostringstream oss;
    oss << "[";
    size_t count = std::min(top_k, vec.size());
    for (size_t i = 0; i < count; ++i) {
        oss << std::fixed << std::setprecision(4) << vec[i];
        if (i != count - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

int main(int argc, char* argv[]) {
    // 模型路径（支持命令行参数）
    std::string model_path = "../models/resnet18.onnx";
    if (argc > 1) {
        model_path = argv[1];
    }
    
    std::cout << "Initializing InferenceEngine with model: " << model_path << std::endl;
    InferenceEngine engine(model_path);
    
    // 创建 HTTP 服务器
    httplib::Server svr;
    
    // POST /api/inference 接口
    svr.Post("/api/inference", [&](const httplib::Request& req, httplib::Response& res) {
        // 从请求体读取图片二进制数据
        std::vector<char> img_data(req.body.begin(), req.body.end());
        cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);
        
        if (img.empty()) {
            res.status = 400;
            res.set_content(R"({"error": "Invalid image data"})", "application/json");
            return;
        }
        
        try {
            // 执行推理
            auto [results, time_us] = engine.infer(img);
            
            // 构造 JSON 响应
            std::ostringstream json;
            json << "{";
            json << "\"top5\":" << vector_to_json_array(results, 5) << ",";
            json << "\"inference_time_us\":" << time_us;
            json << "}";
            
            res.set_content(json.str(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            std::string err = R"({"error": ")" + std::string(e.what()) + R"("})";
            res.set_content(err, "application/json");
        }
    });
    
    // 健康检查接口
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status": "ok"})", "application/json");
    });
    
    std::cout << "C++ Inference Server listening on http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}