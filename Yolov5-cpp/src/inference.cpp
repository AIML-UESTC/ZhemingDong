
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

ObjectDetect::ObjectDetect(const std::string& modelPath): modelPath_(modelPath) {}

Result ObjectDetect::loadMultiModel(uint32_t loadModelCnt)
{
    Result ret;
    std::string loadModelName_;

    if(loadModelCnt == 0)
    {
        ERROR_LOG("no any models found in %s",modelPath_.c_str());
        return FAILED;
    }

    for (uint32_t i=0;i<loadModelCnt;i++)/* 加载多个模型 */
    {
        loadModelName_ = modelPath_ + to_string(gModeInfo_[i].modNum)+ ".om";
        ret = model_.LoadModelFromFileWithMem(loadModelName_,gModeInfo_[i].modId);
        if (ret != SUCCESS) {
            ERROR_LOG("execute LoadModelFromFileWithMem failed,ret = %d",ret);
            return FAILED;
        }
        else
        {
            INFO_LOG("%s is load in success,modelId : %d",loadModelName_.c_str(),gModeInfo_[i].modId);
        }
    }
    return SUCCESS;
}

void ObjectDetect ::storeModelInfo(uint32_t infoIndex,MOD_INDEX_INFO storeInfo)
{
    gModeInfo_[infoIndex].modIndex        = storeInfo.modIndex;
    gModeInfo_[infoIndex].modNum          = storeInfo.modNum;
    gModeInfo_[infoIndex].modId           = storeInfo.modId;
    gModeInfo_[infoIndex].modWidth        = storeInfo.modWidth;
    gModeInfo_[infoIndex].modHeight       = storeInfo.modHeight;
    gModeInfo_[infoIndex].modthresholdVal = storeInfo.modthresholdVal;
    
    // cout << "g_modeIndex = " << gModeInfo_[infoIndex].modIndex << endl;
    // cout << "g_modeOrdinal = " << gModeInfo_[infoIndex].modNum << endl;
    // cout << "g_inferWidth = " << gModeInfo_[infoIndex].modWidth << endl;
    // cout << "g_inferHeight = " << gModeInfo_[infoIndex].modHeight << endl;
    // cout << "g_thresholdValue = " << gModeInfo_[infoIndex].modthresholdVal << endl;
}

void ObjectDetect ::storeModelNum(uint32_t totalNum)
{
    gModTotalNum_ = totalNum;
}

uint32_t ObjectDetect ::checkModelId(uint32_t inferModelIndex)
{
    for (uint32_t i=0;i<gModTotalNum_;i++)
    {
        if (inferModelIndex == gModeInfo_[i].modNum)
        {
            modelWidth_ = gModeInfo_[i].modWidth;
            modelHeight_ = gModeInfo_[i].modHeight;
            return gModeInfo_[i].modId;
        }
    }
    return -1;
}

Result ObjectDetect :: inferBeforeCreatDesc(uint32_t gInferId, uint32_t inferWidth ,uint32_t inferHeight)
{

    Result ret = model_.CreateDesc(gInferId);
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateDesc failed");
        return FAILED;
    }

    ret = model_.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateOutput failed");
        return FAILED;
    }
    printf("crear output success\r\n");
    ret = CreateImageInfoBuffer(inferWidth,inferHeight);
    if (ret != SUCCESS) {
        ERROR_LOG("Create image info buf failed");
        return FAILED;
    }
    printf("crear ImageInfoBuffer success\r\n");
    return SUCCESS;
}

void ObjectDetect :: inferEndDestroyDesc()
{
    model_.DestroyOutput();
    model_.DestroyDesc();
}


Result ObjectDetect::Init(uint32_t loadModelCnt){
    Result ret = model_.InitResource(deviceId_,context_,stream_);
    if (ret != SUCCESS) {
        ERROR_LOG("Init Resource failed");
        return FAILED;
    }

    ret = loadMultiModel(loadModelCnt);
    if (ret != SUCCESS) {
        ERROR_LOG("LoadMuldel failed");
        return FAILED;
    }
    return SUCCESS;
};

