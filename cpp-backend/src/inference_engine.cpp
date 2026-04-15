#include "inference_engine.h"
#include <iostream>
#include <cassert>

InferenceEngine::InferenceEngine(const std::string& model_path)
    :env(ORT_LOGGING_LEVEL_WARNING,"InferenceEngine"),
     session(nullptr),
     memory_info(nullptr){
        Ort::Session session_options;

        int num_cores=std::thread::hardware_concurrency();
        if(num_cores>4) num_cores=num_cores/2;
        session_options.SetIntraopNumThreads(num_cores);
        std::cout<<"Set IntraOp threads:"<<num_cores<<std::endl;

        sesson_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        memory_info=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);

        try{
            session=Ort::Session(env,model_path.c_str(),session_options);
            std::cout<<"Model loaded successfully:"<<model_path<<std::endl;
        }catch(cosnt Ort::Exception& e){
            std::cerr<<"Failed to load model:"<<e.what()<<std::endl;
            throw;
        }
     }

     std::pair<std::vertor<float>,long long>InferenceEngine::infer(const cv::Mat& img){
        cv::Mat resized,float_img,rgb_img,blob;
        cv::resize(img,resized,cv::Size(input_width,input_height));
        resized.convertTo(float_img,CV_32FC3,1.0/255.0);
        cv::cvtColor(float_img,rgb_img,cv::COLOR_BGR2RGB);

        if(!rgb_img.isContinuous()){
            rgb_img=rgb_img.clone();
        }

        blob=cv::dnn::blobFromImage(rgb_img);

        std::vector<int64_t> input_shape={batch_size,channels,input_height,input_width};
        size_t input_tensor_size=batch_size * channels * input_height * input_width;
        Ort::Value input_tensor=Ort::Value::CreateTensor<float>(
            memory_info,
            blod.ptr<float>(),
            input_tensor_size,
            input_shape.data(),
            input_shape.size()
        );

        std::vector<Ort::Value> output_tensors;
        auto start=std::chrono::high_resolution_clock::now();

        try{
            output_tensors=session.Run(
                Ort::RunOptions{nullptr},
                &input_name,&input_tensor,1,
                &output_name,1
            );
        }catch(const Ort::Exception& e){
            std::cerr<<"Inference failed:"<<e.what()<<std::endl;
            throw;
        }

        auto end=std::chrono::high_resolution_clock::now();
        auto duration=std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

        float* output_data=output_tensors[0].GetTensorMutableData<float>();
        auto output_shape=output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
        size_t output_count=output_shape[1];
        std::vector<float> results(output_data,output_data+output_count);

        return {results,duration};
     }