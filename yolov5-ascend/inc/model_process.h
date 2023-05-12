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


#pragma once
#include <iostream>
#include "utils.h"

/**
* ModelProcess
*/
class ModelProcess {
public:
    /**
    * @brief Constructor
    */
    ModelProcess();

    /**
    * @brief Destructor
    */
    ~ModelProcess();

    /**
    * @brief load model from file with mem
    * @param [in] modelPath: model path
    * @return result
    */
    Result LoadModelFromFileWithMem(const std::string modelPath,uint32_t & gLoadModelId_);

    /**
    * @brief release all acl resource
    */
    void DestroyResource();

    /**
    * @brief unload model
    */
    void Unload();

    /**
    * @brief create model desc
    * @return result
    */
    Result CreateDesc(uint32_t loadId);

    /**
    * @brief destroy desc
    */
    void DestroyDesc();

    /**
    * @brief create model input
    * @param [in] inputDataBuffer: input buffer
    * @param [in] bufferSize: input buffer size
    * @return result
    */
    Result CreateInput(void *input1, size_t input1size,
    void* input2, size_t input2size);

    /**
    * @brief destroy input resource
    */
    void DestroyInput();

    /**
    * @brief create output buffer
    * @return result
    */
    Result CreateOutput();

    /**
    * @brief destroy output resource
    */
    void DestroyOutput();

    /**
    * @brief model execute
    * @return result
    */
    Result Execute(uint32_t exModelId);

    /**
    * @brief get model output data
    * @return output dataset
    */
    aclmdlDataset *GetModelOutputData();

    Result InitResource(int32_t& deviceId,aclrtContext& context,aclrtStream& stream);
    void destroyAcl(int32_t& deviceId,aclrtContext& context,aclrtStream& stream);
private:
    bool g_loadFlag_;  // model load flag
    uint32_t g_modelId_;
    void *g_modelMemPtr_;
    size_t g_modelMemSize_;
    void *g_modelWeightPtr_;
    size_t g_modelWeightSize_;
    aclmdlDesc *g_modelDesc_;
    aclmdlDataset *g_input_;
    aclmdlDataset *g_output_;

    bool g_isReleased_;
    
    bool isDevice_;
    int32_t deviceId_ = 0;
    aclrtContext context_;
    aclrtStream stream_;
    aclrtRunMode runMode_;
};
