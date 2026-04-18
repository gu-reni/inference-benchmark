#include "inference_engine.h"
#include <httplib.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

std::string vector_to_json_array(const std::vector<float>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << std::fixed << std::setprecision(4) << vec[i];
        if (i != vec.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

std::string vector_to_json_array_int(const std::vector<int>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) oss << ",";
    }
    oss << "]";
    return oss.str();
}

int main(int argc, char* argv[]) {
    std::string model_path = "../models/resnet18.onnx";
    if (argc > 1) {
        model_path = argv[1];
    }
    
    std::cout << "Initializing InferenceEngine with model: " << model_path << std::endl;
    InferenceEngine engine(model_path);
    
    httplib::Server svr;
    svr.set_payload_max_length(10 * 1024 * 1024);
    
    svr.Post("/api/inference", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::vector<char> img_data(req.body.begin(), req.body.end());
            cv::Mat img = cv::imdecode(img_data, cv::IMREAD_COLOR);
            
            if (img.empty()) {
                res.status = 400;
                res.set_content(R"({"error": "Invalid image data"})", "application/json");
                return;
            }
            
            auto [top5_values, top5_indices, time_us] = engine.infer(img);
            
            std::ostringstream json;
            json << "{";
            json << "\"top5_values\":" << vector_to_json_array(top5_values) << ",";
            json << "\"top5_indices\":" << vector_to_json_array_int(top5_indices) << ",";
            json << "\"inference_time_us\":" << time_us;
            json << "}";
            
            res.set_content(json.str(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            std::string err = R"({"error": ")" + std::string(e.what()) + R"("})";
            res.set_content(err, "application/json");
        }
    });
    
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status": "ok"})", "application/json");
    });
    
    std::cout << "C++ Inference Server listening on http://0.0.0.0:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}