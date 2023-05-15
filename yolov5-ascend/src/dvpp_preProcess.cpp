#include <iostream>
#include <fstream>
#include <time.h>  
#include "dvpp_preProcess.h"


uint32_t alignment(uint32_t origSize, uint32_t alignment)
{
    if (alignment == 0) {
        return 0;
    }
    uint32_t alignmentH = alignment - 1;
    return (origSize + alignmentH) / alignment * alignment;
}

Result PreProcess::InitResource(int32_t& deviceId,aclrtContext& context,aclrtStream& stream){
    // acl init 
    const char *aclConfigPath = "../src/acl.json";
    aclError ret = aclInit(aclConfigPath);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl init failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    INFO_LOG("acl init success");

    // set device
    ret = aclrtSetDevice(deviceId);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl set device %d failed, errorCode = %d", deviceId, static_cast<int32_t>(ret));
        return FAILED;
    }
    INFO_LOG("set device %d success", deviceId);

    // create context (set current)
    ret = aclrtCreateContext(&context, deviceId);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl create context failed, deviceId = %d, errorCode = %d",
                  deviceId, static_cast<int32_t>(ret));
        return FAILED;
    }
    INFO_LOG("create context success");

    // create stream
    ret = aclrtCreateStream(&stream);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl create stream failed, deviceId = %d, errorCode = %d",
                  deviceId, static_cast<int32_t>(ret));
        return FAILED;
    }
    INFO_LOG("create stream success");

    // get run mode
    ret = aclrtGetRunMode(&runMode_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl get run mode failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    isDevice_ = (runMode_ == ACL_DEVICE);
    INFO_LOG("get run mode success");

    dvppChannelDesc_ = acldvppCreateChannelDesc();
    if (dvppChannelDesc_ == nullptr) {
        ERROR_LOG("acldvppCreateChannelDesc failed");
        return FAILED;
    }

    ret = acldvppCreateChannel(dvppChannelDesc_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acldvppCreateChannel failed, aclRet = %d", ret);
        return FAILED;
    }

    deviceId_ = deviceId;
    context_ = context;
    stream_ = stream;
    return SUCCESS;
}

void PreProcess::DestroyResource(){
    aclError ret;
    imgDecode_.DestroyResource();

    if (stream_ != nullptr) {
        ret = aclrtDestroyStream(stream_);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("destroy stream failed");
        }
        stream_ = nullptr;
    }
    INFO_LOG("end to destroy stream");

    if (context_ != nullptr) {
        ret = aclrtDestroyContext(context_);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("destroy context failed");
        }
        context_ = nullptr;
    }
    INFO_LOG("end to destroy context");

    ret = aclrtResetDevice(deviceId_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("reset device failed");
    }
    INFO_LOG("end to reset device is %d", deviceId_);

    ret = aclFinalize();
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("finalize acl failed");
    }
    INFO_LOG("end to finalize acl");
}
   
