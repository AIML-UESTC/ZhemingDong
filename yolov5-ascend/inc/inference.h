#pragma once
#include <memory>
#include <vector>
#include "dvpp_preProcess.h"
#include "model_process.h"

/**
* ClassifyProcess
*/



class ObjectDetect {
public:
    ObjectDetect(const std::string& modelPath, const uint32_t& modelWidth,const uint32_t& modelHeight);
    Result Init();
    Result Preprocess(const string &image_path,Image& image);
    Result Inference(aclmdlDataset*& inferenceOutput, Image& image);
    Result Postprocess(Image& image, aclmdlDataset*& modelOutput,
                       const std::string& origImagePath);
    void DestroyResource();
private:
    void* GetInferenceOutputItem(uint32_t& itemDataSize,
                                 aclmdlDataset* inferenceOutput,
                                 uint32_t idx);
    void DrawBoundBoxToImage(std::vector<BoundingBox>& detectionResults,
                             const std::string& origImagePath);

    Result CreateImageInfoBuffer();

private:    
    void* imageInfoBuf_ = nullptr;
    void* imageDataBuf_ = nullptr ;
    int32_t deviceId_ = 0;
    aclrtContext context_;
    aclrtStream stream_;
    
    uint32_t imageInfoSize_ = 0;


    ModelProcess model_;
    string modelPath_;
    uint32_t modelWidth_;
    uint32_t modelHeight_;

    PreProcess dvpp_;
    
    aclrtRunMode runMode_;
};