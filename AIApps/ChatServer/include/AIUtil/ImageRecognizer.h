#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

class ImageRecognizer
{
public:
    // 构造函数，加载模型和标签文件
    explicit ImageRecognizer(const std::string &model_path,
                             const std::string &label_path = "/root/imagenet_classes.txt");

    // 从文件路径预测图像类别
    std::string PredictFromFile(const std::string &image_path);

    // 从内存缓冲区预测图像类别
    std::string PredictFromBuffer(const std::vector<unsigned char> &image_data);

    // 从 OpenCV Mat 预测图像类别
    std::string PredictFromMat(const cv::Mat &img);

private:
    Ort::Env env;
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::AllocatorWithDefaultOptions> allocator;

    std::string input_name;
    std::string output_name;
    std::vector<int64_t> input_shape;
    int input_height{}, input_width{};

    std::vector<std::string> labels; // 类别标签

    void LoadLabels(const std::string &label_path);
};