Result PreProcess::DecodeAndResize(const string &image_path, Image& image, bool saveImg){
        //4.Create image data processing channel
    
    INFO_LOG("dvpp init resource success");
        
    uint32_t image_width = 0;
    uint32_t image_height = 0;
    image.picdesc = {image_path, image_width, image_height};
    if (image.imgFormat == "jpg")
    {
        image.picdesc.inputImgFormat = JPG;
        image.picdesc.outputImgFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    }
    else
    {
        image.picdesc.inputImgFormat = PNG;
        image.picdesc.outputImgFormat = PIXEL_FORMAT_RGB_888;
    }
    INFO_LOG("start to process picture:%s", image.picdesc.picName.c_str());
    aclError ret = imgDecode_.Process(image.picdesc,runMode_,dvppChannelDesc_,stream_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("decode failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    ret = imgResize_.Process(image,imgDecode_.getDataPtr(),runMode_,dvppChannelDesc_,stream_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("resize failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }
    image.data      =   imgResize_.getDataPtr();
    image.dataSize  =   imgResize_.getDataSize();
    image.imgFormat =   "yuv420";
    saveImg = 0;
    if(saveImg){
        const char* saveFileName = "out.yuv";
        ret = SaveOutputImg(saveFileName,imgResize_.getDataPtr(),image.dataSize,runMode_);
        if (ret != ACL_SUCCESS) {
        ERROR_LOG("save img failed, errorCode = %d", static_cast<int32_t>(ret));
        return FAILED;
    }   
    INFO_LOG("save img success");
    }
    return SUCCESS;
}

void* Decode::GetDeviceBufferOfPicture(PicDesc &picDesc, uint32_t &devPicBufferSize,aclrtRunMode runMode){
    if (picDesc.picName.empty()) {
        ERROR_LOG("picture file name is empty");
        return nullptr;
        }

    FILE *fp = fopen(picDesc.picName.c_str(), "rb");
    if (fp == nullptr) {
        ERROR_LOG("open file %s failed", picDesc.picName.c_str());
        return nullptr;
        }

    fseek(fp, 0, SEEK_END);
    uint32_t fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint32_t inputBuffSize = fileLen;

    char* inputBuff = new(std::nothrow) char[inputBuffSize];
    size_t readSize = fread(inputBuff, sizeof(char), inputBuffSize, fp);
    if (readSize < inputBuffSize) {
        ERROR_LOG("need read file %s %u bytes, but only %zu readed",
        picDesc.picName.c_str(), inputBuffSize, readSize);
        delete[] inputBuff;
		fclose(fp);
        return nullptr;
        }

    int32_t ch=0;
    if (picDesc.inputImgFormat == JPG )
    {   
        aclError aclRet = acldvppJpegGetImageInfo(inputBuff, inputBuffSize, &picDesc.width, &picDesc.height, &ch);
        if (aclRet != ACL_SUCCESS) {
            ERROR_LOG("get jpeg image info failed, errorCode is %d", static_cast<int32_t>(aclRet));
            delete[] inputBuff;
            fclose(fp);
            return nullptr;
            }
        aclRet = acldvppJpegPredictDecSize(inputBuff, inputBuffSize, PIXEL_FORMAT_YUV_SEMIPLANAR_420, &picDesc.decodeSize);    
        if (aclRet != ACL_SUCCESS) {
            ERROR_LOG("get jpeg decode size failed, errorCode is %d", static_cast<int32_t>(aclRet));
            delete[] inputBuff;
            fclose(fp);
            return nullptr;
            }
        INFO_LOG("get jpeg image info successed, width=%d, height=%d, decodeSize=%d", picDesc.width, picDesc.height, picDesc.decodeSize);
        }
    if (picDesc.inputImgFormat == PNG )
    {   
        aclError aclRet = acldvppPngGetImageInfo(inputBuff, inputBuffSize, &picDesc.width, &picDesc.height, &ch);
        if (aclRet != ACL_SUCCESS) {
            ERROR_LOG("get png image info failed, errorCode is %d", static_cast<int32_t>(aclRet));
            delete[] inputBuff;
            fclose(fp);
            return nullptr;
            }

        aclRet = acldvppPngPredictDecSize(inputBuff, inputBuffSize, PIXEL_FORMAT_RGB_888, &picDesc.decodeSize);    
        if (aclRet != ACL_SUCCESS) {
            ERROR_LOG("get jpeg decode size failed, errorCode is %d", static_cast<int32_t>(aclRet));
            delete[] inputBuff;
            fclose(fp);
            return nullptr;
            }
        INFO_LOG("get png image info successed, width=%d, height=%d, decodeSize=%d", picDesc.width, picDesc.height, picDesc.decodeSize);
        }
    
    void *inBufferDev = nullptr;
    aclError ret = acldvppMalloc(&inBufferDev, inputBuffSize);
    if (ret !=  ACL_SUCCESS) {
        delete[] inputBuff;
        ERROR_LOG("malloc device data buffer failed, aclRet is %d", ret);
		fclose(fp);
        return nullptr;
    }

    if (runMode == ACL_HOST) {
        ret = aclrtMemcpy(inBufferDev, inputBuffSize, inputBuff, inputBuffSize, ACL_MEMCPY_HOST_TO_DEVICE);
    }
    else {
        ret = aclrtMemcpy(inBufferDev, inputBuffSize, inputBuff, inputBuffSize, ACL_MEMCPY_DEVICE_TO_DEVICE);
    }
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("memcpy failed. Input host buffer size is %u",
        inputBuffSize);
        acldvppFree(inBufferDev);

        delete[] inputBuff;
		fclose(fp);
        return nullptr;
    }
    
    delete[] inputBuff;
    devPicBufferSize = inputBuffSize;
	fclose(fp);
    return inBufferDev; 
}

void Decode::SetInput(void *inDevBuffer, int inDevBufferSize, int inputWidth, int inputHeight)
{
    inDevBuffer_ = inDevBuffer;
    inDevBufferSize_ = inDevBufferSize;
    inputWidth_ = inputWidth;
    inputHeight_ = inputHeight;
}

void Decode::DestroyResource(){
    (void)acldvppFree(inDevBuffer_);
    inDevBuffer_ = nullptr;
     if (decodeOutputDesc_ != nullptr) {
        acldvppDestroyPicDesc(decodeOutputDesc_);
        decodeOutputDesc_ = nullptr;
    }
}

