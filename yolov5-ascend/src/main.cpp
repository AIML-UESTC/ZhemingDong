/*
* Copyright (C) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
*/
/**
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

* File main.cpp
* Description: main function
*/

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include "inference.h"

using namespace std;


namespace {
uint32_t modelWidth = 640;
uint32_t modelHeight = 640;
const std::string modelPath = "../model/yolov5s_aipp_u8.om";
}

int readFileList(char *basePath,char imgnames[][100])
{
    DIR *dir;
    struct dirent *ptr;
    char base[50];
    char imgname[100];
    int namelen = 0;
    int x=0;
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
        	continue;
        else if(ptr->d_type == 8)
        {
        	namelen = strlen(ptr->d_name);
        	if(ptr->d_name[namelen-4] == '.' && ( ptr->d_name[namelen-3] == 'j' || ptr->d_name[namelen-3] == 'J' ) && ( ptr->d_name[namelen-2] == 'p' || ptr->d_name[namelen-2] == 'P' ) && ( ptr->d_name[namelen-1] == 'g' || ptr->d_name[namelen-1] == 'G' ))    ///jpgfile
        		{
        		ptr->d_name[namelen] = '\0';
        		strcpy(imgnames[x],ptr->d_name);
        		x++;
        		}
        }
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base,'\0',sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            readFileList(base,imgnames);
        }
    }
    closedir(dir);
    return x;
}

int main(int argc, char *argv[]) {

    if((argc < 3) || (argv[1] == nullptr)){
        ERROR_LOG("Please input: ./main <image_path> <saveImg>(0/1)");
        return FAILED;
    }
    char image_names[5000][100];
    string image_dir = string(argv[1]);
    int fnum = readFileList(argv[1],image_names);
    string image_path;
    // string imgFormat = image_path.substr(image_path.size()-3,image_path.size());
    // if ((imgFormat !="jpg") && (imgFormat !="png")){
    //     ERROR_LOG("Only support JPG or PNG img!!");
    //     return FAILED;
    // }
    string imgFormat = string("jpg");
    ObjectDetect detect(modelPath,modelWidth,modelHeight);
    
    aclError ret = detect.Init();
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("Init resource failed %d", ret); 
        return FAILED;               
    }
    struct  timeval tstart,tend;
    double timeuse;
    gettimeofday(&tstart,NULL);
    aclmdlDataset* inferenceOutput;
    for(int i =0;i<fnum;i++)
    {
        // printf("run %d times\n",i);
        image_path = image_dir+image_names[i];
        Image image;
        image.imgFormat = imgFormat;
        image.modelWidth = modelWidth;
        image.modelHeight = modelHeight;
        ret = detect.Preprocess(image_path, image);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("Preprocess failed %d", ret); 
            return FAILED;               
        }
        ret = detect.Inference(inferenceOutput, image);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("Inference model inference output data failed");
            return FAILED;
        }

        ret = detect.Postprocess(image, inferenceOutput, image_path);
        if (ret != ACL_SUCCESS) {
            ERROR_LOG("Process model inference output data failed");
            return FAILED;
        }
    }

    gettimeofday(&tend,NULL);
    timeuse = 1000000*(tend.tv_sec - tstart.tv_sec) + \
				(tend.tv_usec - tstart.tv_usec);
    INFO_LOG("whole time: %f ms",timeuse/1000);
    detect.DestroyResource();
    return SUCCESS;
}
