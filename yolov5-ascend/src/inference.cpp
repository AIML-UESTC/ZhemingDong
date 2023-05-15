
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include "inference.h"
#include "model_process.h"

namespace {
    const static std::vector<std::string> yolov3Label = { "person", "bicycle", "car", "motorbike",
        "aeroplane", "bus", "train", "truck", "boat",
        "traffic light", "fire hydrant", "stop sign", "parking meter",
        "bench", "bird", "cat", "dog", "horse",
        "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie",
        "suitcase", "frisbee", "skis", "snowboard", "sports ball",
        "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana",
        "apple", "sandwich", "orange", "broccoli", "carrot",
        "hot dog", "pizza", "donut", "cake", "chair",
        "sofa", "potted plant", "bed", "dining table", "toilet",
        "TV monitor", "laptop", "mouse", "remote", "keyboard",
        "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors",
        "teddy bear", "hair drier", "toothbrush" };

    const uint32_t g_bBoxDataBufId = 0;
    const uint32_t g_boxNumDataBufId = 1;

    enum BBoxIndex { TOPLEFTX = 0, TOPLEFTY, BOTTOMRIGHTX, BOTTOMRIGHTY, SCORE, LABEL };
    // bounding box line solid
    const uint32_t g_lineSolid = 2;

    // output image prefix
    const string g_outputFilePrefix = "out_";
    // opencv draw label params.
    const double g_fountScale = 0.5;
    const cv::Scalar g_fontColor(0, 0, 255);
    const uint32_t g_labelOffset = 11;
    const string g_fileSperator = "/";

    // opencv color list for boundingbox
    const vector<cv::Scalar> g_colors {
        cv::Scalar(237, 149, 100), cv::Scalar(0, 215, 255), cv::Scalar(50, 205, 50),
        cv::Scalar(139, 85, 26) };

}

void* CopyDataToDevice(void* data, uint32_t dataSize, aclrtMemcpyKind policy)
{
    void* buffer = nullptr;
    aclError aclRet = aclrtMalloc(&buffer, dataSize, ACL_MEM_MALLOC_HUGE_FIRST);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("malloc device data buffer failed, aclRet is %d", aclRet);
        return nullptr;
    }

    aclRet = aclrtMemcpy(buffer, dataSize, data, dataSize, policy);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("Copy data to device failed, aclRet is %d", aclRet);
        (void)aclrtFree(buffer);
        return nullptr;
    }

    return buffer;
}

void* CopyDataDeviceToLocal(void* deviceData, uint32_t dataSize)
{
    uint8_t* buffer = new uint8_t[dataSize];
    if (buffer == nullptr) {
        ERROR_LOG("New malloc memory failed");
        return nullptr;
    }

    aclError aclRet = aclrtMemcpy(buffer, dataSize, deviceData, dataSize, ACL_MEMCPY_DEVICE_TO_HOST);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("Copy device data to local failed, aclRet is %d", aclRet);
        delete[](buffer);
        return nullptr;
    }

    return (void*)buffer;
}
void* CopyDataDeviceToDevice(void* deviceData, uint32_t dataSize)
{
    return CopyDataToDevice(deviceData, dataSize, ACL_MEMCPY_DEVICE_TO_DEVICE);
}

void* CopyDataHostToDevice(void* deviceData, uint32_t dataSize)
{
    return CopyDataToDevice(deviceData, dataSize, ACL_MEMCPY_HOST_TO_DEVICE);
}

ObjectDetect::ObjectDetect(const std::string& modelPath,const uint32_t& modelWidth,const uint32_t& modelHeight)
    : modelPath_(modelPath),modelWidth_(modelWidth), modelHeight_(modelHeight) {}