Result Decode::Process(PicDesc& testPic,aclrtRunMode runMode,acldvppChannelDesc* dvppChannelDesc,aclrtStream stream){
    // dvpp process
    /*3.Read the picture into memory. InDevBuffer_ indicates the memory for storing the input picture, 
	inDevBufferSize indicates the memory size, please apply for the input memory in advance*/
    uint32_t devPicBufferSize;
    void *picDevBuffer = GetDeviceBufferOfPicture(testPic, devPicBufferSize,runMode);
    if (picDevBuffer == nullptr) {
        ERROR_LOG("get pic device buffer failed,index is 0");
        return FAILED;
    }

    SetInput(picDevBuffer, devPicBufferSize, testPic.width, testPic.height);
    // InitDecodeOutputDesc
    uint32_t decodeOutWidthStride = alignment(testPic.width,128); // 128-byte alignment
    if(testPic.inputImgFormat == PNG)
        decodeOutWidthStride *=3;
    uint32_t decodeOutHeightStride = alignment(testPic.width,16); // 16-byte alignment
    
    // use acldvppJpegPredictDecSize to get output size.
    // uint32_t decodeOutBufferSize = decodeOutWidthStride * decodeOutHeightStride * 3 / 2; // yuv format size
    // uint32_t decodeOutBufferSize = testPic.decodeSize;
    aclError ret = acldvppMalloc(&decodeOutDevBuffer_, testPic.decodeSize);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acldvppMalloc jpegOutBufferDev failed, ret = %d", ret);
        return FAILED;
    }

    decodeOutputDesc_ = acldvppCreatePicDesc();
    if (decodeOutputDesc_ == nullptr) {
        ERROR_LOG("acldvppCreatePicDesc decodeOutputDesc failed");
        return FAILED;
    }
    acldvppSetPicDescData(decodeOutputDesc_, decodeOutDevBuffer_);
    // here the format shoud be same with the value you set when you get decodeOutBufferSize from
    acldvppSetPicDescFormat(decodeOutputDesc_, testPic.outputImgFormat); 
    acldvppSetPicDescWidth(decodeOutputDesc_, inputWidth_);
    acldvppSetPicDescHeight(decodeOutputDesc_, inputHeight_);
    acldvppSetPicDescWidthStride(decodeOutputDesc_, decodeOutWidthStride);
    acldvppSetPicDescHeightStride(decodeOutputDesc_, decodeOutHeightStride);
    acldvppSetPicDescSize(decodeOutputDesc_, testPic.decodeSize);
    if(testPic.inputImgFormat == JPG)
    {
        ret = acldvppJpegDecodeAsync(dvppChannelDesc, inDevBuffer_, inDevBufferSize_,
        decodeOutputDesc_, stream);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("acl dvpp Jpeg Decode Async failed, ret = %d", ret);
            return FAILED;
        }
        INFO_LOG("acl dvpp Jpeg Decode Async success");
    }
    if(testPic.inputImgFormat == PNG)
    {
        ret = acldvppPngDecodeAsync(dvppChannelDesc, inDevBuffer_, inDevBufferSize_,
        decodeOutputDesc_, stream);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("acl dvpp Png Decode Async failed, ret = %d", ret);
            return FAILED;
        }
        INFO_LOG("acl dvpp Png Decode Async success");
    }

    ret = aclrtSynchronizeStream(stream);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("acl rt Synchronize Stream failed, ret=%d", ret);
        return FAILED;
    }

    decodeDataSize_ = acldvppGetPicDescSize(decodeOutputDesc_);
    picDevBuffer = nullptr;

    DestroyResource();
    return SUCCESS; 
}

Result Resize::InitResource(PicDesc& picDesc){
    format_ = static_cast<acldvppPixelFormat>(PIXEL_FORMAT_YUV_SEMIPLANAR_420);
    resizeConfig_ = acldvppCreateResizeConfig();
    if (resizeConfig_ == nullptr) {
        ERROR_LOG("Dvpp resize init failed for create config failed");
        return FAILED;
    }

    return SUCCESS; 
}

void Resize::DestroyResource(){
    if (resizeConfig_ != nullptr) {
        (void)acldvppDestroyResizeConfig(resizeConfig_);
        resizeConfig_ = nullptr;
    }

    if (resizeInputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(resizeInputDesc_);
        resizeInputDesc_ = nullptr;
    }

    if (resizeOutputDesc_ != nullptr) {
        (void)acldvppDestroyPicDesc(resizeOutputDesc_);
        resizeOutputDesc_ = nullptr;
    }

    if(outDevBuffer_ != nullptr){
        (void)acldvppFree(outDevBuffer_);
        outDevBuffer_ = nullptr;
    }
}

