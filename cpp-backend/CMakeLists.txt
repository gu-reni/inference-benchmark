cmake_minimum_required(VERSION 3.16)
project(InferenceBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Release)

# 查找依赖
set(OpenCV_DIR "/usr/lib/x86_64-linux-gnu/cmake/opencv4")
find_package(OpenCV REQUIRED)
# 设置 ONNX Runtime 的路径
set(ORT_INCLUDE_DIR "/usr/local/include")
set(ORT_LIBRARY_DIR "/usr/local/lib")

# 创建导入的库对象 onnxruntime
add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
    IMPORTED_LOCATION "${ORT_LIBRARY_DIR}/libonnxruntime.so"
    INTERFACE_INCLUDE_DIRECTORIES "${ORT_INCLUDE_DIR}"
)

# 定义变量以保持兼容性
set(ONNXRuntime_LIBRARIES onnxruntime)

# 添加可执行文件
add_executable(inference_server 
    src/main.cpp
    src/inference_engine.cpp
)

# 包含头文件目录
target_include_directories(inference_server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
)

# 链接库
target_link_libraries(inference_server 
    ${OpenCV_LIBS}
    ${ONNXRuntime_LIBRARIES}
    pthread
)

# 设置输出目录
set_target_properties(inference_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)