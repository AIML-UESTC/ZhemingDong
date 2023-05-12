#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include "model_process.h"

using namespace std;

typedef struct _mod_index_info_
{
	unsigned int modIndex;/* 模型编号 */
	unsigned int modNum;/* 模型序号 */
	unsigned int modId;
    unsigned int modWidth;/*推理模型使用的图片宽度*/
	unsigned int modHeight;/*推理模型使用的图片高度*/
	float modthresholdVal;/*推理模型置信度阈值*/
}MOD_INDEX_INFO;

/* 结果信息结构体 */
typedef struct _object_info_
{
    unsigned short objLabel; 	/* 目标标签 */
    unsigned short obj_x;    	  /* 目标坐标X */
    unsigned short obj_y;    	 /* 目标坐标Y */
    unsigned short width;    	  /* 目标宽度 */
    unsigned short height;    	  /* 目标高度 */
    unsigned short confidence;    /* 目标置信度 */
    unsigned short res[4];        /* 保留位 */
}OBJECT_INFO;

typedef struct _detec_result_
{
	uint32_t picIndex;
    uint16_t dTarNum;
	OBJECT_INFO objInfo[40];
}DETECT_RESULT;

class ObjectDetect {
public:
    ObjectDetect(const std::string& modelPath);
    Result Init(uint32_t loadModelCnt);/* 初始化 */
    Result Inference(aclmdlDataset*& inferenceOutput, Image& image, uint32_t detectModelId);/* 推理 */
    Result Postprocess(Image& image, aclmdlDataset*& modelOutput,DETECT_RESULT &gOutResult);/* 后处理:结果读取 */

    void storeModelInfo(uint32_t infoIndex,MOD_INDEX_INFO storeInfo);/* 存储模型信息 */
    void storeModelNum(uint32_t totalNum);/* 获取存储模型总数 */
    uint32_t checkModelId(uint32_t inferModelIndex);/* 检查该模型的ID */
    Result inferBeforeCreatDesc(uint32_t gInferId, uint32_t inferWidth ,uint32_t inferHeight);/* 推理前输入输出流设备创建 */
    void inferEndDestroyDesc();
    string get_ModelPath(void) {return modelPath_;}/* 获取模型读取地址 */
    uint16_t get_tarCnt(void){return tarCnt_;}/* 获取识别到的模型个数 */
private:
    void* GetInferenceOutputItem(uint32_t& itemDataSize,
                                 aclmdlDataset* inferenceOutput,
                                 uint32_t idx);
    void DrawBoundBoxToImage(std::vector<BoundingBox>& detectionResults,
                             const std::string& origImagePath);

    Result CreateImageInfoBuffer(uint32_t inferImageWidth_, uint32_t inferImageHeight_);
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
    uint16_t tarCnt_;
    aclrtRunMode runMode_;
private:
    unsigned int gModTotalNum_;/* 文件列表读取的模型总数 */
    MOD_INDEX_INFO gModeInfo_[20];/* 解析后的模型信息存储数组 */
    Result loadMultiModel(uint32_t loadModelCnt);

};