Result Resize::Process(Image& image,
                       void* inputPtr,aclrtRunMode runMode,acldvppChannelDesc* dvppChannelDesc,aclrtStream stream)
{
    if (SUCCESS != InitResource(image.picdesc)) {
        ERROR_LOG("Dvpp resize failed for init error");
        return FAILED;
    }
    outDevBuffer_ = inputPtr;
    uint32_t alignWidth = alignment(image.picdesc.width,128);
    uint32_t alignHeight = alignment(image.picdesc.height,16);
    if (alignWidth == 0 || alignHeight == 0) {
        ERROR_LOG("Init Resize Input Desc Alignment failed. image w %d, h %d, align w%d, h%d",
         image.picdesc.width, image.picdesc.height, alignWidth, alignHeight);
        return FAILED;
    }

    uint32_t inputBufferSize = alignWidth*alignHeight*3;
    if(image.picdesc.inputImgFormat == JPG){
        inputBufferSize /= 2;
    }

    resizeInputDesc_ = acldvppCreatePicDesc();
    if (resizeInputDesc_ == nullptr) {
        ERROR_LOG("acldvppCreatePicDesc resizeInputDesc_ failed");
        return FAILED;
    }
    acldvppSetPicDescData(resizeInputDesc_, outDevBuffer_);
    acldvppSetPicDescFormat(resizeInputDesc_, format_);
    acldvppSetPicDescWidth(resizeInputDesc_, image.picdesc.width);
    acldvppSetPicDescHeight(resizeInputDesc_, image.picdesc.height);
    acldvppSetPicDescWidthStride(resizeInputDesc_, alignWidth);
    acldvppSetPicDescHeightStride(resizeInputDesc_, alignHeight);
    acldvppSetPicDescSize(resizeInputDesc_, inputBufferSize);

    int resizeOutWidth = alignment(image.modelWidth,2);
    int resizeOutHeight = alignment(image.modelHeight,2);
    int resizeOutWidthStride = alignment(resizeOutWidth,16);
    int resizeOutHeightStride = alignment(resizeOutHeight,2);
    if (resizeOutWidthStride == 0 || resizeOutHeightStride == 0) {
        ERROR_LOG("Init Resize Out put Desc Alignment failed");
        return FAILED;
    }
    resizeOutDevBufferSize_ = resizeOutWidthStride*resizeOutHeightStride*3/2;

    aclError aclRet = acldvppMalloc(&resizeOutDevBuffer_, resizeOutDevBufferSize_);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("acldvppMalloc vpcOutBufferDev_ failed, aclRet = %d", aclRet);
        return FAILED;
    }

    resizeOutputDesc_ = acldvppCreatePicDesc();
    if (resizeOutputDesc_ == nullptr) {
        ERROR_LOG("acldvppCreatePicDesc resizeOutputDesc_ failed");
        return FAILED;
    }

    acldvppSetPicDescData(resizeOutputDesc_, resizeOutDevBuffer_);
    acldvppSetPicDescFormat(resizeOutputDesc_, format_);
    acldvppSetPicDescWidth(resizeOutputDesc_, resizeOutWidth);
    acldvppSetPicDescHeight(resizeOutputDesc_, resizeOutHeight);
    acldvppSetPicDescWidthStride(resizeOutputDesc_, resizeOutWidthStride);
    acldvppSetPicDescHeightStride(resizeOutputDesc_, resizeOutHeightStride);
    acldvppSetPicDescSize(resizeOutputDesc_, resizeOutDevBufferSize_);
    
    // resize pic
    aclRet = acldvppVpcResizeAsync(dvppChannelDesc, resizeInputDesc_,
        resizeOutputDesc_, resizeConfig_, stream);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("acldvppVpcResizeAsync failed, aclRet = %d", aclRet);
        return FAILED;
    }
    INFO_LOG("acldvppVpcResizeAsync success");

    aclRet = aclrtSynchronizeStream(stream);
    if (aclRet != ACL_SUCCESS) {
        ERROR_LOG("resize aclrtSynchronizeStream failed, aclRet = %d", aclRet);
        return FAILED;
    }
    inputPtr = nullptr;
    DestroyResource();
    return SUCCESS;
}

uint32_t SaveOutputImg(const char *fileName, const void *devPtr, uint32_t dataSize,aclrtRunMode runMode)
{
    FILE* outFileFp = fopen(fileName, "wb+");
    if (runMode == ACL_HOST) {
        void* hostPtr = nullptr;
        aclrtMallocHost(&hostPtr, dataSize);
        aclrtMemcpy(hostPtr, dataSize, devPtr, dataSize, ACL_MEMCPY_DEVICE_TO_HOST);
        fwrite(hostPtr, sizeof(char), dataSize, outFileFp);
        (void)aclrtFreeHost(hostPtr);
    } else {
        fwrite(devPtr, sizeof(char), dataSize, outFileFp);
    }
    fflush(outFileFp);
    fclose(outFileFp);
    return SUCCESS;
}