Result ObjectDetect::Init(){
    Result ret = dvpp_.InitResource(deviceId_,context_,stream_);
    if (ret != SUCCESS) {
        ERROR_LOG("Init Resource failed");
        return FAILED;
    }

    ret = 
    ret = model_.LoadModelFromFileWithMem(modelPath_);
    if (ret != SUCCESS) {
        ERROR_LOG("execute LoadModelFromFileWithMem failed");
        return FAILED;
    }

    ret = model_.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateDesc failed");
        return FAILED;
    }

    ret = model_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateOutput failed");
        return FAILED;
    }

    ret = CreateImageInfoBuffer();
    if (ret != SUCCESS) {
        ERROR_LOG("Create image info buf failed");
        return FAILED;
    }
    return SUCCESS;
};

Result ObjectDetect::CreateImageInfoBuffer()
{
    const float imageInfo[4] = {(float)modelWidth_, (float)modelWidth_,
                                (float)modelHeight_, (float)modelHeight_};
    imageInfoSize_ = sizeof(imageInfo);
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl get run mode failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    if (runMode_ == ACL_HOST)
        imageInfoBuf_ = CopyDataHostToDevice((void *)imageInfo, imageInfoSize_);
    else
        imageInfoBuf_ = CopyDataDeviceToDevice((void *)imageInfo, imageInfoSize_);
    if (imageInfoBuf_ == nullptr) {
        ERROR_LOG("Copy image info to device failed");
        return FAILED;
    }

    return SUCCESS;
}

Result ObjectDetect::Preprocess(const string &image_path,Image& image)
{
    struct  timeval tstart,tend;
    double timeuse;
    gettimeofday(&tstart,NULL);
    aclError ret=dvpp_.DecodeAndResize(image_path,image,1);
    if (ret != SUCCESS){
        ERROR_LOG("Decode and resize failed, error code %d" ,ret);
        return FAILED;
    }
    gettimeofday(&tend,NULL);
    timeuse = 1000000*(tend.tv_sec - tstart.tv_sec) + \
				(tend.tv_usec - tstart.tv_usec);
    INFO_LOG("Preprocess time: %f ms",timeuse/1000);
    return SUCCESS;
}


Result ObjectDetect::Inference(aclmdlDataset*& inferenceOutput, Image& image)
{    
    struct  timeval tstart,tend;
    double timeuse;
    gettimeofday(&tstart,NULL);
    aclError ret = model_.CreateInput(image.data,image.dataSize,
                                      imageInfoBuf_, imageInfoSize_);
    if (ret != SUCCESS) {
        ERROR_LOG("Create mode input dataset failed");
        return FAILED;
    }

    ret = model_.Execute();
    if (ret != SUCCESS) {
        ERROR_LOG("Execute model inference failed");
        return FAILED;
    }

    inferenceOutput = model_.GetModelOutputData();
    gettimeofday(&tend,NULL);
    timeuse = 1000000*(tend.tv_sec - tstart.tv_sec) + \
				(tend.tv_usec - tstart.tv_usec);
    INFO_LOG("Inference time: %f ms",timeuse/1000);
    model_.DestroyInput();
    (void)acldvppFree(image.data);
    image.data = nullptr;
    return SUCCESS;
}


