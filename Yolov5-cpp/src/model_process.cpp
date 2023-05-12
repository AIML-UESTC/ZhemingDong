/*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <iostream>
#include "model_process.h"
// #include "utils.h"
using namespace std;

ModelProcess::ModelProcess()
    : g_loadFlag_(false), g_modelId_(0), g_modelMemPtr_(nullptr), g_modelMemSize_(0),
      g_modelWeightPtr_(nullptr), g_modelWeightSize_(0), g_modelDesc_(nullptr),
      g_input_(nullptr), g_output_(nullptr), g_isReleased_(false)
{
}

ModelProcess::~ModelProcess()
{
    destroyAcl(deviceId_,context_,stream_);
    DestroyResource();
}

void ModelProcess::DestroyResource()
{
    if (g_isReleased_)
        return;
    Unload();
    DestroyDesc();
    DestroyOutput();
    g_isReleased_ = true;
}

void ModelProcess::destroyAcl(int32_t& deviceId,aclrtContext& context,aclrtStream& stream)
{
    aclError ret;
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


Result ModelProcess::InitResource(int32_t& deviceId,aclrtContext& context,aclrtStream& stream){
    // acl init 
    const char *aclConfigPath = "./acl.json";
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
    INFO_LOG("get run mode success. isDevice_ = %d",isDevice_);

    deviceId_ = deviceId;
    context_ = context;
    stream_ = stream;

    return SUCCESS;
}

Result ModelProcess::LoadModelFromFileWithMem(const string modelPath,uint32_t &gLoadModelId_)
{

    aclError ret = aclmdlQuerySize(modelPath.c_str(), &g_modelMemSize_, &g_modelWeightSize_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("query model failed, model file is %s,ret = %d", modelPath.c_str(),ret);
        return FAILED;
    }

    ret = aclrtMalloc(&g_modelMemPtr_, g_modelMemSize_, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("malloc buffer for mem failed, require size is %zu", g_modelMemSize_);
        return FAILED;
    }

    ret = aclrtMalloc(&g_modelWeightPtr_, g_modelWeightSize_, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("malloc buffer for weight failed, require size is %zu", g_modelWeightSize_);
        return FAILED;
    }

    ret = aclmdlLoadFromFileWithMem(modelPath.c_str(), &gLoadModelId_, g_modelMemPtr_,
    g_modelMemSize_, g_modelWeightPtr_, g_modelWeightSize_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("load model from file failed, model file is %s", modelPath.c_str());
        return FAILED;
    }

    g_loadFlag_ = true;
    INFO_LOG("load model %s success,model id is %d", modelPath.c_str(),gLoadModelId_);
    return SUCCESS;
}

Result ModelProcess::CreateDesc(uint32_t loadId)
{
    g_modelDesc_ = aclmdlCreateDesc();
    if (g_modelDesc_ == nullptr) {
        ERROR_LOG("create model description failed");
        return FAILED;
    }

    aclError ret = aclmdlGetDesc(g_modelDesc_, loadId);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("get model description failed");
        return FAILED;
    }

    INFO_LOG("create model %d description success",loadId);
    return SUCCESS;
}

void ModelProcess::DestroyDesc()
{
    if (g_modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(g_modelDesc_);
        g_modelDesc_ = nullptr;
    }
}



Result ModelProcess::CreateInput(void *input1, size_t input1size,
                                 void* input2, size_t input2size)
{   
    g_input_ = aclmdlCreateDataset();
    if (g_input_ == nullptr) {
        ERROR_LOG("can't create dataset, create input failed");
        return FAILED;
    }

    aclDataBuffer* inputData = aclCreateDataBuffer(input1, input1size);
    if (inputData == nullptr) {
        ERROR_LOG("can't create data buffer, create input failed");
        return FAILED;
    }

    aclmdlAddDatasetBuffer(g_input_, inputData);
    if (inputData == nullptr) {
        ERROR_LOG("can't add data buffer, create input failed");
        aclDestroyDataBuffer(inputData);
        inputData = nullptr;
        return FAILED;
    }

    aclDataBuffer* inputData2 = aclCreateDataBuffer(input2, input2size);
    if (inputData2 == nullptr) {
        ERROR_LOG("can't create data buffer, create input failed");
        return FAILED;
    }

    aclmdlAddDatasetBuffer(g_input_, inputData2);
    if (inputData2 == nullptr) {
        ERROR_LOG("can't add data buffer, create input failed");
        aclDestroyDataBuffer(inputData2);
        inputData = nullptr;
        return FAILED;
    }

    return SUCCESS;
}

void ModelProcess::DestroyInput()
{
    if (g_input_ == nullptr) {
        return;
    }

    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(g_input_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(g_input_, i);
        aclDestroyDataBuffer(dataBuffer);
    }
    aclmdlDestroyDataset(g_input_);
    g_input_ = nullptr;
}

Result ModelProcess::CreateOutput()
{
    if (g_modelDesc_ == nullptr) {
        ERROR_LOG("no model description, create ouput failed");
        return FAILED;
    }

    g_output_ = aclmdlCreateDataset();
    if (g_output_ == nullptr) {
        ERROR_LOG("can't create dataset, create output failed");
        return FAILED;
    }

    size_t outputSize = aclmdlGetNumOutputs(g_modelDesc_);
    INFO_LOG("------outputSize : %ld",outputSize);
    for (size_t i = 0; i < outputSize; ++i) {
        size_t buffer_size = aclmdlGetOutputSizeByIndex(g_modelDesc_, i);

        void *outputBuffer = nullptr;
        aclError ret = aclrtMalloc(&outputBuffer, buffer_size, ACL_MEM_MALLOC_NORMAL_ONLY);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("can't malloc buffer, size is %zu, create output failed", buffer_size);
            return FAILED;
        }

        aclDataBuffer* outputData = aclCreateDataBuffer(outputBuffer, buffer_size);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("can't create data buffer, create output failed");
            aclrtFree(outputBuffer);
            return FAILED;
        }

        ret = aclmdlAddDatasetBuffer(g_output_, outputData);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("can't add data buffer, create output failed");
            aclrtFree(outputBuffer);
            aclDestroyDataBuffer(outputData);
            return FAILED;
        }
    }

    INFO_LOG("create model output success");
    return SUCCESS;
}

void ModelProcess::DestroyOutput()
{
    if (g_output_ == nullptr) {
        return;
    }

    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(g_output_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(g_output_, i);
        void* data = aclGetDataBufferAddr(dataBuffer);
        (void)aclrtFree(data);
        (void)aclDestroyDataBuffer(dataBuffer);
    }

    (void)aclmdlDestroyDataset(g_output_);
    g_output_ = nullptr;
}

Result ModelProcess::Execute(uint32_t exModelId)
{
    aclError ret = aclmdlExecute(exModelId, g_input_, g_output_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("execute model failed, modelId is %u, error code is %d", exModelId,ret);
        return FAILED;
    }

    INFO_LOG("model execute success");
    return SUCCESS;
}

void ModelProcess::Unload()
{
    if (!g_loadFlag_) {
        WARN_LOG("no model had been loaded, unload failed");
        return;
    }

    aclError ret = aclmdlUnload(g_modelId_);
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("unload model failed, modelId is %u", g_modelId_);
    }

    if (g_modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(g_modelDesc_);
        g_modelDesc_ = nullptr;
    }

    if (g_modelMemPtr_ != nullptr) {
        aclrtFree(g_modelMemPtr_);
        g_modelMemPtr_ = nullptr;
        g_modelMemSize_ = 0;
    }

    if (g_modelWeightPtr_ != nullptr) {
        aclrtFree(g_modelWeightPtr_);
        g_modelWeightPtr_ = nullptr;
        g_modelWeightSize_ = 0;
    }

    g_loadFlag_ = false;
    INFO_LOG("unload model success, modelId is %u", g_modelId_);
}

aclmdlDataset *ModelProcess::GetModelOutputData()
{
    return g_output_;
}