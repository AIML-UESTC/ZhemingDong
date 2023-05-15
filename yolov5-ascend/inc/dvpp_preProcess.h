#ifndef RC_MAIN_H
#define RC_MAIN_H
#pragma once

#include <iostream>
#include <string>
#include "utils.h"

using namespace std;

static const int PNG = 0;
static const int JPG = 1;

class Decode{
public:
    Decode() = default;

    void DestroyResource();

    Result Process(PicDesc&,aclrtRunMode,acldvppChannelDesc*,aclrtStream);
    void* GetDeviceBufferOfPicture(PicDesc&, uint32_t&, aclrtRunMode );
    void SetInput(void *inDevBuffer, int inDevBufferSize, int inputWidth, int inputHeight);
    void* getDataPtr() {return decodeOutDevBuffer_;}
private:

    void *inDevBuffer_ = nullptr;  // decode input buffer
    uint32_t inDevBufferSize_ = 0; // dvpp input buffer size

    void* decodeOutDevBuffer_= nullptr; // decode output buffer
    acldvppPicDesc *decodeOutputDesc_= nullptr; //decode output desc
    uint32_t decodeDataSize_=0;

    uint32_t inputWidth_= 0; // input pic width
    uint32_t inputHeight_= 0; // input pic height
}; 

class Resize{
public:
    Resize() = default;

    Result InitResource(PicDesc& );
    void DestroyResource();

    Result Process(Image& image,void* inputPtr,aclrtRunMode runMode,acldvppChannelDesc* dvppChannelDesc,aclrtStream stream);
    void* GetDeviceBufferOfPicture(PicDesc&, uint32_t&, aclrtRunMode );
    void SetInput(void *inDevBuffer, int inDevBufferSize, int inputWidth, int inputHeight);
    void* getDataPtr() {return resizeOutDevBuffer_ ; }
    uint32_t getDataSize() {return resizeOutDevBufferSize_ ; }

private:
    void *outDevBuffer_= nullptr;  // decode output buffer
    uint32_t inDevBufferSize_= 0; // dvpp input buffer size

    void* resizeOutDevBuffer_= nullptr; // resize output buffer
    uint32_t resizeOutDevBufferSize_= 0; // dvpp input buffer size
    acldvppPicDesc *resizeInputDesc_= nullptr; //resize input desc
    acldvppPicDesc *resizeOutputDesc_= nullptr; //resize output desc
    acldvppResizeConfig *resizeConfig_ = nullptr;
    uint32_t resizeDataSize_=0;

    uint32_t inputWidth_=0; // input pic width
    uint32_t inputHeight_=0; // input pic height
    acldvppPixelFormat format_;
}; 

class PreProcess{
public:
    PreProcess() = default;

    Result InitResource(int32_t& deviceId,aclrtContext& context,aclrtStream& stream);
    Result DecodeAndResize(const string &image_path, Image& image, bool saveImg=0);
    void DestroyResource();
    void createDvppChannel();

private:
    bool isDevice_;
    int32_t deviceId_ = 0;
    aclrtContext context_;
    aclrtStream stream_;
    acldvppChannelDesc *dvppChannelDesc_;
    aclrtRunMode runMode_;
    Decode imgDecode_;
    Resize imgResize_;
};

uint32_t SaveOutputImg(const char *fileName, const void *devPtr, uint32_t dataSize,aclrtRunMode runMode);


#endif