Result ObjectDetect::Postprocess(Image& image, aclmdlDataset*& modelOutput,
                       const std::string& origImagePath)
{   
    struct  timeval tstart,tend;
    double timeuse;
    gettimeofday(&tstart,NULL);
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl get run mode failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    uint32_t dataSize = 0;
    float* detectData = (float *)GetInferenceOutputItem(dataSize, modelOutput,
    g_bBoxDataBufId);
    uint32_t* boxNum = (uint32_t *)GetInferenceOutputItem(dataSize, modelOutput,
    g_boxNumDataBufId);

    if (boxNum == nullptr) return FAILED;
    uint32_t totalBox = boxNum[0];
    vector<BoundingBox> detectResults;

    float widthScale = (float)(image.picdesc.width) / modelWidth_;
    float heightScale = (float)(image.picdesc.height) / modelHeight_;

    for (uint32_t i = 0; i < totalBox; i++) {
        BoundingBox boundBox;

        uint32_t score = uint32_t(detectData[totalBox * SCORE + i] * 100);
        boundBox.lt_x = detectData[totalBox * TOPLEFTX + i] * widthScale;
        boundBox.lt_y = detectData[totalBox * TOPLEFTY + i] * heightScale;
        boundBox.rb_x = detectData[totalBox * BOTTOMRIGHTX + i] * widthScale;
        boundBox.rb_y = detectData[totalBox * BOTTOMRIGHTY + i] * heightScale;

        uint32_t objIndex = (uint32_t)detectData[totalBox * LABEL + i];
        boundBox.text = yolov3Label[objIndex] + std::to_string(score) + "\%";
        // printf("%d %d %d %d %s\n", boundBox.lt_x, boundBox.lt_y,
        //        boundBox.rb_x, boundBox.rb_y, boundBox.text.c_str());

        detectResults.emplace_back(boundBox);
    }
    
    //DrawBoundBoxToImage(detectResults, origImagePath);
    if (runMode_ == ACL_HOST) {
        delete[]((uint8_t *)detectData);
        delete[]((uint8_t*)boxNum);
    }
    gettimeofday(&tend,NULL);
    timeuse = 1000000*(tend.tv_sec - tstart.tv_sec) + \
				(tend.tv_usec - tstart.tv_usec);
    INFO_LOG("Postprocess time: %f ms",timeuse/1000);
    return SUCCESS;

}
void* ObjectDetect::GetInferenceOutputItem(uint32_t& itemDataSize,
                                           aclmdlDataset* inferenceOutput,
                                           uint32_t idx)
{
    aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(inferenceOutput, idx);
    if (dataBuffer == nullptr) {
        ERROR_LOG("Get the %dth dataset buffer from model "
                  "inference output failed", idx);
        return nullptr;
    }

    void* dataBufferDev = aclGetDataBufferAddr(dataBuffer);
    if (dataBufferDev == nullptr) {
        ERROR_LOG("Get the %dth dataset buffer address "
                  "from model inference output failed", idx);
        return nullptr;
    }

    size_t bufferSize = aclGetDataBufferSize(dataBuffer);
    if (bufferSize == 0) {
        ERROR_LOG("The %dth dataset buffer size of "
                  "model inference output is 0", idx);
        return nullptr;
    }
    void* data = nullptr;
    if (runMode_ == ACL_HOST) {
        data = CopyDataDeviceToLocal(dataBufferDev, bufferSize);
        if (data == nullptr) {
            ERROR_LOG("Copy inference output to host failed");
            return nullptr;
        }
    } else {
        data = dataBufferDev;
    }

    itemDataSize = bufferSize;
    return data;
}

void ObjectDetect::DrawBoundBoxToImage(vector<BoundingBox>& detectionResults,
                                       const string& origImagePath)
{
    cv::Mat image = cv::imread(origImagePath, CV_LOAD_IMAGE_UNCHANGED);
    for (int i = 0; i < detectionResults.size(); ++i) {
        cv::Point p1, p2;
        p1.x = detectionResults[i].lt_x;
        p1.y = detectionResults[i].lt_y;
        p2.x = detectionResults[i].rb_x;
        p2.y = detectionResults[i].rb_y;
        cv::rectangle(image, p1, p2, g_colors[i % g_colors.size()], g_lineSolid);
        cv::putText(image, detectionResults[i].text, cv::Point(p1.x, p1.y + g_labelOffset),
                    cv::FONT_HERSHEY_COMPLEX, g_fountScale, g_fontColor);
    }

    string folderPath = "./output";
    if (NULL == opendir(folderPath.c_str())) {
        mkdir(folderPath.c_str(), 0775);
    }
    int pos = origImagePath.find_last_of("/");
    string filename(origImagePath.substr(pos + 1));
    stringstream sstream;
    sstream.str("");
    sstream << "./output/out_" << filename;
    cv::imwrite(sstream.str(), image);
}

void  ObjectDetect::DestroyResource(){
    dvpp_.DestroyResource();
}