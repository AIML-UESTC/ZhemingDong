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
    SUCCESS = 0,
    FAILED = 1
};

typedef struct PicDesc {
    std::string picName;
    uint32_t width;
    uint32_t height;
    uint32_t decodeSize;
    uint32_t inputImgFormat;
    acldvppPixelFormat outputImgFormat;
} PicDesc;

struct Image{
    void* data;
    size_t dataSize;
    PicDesc picdesc;
    uint32_t modelWidth;
    uint32_t modelHeight;
    std::string imgFormat;
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