Result ObjectDetect::CreateImageInfoBuffer(uint32_t inferImageWidth_, uint32_t inferImageHeight_)
{
    const float imageInfo[4] = {(float)inferImageWidth_, (float)inferImageWidth_,
                                (float)inferImageHeight_, (float)inferImageHeight_};
    imageInfoSize_ = sizeof(imageInfo);

    // std :: cout << "inferImageWidth_ :"<<inferImageWidth_<<endl;
    // std :: cout << "inferImageHeight_ :"<<inferImageHeight_<<endl;
    // std :: cout << "imageInfoSize_ :"<<imageInfoSize_<<endl;
    
    aclError ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl get run mode failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }

    if (runMode_ == ACL_HOST){
        // INFO_LOG("Host mode...");
        imageInfoBuf_ = CopyDataHostToDevice((void *)imageInfo, imageInfoSize_);
    }  
    else{
        // INFO_LOG("Device mode...");
        imageInfoBuf_ = CopyDataDeviceToDevice((void *)imageInfo, imageInfoSize_);  
    }
    if (imageInfoBuf_ == nullptr) {
        ERROR_LOG("Copy image info to device failed");
        return FAILED;
    }   

    return SUCCESS;
}

Result ObjectDetect::Inference(aclmdlDataset*& inferenceOutput, Image& image, uint32_t detectModelId)
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

    ret = model_.Execute(detectModelId);
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


Result ObjectDetect::Postprocess(Image& image, aclmdlDataset*& modelOutput, DETECT_RESULT &gOutResult)
{   
    struct  timeval tstart,tend;
    uint32_t i;
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
    INFO_LOG("totalBox = %d\r\n",totalBox);
    vector<BoundingBox> detectResults;

    float widthScale = (float)(image.imgWidth) / modelWidth_;
    float heightScale = (float)(image.imgHeight) / modelHeight_;

    for (i = 0; i < totalBox; i++) {
        BoundingBox boundBox;

        uint32_t score = uint32_t(detectData[totalBox * SCORE + i] * 100);
        boundBox.lt_x = detectData[totalBox * TOPLEFTX + i] * widthScale;
        boundBox.lt_y = detectData[totalBox * TOPLEFTY + i] * heightScale;
        boundBox.rb_x = detectData[totalBox * BOTTOMRIGHTX + i] * widthScale;
        boundBox.rb_y = detectData[totalBox * BOTTOMRIGHTY + i] * heightScale;
        uint32_t objIndex = (uint32_t)detectData[totalBox * LABEL + i];
        boundBox.text = yolov3Label[objIndex] + std::to_string(score) + "\%";
        /* 填充需要返回的结果内容 */
        gOutResult.picIndex = image.deImgIndex;
        gOutResult.objInfo[i].objLabel   = (uint16_t)objIndex;
        gOutResult.objInfo[i].obj_x      = (uint16_t)((boundBox.rb_x - boundBox.lt_x) / 2 + boundBox.lt_x);/* 中心坐标X */
        gOutResult.objInfo[i].obj_y      = (uint16_t)((boundBox.lt_y - boundBox.rb_y) / 2 + boundBox.rb_y);/* 中心坐标y */
        gOutResult.objInfo[i].width      = (uint16_t)image.imgWidth;
        gOutResult.objInfo[i].height     = (uint16_t)image.imgHeight;
        gOutResult.objInfo[i].confidence = (uint16_t)score;
        INFO_LOG("obj   x:%d , y:%d",gOutResult.objInfo[i].obj_x ,gOutResult.objInfo[i].obj_y );
        INFO_LOG("image w:%d , h:%d",gOutResult.objInfo[i].width ,gOutResult.objInfo[i].height);
        INFO_LOG("confidence:%d",gOutResult.objInfo[i].confidence);
        INFO_LOG("obj[%d] result:%s\n",i,boundBox.text.c_str());
        // INFO_LOG("boundBox.lt_x = %d---boundBox.lt_y = %d",boundBox.lt_x, boundBox.lt_y);
        // INFO_LOG("boundBox.rb_x = %d---boundBox.rb_y = %d",boundBox.rb_x, boundBox.rb_y);
        

        detectResults.emplace_back(boundBox);
    }
    gOutResult.dTarNum = i;/* 目标数量 */

    // DrawBoundBoxToImage(detectResults, origImagePath);
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

    /* aclGetDataBufferSize此接口后续版本会废弃，请使用aclGetDataBufferSizeV2接口 */
    size_t bufferSize = aclGetDataBufferSizeV2(dataBuffer);
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
    for (uint32_t i = 0; i < detectionResults.size(); ++i) {
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