#pragma once
#include <string>
#include <memory>
#include <iostream>
#include "acl/acl.h"
#include "acl/ops/acl_dvpp.h"


#define INFO_LOG(fmt, args...) fprintf(stdout, "[INFO]  " fmt "\n", ##args)
#define WARN_LOG(fmt, args...) fprintf(stdout, "[WARN]  " fmt "\n", ##args)
#define ERROR_LOG(fmt, args...) fprintf(stdout, "[ERROR] " fmt "\n", ##args)



enum Result {
    SUCCESS           = (0),       /* 成功 */
    FAILED            = (-1),      /* 失败 */
    CRC_ERR           = (0x1 << 0),/* crc计算错误 */
    FILE_OPEN_ERR     = (0x1 << 1),/* 文件创建失败 */
    FILE_WRITE_ERR    = (0x1 << 2),/* 文件写入失败 */
    FILE_ORD_ERR      = (0x1 << 3),/* 文件编号不存在 */
    FILE_VER_ERR      = (0x1 << 4),/* 版本信息文件更新失败 */
    FILE_TYPE_ERR     = (0x1 << 5),/*文件类型错误*/
    FILE_LANGUAGE_ERR = (0x1 << 6),/* 语言类型错误 */
    NO_PIC_INDEX      = (0x1 << 7),/* 无此图片 */
    NO_MODEL_INDEX    = (0x1 << 8),/* 无此模型 */
    NO_INFO_INDEX     = (0x1 << 9),/* 无此信息 */
    NO_DETECT_RES     = (0x1 << 10)/* 无此推理结果 */
};

struct Image{
    uint32_t deImgIndex;
    char* data;
    size_t dataSize;
    uint32_t imgWidth;
    uint32_t imgHeight;
    uint32_t detecId;
};

struct BoundingBox {
    uint32_t lt_x;
    uint32_t lt_y;
    uint32_t rb_x;
    uint32_t rb_y;
    uint32_t attribute;
    float score;
    std::string text;
};
