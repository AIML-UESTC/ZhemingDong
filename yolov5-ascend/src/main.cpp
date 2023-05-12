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
#include "udpTransmit.h"
#include "utils.h"
using namespace std;
#define APP_VER_MAJOR   1
#define APP_VER_MINOR   0

struct timeval sysStart, sysNow;	  /*时间记录相关定义,程序运行起始时间、结束时间*/

void usrBoardInfoShow(void)
{
	INFO_LOG("apprun version          :%d.%d",APP_VER_MAJOR&0xF, APP_VER_MINOR);
	INFO_LOG("apprun build time       :%s-%s",__DATE__,__TIME__);
}

namespace {
const std::string modelPath = "/home/HwHiAiUser/jjk/models/";
const std::string cplusPath = "/home/HwHiAiUser/jjk/cplus_code/";
}

int main(int argc, char *argv[]) 
{
    usrBoardInfoShow();
    gettimeofday(&sysStart, NULL);/* 获取系统运行起始时间 */

    shared_ptr<trans> udpObj(new trans(modelPath,cplusPath));
    
    udpObj->ThreadCreate();
    udpObj->waitThreadClose();
    return SUCCESS;
}
