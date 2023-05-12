//#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include "udpTransmit.h"

// #define testDebug
#define gTestTime
using namespace std;
/************************************宏定义************************************/
/************************************全局变量************************************/
struct timespec tpoint;/*时间记录相关定义,时间值、起始时间、结束时间*/
/************************************参数声明************************************/
extern struct timeval sysStart, sysNow;	  /*时间记录相关定义,程序运行起始时间、结束时间*/
// static pthread_t udpThreadPid0;
// static pthread_t inferThreadPid0;

#ifdef testDebug
static int fdTest0 = 0;	
#endif

#ifdef gTestTime
	struct timespec tRecordStart,tRecordEnd;
	struct timespec tRecordStart1,tRecordEnd1;
	unsigned long long testInferTimeMs,testTimeCpyUs;
	unsigned long long testPthTimeMs;
#endif
/***************************crc,print等功能性函数***************************/
const unsigned int CRCtbl[256] = {
	0x00000,0x08005,0x0800F,0x0000A,0x0801B,0x0001E,0x00014,0x08011,0x08033,0x00036,
	0x0003C,0x08039,0x00028,0x0802D,0x08027,0x00022,0x08063,0x00066,0x0006C,0x08069,
	0x00078,0x0807D,0x08077,0x00072,0x00050,0x08055,0x0805F,0x0005A,0x0804B,0x0004E,
	0x00044,0x08041,0x080C3,0x000C6,0x000CC,0x080C9,0x000D8,0x080DD,0x080D7,0x000D2,
	0x000F0,0x080F5,0x080FF,0x000FA,0x080EB,0x000EE,0x000E4,0x080E1,0x000A0,0x080A5,
	0x080AF,0x000AA,0x080BB,0x000BE,0x000B4,0x080B1,0x08093,0x00096,0x0009C,0x08099,
	0x00088,0x0808D,0x08087,0x00082,0x08183,0x00186,0x0018C,0x08189,0x00198,0x0819D,
	0x08197,0x00192,0x001B0,0x081B5,0x081BF,0x001BA,0x081AB,0x001AE,0x001A4,0x081A1,
	0x001E0,0x081E5,0x081EF,0x001EA,0x081FB,0x001FE,0x001F4,0x081F1,0x081D3,0x001D6,
	0x001DC,0x081D9,0x001C8,0x081CD,0x081C7,0x001C2,0x00140,0x08145,0x0814F,0x0014A,
	0x0815B,0x0015E,0x00154,0x08151,0x08173,0x00176,0x0017C,0x08179,0x00168,0x0816D,
	0x08167,0x00162,0x08123,0x00126,0x0012C,0x08129,0x00138,0x0813D,0x08137,0x00132,
	0x00110,0x08115,0x0811F,0x0011A,0x0810B,0x0010E,0x00104,0x08101,0x08303,0x00306,
	0x0030C,0x08309,0x00318,0x0831D,0x08317,0x00312,0x00330,0x08335,0x0833F,0x0033A,
	0x0832B,0x0032E,0x00324,0x08321,0x00360,0x08365,0x0836F,0x0036A,0x0837B,0x0037E,
	0x00374,0x08371,0x08353,0x00356,0x0035C,0x08359,0x00348,0x0834D,0x08347,0x00342,
	0x003C0,0x083C5,0x083CF,0x003CA,0x083DB,0x003DE,0x003D4,0x083D1,0x083F3,0x003F6,
	0x003FC,0x083F9,0x003E8,0x083ED,0x083E7,0x003E2,0x083A3,0x003A6,0x003AC,0x083A9,
	0x003B8,0x083BD,0x083B7,0x003B2,0x00390,0x08395,0x0839F,0x0039A,0x0838B,0x0038E,
	0x00384,0x08381,0x00280,0x08285,0x0828F,0x0028A,0x0829B,0x0029E,0x00294,0x08291,
	0x082B3,0x002B6,0x002BC,0x082B9,0x002A8,0x082AD,0x082A7,0x002A2,0x082E3,0x002E6,
	0x002EC,0x082E9,0x002F8,0x082FD,0x082F7,0x002F2,0x002D0,0x082D5,0x082DF,0x002DA,
	0x082CB,0x002CE,0x002C4,0x082C1,0x08243,0x00246,0x0024C,0x08249,0x00258,0x0825D,
	0x08257,0x00252,0x00270,0x08275,0x0827F,0x0027A,0x0826B,0x0026E,0x00264,0x08261,
	0x00220,0x08225,0x0822F,0x0022A,0x0823B,0x0023E,0x00234,0x08231,0x08213,0x00216,
	0x0021C,0x08219,0x00208,0x0820D,0x08207,0x00202};
//查表法计算CRC校验和
uint16_t crc16(uint8_t *pData, int len)
{
	unsigned short crc16;
	int i;
	crc16 = 0;
	//计算CRC校验和
	for (i = 0; i < len; i++)
	{
		crc16 = (crc16 << 8) ^ CRCtbl[(crc16 >> 8) ^ *pData++];
	}
	return crc16;
}

/*******************************************************
 * 函 数 名：getSumCheck(UINT8 *cData,UINT32 checkLen)
 *
 * 函数说明:	 累加和计算函数
 * 参数说明:
 * 输入参数: 	cData-待校验数据，checkLen-校验长度(字节)
 * 输出参数:
 * 返 回 值: 累加和值
 *
 * 修改记录：
 * --------------------
 *	  2021/06/06  jianjikai
 *
 ********************************************************/
int getSumCheck(char *cData, int checkLen)
{
	int sumVal = 0;
	int i;

	for (i = 0; i < checkLen; i++)
	{
		sumVal += cData[i];
	}

	return sumVal;
}
/*
 *数据内容打印
 */
int printData(char *cData, int checkLen)
{
	int sumVal = 0;
	int i;

	for (i = 0; i < checkLen; i++)
	{
		printf("0x%x ", cData[i]);
	}

	return sumVal;
}

/***************************udp-图像/模型数据接收部分函数***************************/
/*******************************************************
 * 函 数 名：Result checkImgExist(uint32_t checkImgIndex)
 *
 * 函数说明:	 检查该编号图像是否存在
 * 参数说明:
 * 输入参数: 	checkImgIndex-图像编号
 * 输出参数:
 * 返 回 值: 结果
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: checkImgExist(uint32_t checkImgIndex)
{
	uint8_t i;
	deGetImgStPtr=11;

	for(i=0;i<stMaxSize;i++)
	{
		if (picStore[i].picIndex == checkImgIndex)/* 如果找到有这张图，则返回成功 */
		{
			deGetImgStPtr = i;
			return SUCCESS;
		}
	}
	return NO_PIC_INDEX;
}
/*******************************************************
 * 函 数 名：Result checkOmExist(uint32_t checkImgIndex)
 *
 * 函数说明:	 检查该编号模型是否存在
 * 参数说明:
 * 输入参数: 	checkOmIndex-om编号
 * 输出参数:
 * 返 回 值: 结果
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: checkOmExist(uint32_t checkOmIndex)
{
	uint8_t i;
	for(i=0;i<20;i++)
	{
		if (gModeInfo[i].modIndex == checkOmIndex)/* 如果找到有这个带info的om，则返回成功 */
		{
			return SUCCESS;
		}
	}

	for(i=0;i<20;i++)/* 如果没找到,则找一下有没有om存在info不存在的情况 */
	{
		if (gModeNoInfo[i] == checkOmIndex)/* 如果找到有这个带info的om，则返回成功 */
		{
			return NO_INFO_INDEX;
		}
	}
	return NO_MODEL_INDEX;
}
/*******************************************************
 * 函 数 名：Result checkDetectExist(uint32_t checkResIndex)
 *
 * 函数说明:	 检查该编号图片结果是否存在
 * 参数说明:
 * 输入参数: 	checkResIndex-代查询的图片编号
 * 输出参数:
 * 返 回 值: 结果
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: checkDetectExist(uint32_t checkResIndex)
{
	uint8_t i;

	for(i=0;i<stMaxSize;i++)
	{
		if (gDetectRes[i].picIndex == checkResIndex)/* 如果找到有这张图，则返回成功 */
		{
			deGetResPtr = i;
			return SUCCESS;
		}
	}
	return NO_DETECT_RES;
}
/*******************************************************
 * 函 数 名：imageRec
 *
 * 函数说明: 图像接收存储函数
 * 参数说明: 图像接收完成后会转换成bmp格式并在/dev/shm/路径下写成文件
 * 输入参数: imageIndex-图像编号,recImage-图像数据
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/08/07  jianjikai
 *
 *******************************************************updateFileAddr*/
int trans :: imageRec(unsigned int imageIndex, IMAGE_FRAME *recImage)
{
	//总包号不对，或者包编号大于包总数，则图像数据有问题
	// if((recImage == NULL) || (totalPackNum != recImage->packTotalNum))
	// {
	// 	printf("recImage->packTotalNum:%d , totalPackNum:%d\r\n",recImage->packTotalNum,totalPackNum);
	// 	return -1;
	// }
	if (recImage == NULL)
	{
		ERROR_LOG("recImage->packTotalNum:%d , totalPackNum:%d", recImage->packTotalNum, totalPackNum);
		return -1;
	}

	if ((recImage->packIndex == 0) || (recImage->packIndex > totalPackNum))
	{
		ERROR_LOG("recImage->packIndex :%d , recImage->packTotalNum:%d", recImage->packIndex, recImage->packTotalNum);
		return -2;
	}
	//如果准备帧的帧号和图像帧的帧序号不一样，说明收到了一帧不对的图像数据
	if (recImage->imageIndex != imageIndex)
	{
		ERROR_LOG("recImage->imageIndex :%d , imageIndex:%d", recImage->imageIndex, imageIndex);
		return -3;
	}
	//判断数据长度是否正确，否则可能导致数组越界
	//判断每包数据包长度是否正确
	if (recImage->packIndex == totalPackNum) /*是最后一包时*/
	{
		if (recImage->packlen != (totalImageLen % image_payload_len))
		{
			ERROR_LOG("dataLen =%d", (recImage->packlen));
			ERROR_LOG("ai Len =%d,dsp Len=%d", ((totalImageLen) % image_payload_len), recImage->packlen);
			return -4;
		}
		// ERROR_LOG("recImage->packIndex :%d , recImage->packTotalNum:%d\r\n",recImage->packIndex ,recImage->packTotalNum);
		if (((image_payload_len * (totalPackNum - 1)) + recImage->packlen) > imgBufferMaxSize) /*总长度大于最大图像长度*/
		{
			ERROR_LOG("Len over max,rec Len =%d", ((image_payload_len * (totalPackNum - 1)) + recImage->packlen));
			return -6;
		}
	}
	else
	{
		if (recImage->packlen != image_payload_len)
		{
			ERROR_LOG("packLen error =%d", recImage->packlen);
			return -5;
		}
	}

	/*将数据写入图像缓存buf*/
	memcpy((char *)&picStore[stPtr].picAddr[(recImage->packIndex - 1) * image_payload_len], &recImage->imageData[0], recImage->packlen);

	if (recImage->packIndex < lostMaxCnt)
	{
		recPackLog[recImage->packIndex - 1] = 1; //记录收到的数据包
		recCnt++;								 /*记录接收到的图像数据包个数*/
	}
	else
	{
		ERROR_LOG("lost over size! recImage->packIndex :%d , recImage->packTotalNum:%d", recImage->packIndex, recImage->packTotalNum);
		return -6;
	}

	if (recCnt == totalPackNum) /*图像接收完成，为该段地址对应存入图像号*/
	{
		picStore[stPtr].picIndex = imageIndex;
		stPtr = (stPtr + 1) % stMaxSize;
	}
	return 0;
}

/*******************************************************
 * 函 数 名：updateFileRec
 *
 * 函数说明: 升级文件接收存储函数
 * 参数说明: 升级文件接收完成后会转换成bmp格式并在/dev/shm/路径下写成文件
 * 输入参数: recLoadFile-模型数据
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/08/07  jianjikai
 *
 ********************************************************/
int trans :: updateFileRec(MODE_DATA *recLoadFile)
{
	UPDATE_INFO updateInfo;
	dataInfoLen = sizeof(UPDATE_INFO);
	modeInfoLen = sizeof(GET_INFO);
	//总包号不对，或者包编号大于包总数，则图像数据有问题
	if ((recLoadFile == NULL) || (totalPackNum != recLoadFile->packTotalNum))
	{
		return -1;
	}
	if ((recLoadFile->packIndex == 0) || (recLoadFile->packIndex > recLoadFile->packTotalNum))
	{
		return -2;
	}
	
	//判断最后一包数据长度是否正确，否则可能导致数组越界
	//判断每包数据包长度是否正确
	if (recLoadFile->packIndex == recLoadFile->packTotalNum)
	{
#if 1
		if (recLoadFile->packlen != ((upDataLen) % update_payload_len)) /*判断最后一包数据长度是否正确*/
		{
			printf("dataLen =%d\n\r", (upDataLen));
			printf("ai Len =%d,dsp Len=%d\n\r", ((upDataLen) % update_payload_len), recLoadFile->packlen);
			return -4;
		}
#endif
		if (((update_payload_len * (recLoadFile->packTotalNum - 1)) + recLoadFile->packlen) > updateFileMaxSize) // 25M判断是否大小溢出
			return -6;
	}
	else
	{
		if (recLoadFile->packlen != update_payload_len)
			return -5;
	}

	if (recLoadFile->packIndex == 1)/* 如果是首包，则需要解析文件信息 */
	{
		memcpy((char *)&updateInfo, &recLoadFile->modeData[0], dataInfoLen);

		upDataType = updateInfo.updateType;
		if (upDataType == 0xAABBCCDD) //如果为程序，则读取语言类型
		{
			upSoftLanguage = updateInfo.softLanguage;/* 获取程序语言类型 */
		}
		else if(upDataType == 0x11223344)/* 如果是模型，则需要存储模型信息 */
		{
			upSoftLanguage = 0;
			upModelOrd = updateInfo.modelInfo.modeOrdinal;    /* 获取识别模型的序号 */
			/*将数据写入模型信息缓存buf*/
			memcpy(modelInfoAddr,(char *)&updateInfo.modelInfo, sizeof(GET_INFO));
		}
		else
		{
			return -7;/*既不是程序也不是模型*/
		}
		upDataRlLen = updateInfo.dataLen;			   /*获取升级文件实际长度*/	
		upSoftVer = updateInfo.softVer;			           /* 获取程序版本号 */
	}
	/*将整个数据文件写入缓存一个buf，以便Crc*/
	memcpy((char *)&updateFileAddr[(recLoadFile->packIndex - 1)*update_payload_len],
				&recLoadFile->modeData[0], recLoadFile->packlen);

	if(recLoadFile->packIndex ==(recUpFilePackCnt + 1))
	{	
		recUpFilePackCnt++; /*记录接收到的模型数据包个数*/
	}
	else if(recLoadFile->packIndex == recUpFilePackCnt)
	{
		;
	}else
	{
		return -8;//未按顺序传输
	}

	return 0;
}
/*******************************************************
 * 函 数 名：getNpuTemp
 *
 * 函数说明: 获取NPU温度
 * 参数说明: 
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: getNpuTemp(void)
{
	dsmi_get_device_temperature(deviceId, &gNpuTemper);
}

/*******************************************************
 * 函 数 名：getMemInfo
 *
 * 函数说明: 获取总内存和已使用内存信息
 * 参数说明: 
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: getMemInfo(void)
{
	dsmi_get_memory_info(0, &memInfo);
	
	gMemTotal = memInfo.memory_size;/* 内存总大小 */
	gMemUse = (unsigned int)((float)memInfo.utiliza / 100 * gMemTotal);/* 内存使用大小 */
}

/*******************************************************
 * 函 数 名：getRunTime
 *
 * 函数说明: 获取C程序运行时间
 * 参数说明: 
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: getRunTime(void)
{
	gettimeofday(&sysNow, NULL);
	gRunTimeMs = 1000 * (sysNow.tv_sec - sysStart.tv_sec) + (sysNow.tv_usec - sysStart.tv_usec) / 1000;
}

/*******************************************************
 * 函 数 名：getSysInfo
 *
 * 函数说明: 获取系统信息:NPU温度、内存使用情况、C程序运行时间
 * 参数说明: 
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: getSysInfo(void)
{
	getNpuTemp();
	getMemInfo();
	getRunTime();

	if (inferStart)
	{
		gSoftStat = 0x0;/* py正常启动运行 */
	}
	else
	{
		gSoftStat = 0xffffffff;/* py还未正常启动运行 */
	}
}

/*******************************************************
 * 函 数 名：imgMemCreat
 *
 * 函数说明: 开机时创建所有需要的内存空间并将指针存储到列表中
 * 参数说明: creatNum:图像需要创建的空间个数
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: MemCreat(uint32_t creatNum)
{
	uint32_t i;
	char *imgAddr;

	for (i=0;i<creatNum;i++)
	{
		imgAddr = (char *)malloc(imgBufferMaxSize);
		if (imgAddr == NULL)
		{
			ERROR_LOG("imgBuff malloc err, index = %d , maxIndex = %d",i,(creatNum-1));
			return FAILED;
		}
		else
		{
			picStore[i].picAddr = imgAddr;
			memset(picStore[i].picAddr, 0x80, sizeof(imgBufferMaxSize));
			// INFO_LOG("imgAddr = 0x%x,picStore[%d].picAddr = 0x%x",imgAddr,i,picStore[i].picAddr);
		}
	}

	updateFileAddr = (int8_t *)malloc(updateFileMaxSize);
	if (updateFileAddr == NULL)
	{
		ERROR_LOG("updateFileAddr malloc err");
		return FAILED;
	}

	modelInfoAddr = (int8_t *)malloc(modInfoMaxSize);
	if (modelInfoAddr == NULL)
	{
		ERROR_LOG("modelInfoAddr malloc err");
		return FAILED;
	}

	recPackLog = (uint16_t *)malloc(lostMaxCnt*2);
	if (recPackLog == NULL)
	{
		ERROR_LOG("recPackLog malloc err");
		return FAILED;
	}

	return SUCCESS;
}
/*******************************************************
 * 函 数 void MemDestroy()
 *
 * 函数说明:	申请的内存集中销毁 
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 结果
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
void trans :: MemDestroy()
{
	uint32_t i;
	for (i=0;i<stMaxSize;i++)
	{
		if (picStore[i].picAddr != NULL)
		{
			free(picStore[i].picAddr);
			INFO_LOG("destroy picStore[%d].picAddr",i);
		}
		
	}
	if (updateFileAddr != NULL)
	{
		free(updateFileAddr);
		INFO_LOG("destroy updateFileAddr");
	}

	if (modelInfoAddr != NULL)
	{
		free(modelInfoAddr);
		INFO_LOG("destroy modelInfoAddr");
	}
	
	if (recPackLog != NULL)
	{
		free(recPackLog);
		INFO_LOG("destroy recPackLog");
	}
}
/*******************************************************
 * 函 数 名：getModeInfo
 *
 * 函数说明: 开机时通过文件列表获取info的所有信息
 * 参数说明: 
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
int trans :: getModeInfo(void)
{
	DIR *dp;
	int rtn = 0;
	int cpLen = 0;
	char tmpStr[2];
	char find = '.';/* 引索目标 */
	int modeOrd = 0;
	struct dirent *dir;
	char *strPtr = NULL;
	string readInfoName_;
	string check_modelInfoPath_ = ObjectDetect::get_ModelPath();
	
	dp = opendir(check_modelInfoPath_.c_str());
	if (dp == NULL)
	{
		ERROR_LOG("open dir err!!");
		return -1;
	}

	while (1)
	{
		dir = readdir(dp);/* 读取文件列表 */
		if (dir != NULL)
		{
			if (strstr(dir->d_name, ".om") != NULL)
			{
				memset(tmpStr, 0, 2);
				// printf("%d fileName is %s\r\n",dir->d_reclen,dir->d_name);
				// for (i = 0;i<dir->d_reclen;i++)
				// {
				//     //   printf("i=%d,str = %.1s\r\n",i,&dir->d_name[i]);
				//     printf("i=%d,addr = %x,str = %.1s\r\n",i,(char *)&dir->d_name[i],&dir->d_name[i]);
				// }
				strPtr = strpbrk((char *)&dir->d_name[0], (char *)&find);
				cpLen = (int)(strPtr - (char *)&dir->d_name[0]);
				strncpy(tmpStr, dir->d_name, cpLen);
				modeOrd = atoi(tmpStr);/* 获取文件名中的数字 */

				readInfoName_ = check_modelInfoPath_ + to_string(modeOrd) + ".info";
				fdMode = open(readInfoName_.c_str(), O_RDWR | O_CREAT, 0777);
				if (fdMode < 0)
				{
					ERROR_LOG("%s open error", readInfoName_.c_str());
					gModeNoInfo[gModTotalNum] = modeOrd;/* 记录没有info的Om编号 */
					continue;
				}

				rtn = read(fdMode, (char *)&gModeInfoBuf, sizeof(GET_INFO));
				if (rtn != sizeof(GET_INFO))
				{
					ERROR_LOG("%s Read error!!,readLen = %d,infoStructLen = %ld",readInfoName_.c_str(),rtn,sizeof(GET_INFO));
					gModeNoInfo[gModTotalNum] = modeOrd;/* 记录没有info的Om编号 */
					continue;
				}
				
				gModeInfo[gModTotalNum].modIndex        = gModeInfoBuf.modeIndex;     /* 记录模型编号 */
				gModeInfo[gModTotalNum].modNum          = gModeInfoBuf.modeOrdinal;   /* 记录模型序号 */
				gModeInfo[gModTotalNum].modWidth        = gModeInfoBuf.inferWidth;    /* 记录训练推理模型使用的图片宽度 */
				gModeInfo[gModTotalNum].modHeight       = gModeInfoBuf.inferHeight;   /* 记录训练推理模型使用的图片高度 */
				gModeInfo[gModTotalNum].modthresholdVal = gModeInfoBuf.thresholdValue;/* 记录训练推理模型的置信度 */
				ObjectDetect::storeModelInfo(gModTotalNum,gModeInfo[gModTotalNum]);   /* 把信息存到Object类中 */
				gModTotalNum++;											   /* 找到om文件，模型总数+1 */
				if(gModTotalNum >= 20)/* 最多存储20个模型文件的信息 */
				{
					ERROR_LOG("model and info totalNum is overflow");
					break;
				}
			}
		}
		else
		{
			break;
		}
	}
	ObjectDetect ::storeModelNum(gModTotalNum);/* 最后总数量存入Object类 */
	closedir(dp);
    return 0;
}

/***************************udp-命令处理主函数部分***************************/
/*******************************************************
 * 函 数 名：aiAsk
 *
 * 函数说明: AI回复ask帧
 * 参数说明: len命令长度,cmd指令字,cmdCnt指令计数
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: aiAsk(unsigned int len,unsigned int cmd, unsigned int cmdCnt)
{
	CMD_ACK_FRAME ackFrame;				  /*命令回复结构体*/

	memset(&ackFrame, 0, sizeof(CMD_ACK_FRAME));
	ackFrame.head = 0xAA55AA55;
	ackFrame.len = len;
	ackFrame.cmd =cmd;
	ackFrame.state = 0xAA00;
	ackFrame.cmdCnt = cmdCnt;

	ackFrame.checkSum = getSumCheck((char *)&ackFrame.cmd, 1012);
	sendto(udpSocket0, (char *)&ackFrame, sizeof(CMD_ACK_FRAME), 0,
			(const struct sockaddr *)&r_addr, sizeof(r_addr));
}
/*******************************************************
 * 函 数 名：aiReply
 *
 * 函数说明: AI回复reply帧
 * 参数说明: len命令长度,cmd指令字,cmdCnt指令计数,replyData:数据体内容,structLen结构体长度
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
void trans :: aiReply(unsigned int len,unsigned int cmd, unsigned int cmdCnt,char * replyData,int structLen)
{
	CMD_ACK_FRAME ackFrame;				  /*命令回复结构体*/

	memset(&ackFrame, 0, sizeof(CMD_ACK_FRAME));
	ackFrame.head = 0xAA55AA55;
	ackFrame.len = len;
	ackFrame.cmd = cmd;
	ackFrame.state = 0xBB33;
	ackFrame.cmdCnt = cmdCnt;
	memcpy((char *)&ackFrame.data[0],replyData,structLen);

	ackFrame.checkSum = getSumCheck((char *)&ackFrame.cmd, 1012);
	sendto(udpSocket0, (char *)&ackFrame, sizeof(CMD_ACK_FRAME), 0,
			(const struct sockaddr *)&r_addr, sizeof(r_addr));
}

/*******************************************************
 * 函 数 名：updateFile
 *
 * 函数说明: 文件升级
 * 参数说明: fileType:文件类型,fileOrd:文件序号,非py程序文件并未被使用
 * 输入参数: 
 * 输出参数:
 * 返 回 值:
 *
 * 修改记录：
 * --------------------
 *	  2022/10/18  jianjikai
 *
 ********************************************************/
Result trans :: updateFile(unsigned int fileType)
{
	string upModelName;/* 模型名称 */
	string upInfoName; /* 信息名称 */				  
	string upSoftName; /* 程序名称 */
	string testPath = "/home/HwHiAiUser/jjk/testRec/";
	unsigned int rtn=1;

	if (fileType == 0x11223344) //模型数据
	{
		INFO_LOG("update om");

		upInfoName = ObjectDetect::get_ModelPath() + to_string(upModelOrd) + ".info";
		fdSeekInfo = access(upInfoName.c_str(), F_OK); /*查询该路径下info是否存在*/
		if (fdSeekInfo == 0)/* 如果存在则删除该文件 */
		{
			rtn = remove(upInfoName.c_str());/* 删除旧文件 */
			if (rtn != 0)
			{
				ERROR_LOG("remove %s  error", upInfoName.c_str());
			}
		}
		
		fdInfo = open(upInfoName.c_str(), O_RDWR | O_CREAT, 0777);/* 创建新的info文件 */
		if (fdInfo < 0)
		{
			return FILE_OPEN_ERR;
		}

		rtn = write(fdInfo, modelInfoAddr, sizeof(GET_INFO));
		if (rtn != sizeof(GET_INFO))
		{
			close(fdInfo);
			return FILE_WRITE_ERR;
		}

		upModelName = ObjectDetect::get_ModelPath() + to_string(upModelOrd) + ".om";
		fdSeekOm = access(upModelName.c_str(), F_OK); /*查询该路径下om是否存在*/
		if (fdSeekOm == 0)
		{
			rtn = remove(upModelName.c_str());/* 按照序号删除om */
			if (rtn != 0)
			{
				ERROR_LOG("remove %s  error",upModelName.c_str());
			}
		}
		
		fdMode = open(upModelName.c_str(), O_RDWR | O_CREAT, 0777);
		if (fdMode < 0)
		{
			return FILE_OPEN_ERR;
		}

		rtn = write(fdMode, (char *)&updateFileAddr[dataInfoLen], upDataRlLen);
		if (rtn != upDataRlLen)
		{
			close(fdMode);
			return FILE_WRITE_ERR;
		}
		close(fdInfo);
		chmod(upInfoName.c_str(),0777);
		close(fdMode);
		chmod(upModelName.c_str(),0777);
	}
	else if (fileType == 0xAABBCCDD) //程序数据
	{
		if (upSoftLanguage == 0xCC13CC13)/* C++程序 */
		{
			INFO_LOG("update ba9_AppRun");
			upSoftName = cplusPath_ + "ba9_AppRun";
			fdSeekApp = access(upSoftName.c_str(), F_OK); /*查询该路径下app是否存在*/
			if (fdSeekApp == 0)
			{
				rtn = remove(upSoftName.c_str());/* 删除app */
				if (rtn != 0)
				{
					ERROR_LOG("remove %s  error",upSoftName.c_str());
				}
			}
			
			fdApp = open(upSoftName.c_str(), O_RDWR | O_CREAT, 0777);
			if (fdApp < 0)
			{
				return FILE_OPEN_ERR;
			}

			rtn = write(fdApp, (char *)&updateFileAddr[dataInfoLen], upDataRlLen);
			if (rtn != upDataRlLen)
			{
				close(fdApp);
				return FILE_WRITE_ERR;
			}

			close(fdApp);
			chmod(upSoftName.c_str(),0777);
		}
		else
		{
			ERROR_LOG("check upSoftLanguage err : 0x%x",upSoftLanguage);
			return FILE_LANGUAGE_ERR;
		}
	}
	else
	{
		ERROR_LOG("check type err : 0x%x",fileType);
		return FILE_TYPE_ERR;
	}
	INFO_LOG("update success");
	return SUCCESS;
}
/*******************************************************
 * 函 数 int netThread0()
 *
 * 函数说明:	网络处理线程 
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
int trans :: netThread0()
{
	int rtn, on;
	unsigned int i;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	CMD_SEND_FRAME recPackTemp;
	int fistPack = 1;					  //首次接收到udp数据flag
	unsigned int packNum = 0;			  // udp数据包计数,此计数用于记录cmdCnt
	unsigned int imageIndex = 0;		  //图像帧编号
	unsigned int checkSum = 0;			  //数据校验和
	Result fSeekPic;
	Result fSeekOm;
	Result res;
	string infoName;					  //信息名称
	CMD_ACK_FRAME ackFrame;				  /*命令回复结构体*/
	/*自检*/
	// SYSTEM_CHECK *systemCheck;			  /*自检信息结构体*/
	SYSTEM_CHECK_REPLY replySystemCheck; /*自检信息reply结构体*/
	/*图像*/
	PREPARE_FRAME *preParePack; /*图像上传准备信息结构体*/
	PREPARE_REPLY  preReply;/* 图像上传准备reply结构体 */
	REC_QUERY replyQuery;		/* 图像结果查询reply结构体*/
	/*识别*/
	DSP_DETECTION *detecCmd;	/*启动识别信息结构体*/
	DETECTION_ACK ackDetecCmd;/*  启动识别回复 reply*/
	DETECTION_QUERY deteResRely;/* 识别结果查询reply */
	/* 模型 */
	MODE_INFO *upLoadCmd;  /*模型上传准备信息结构体*/
	MODE_INFO ackModeCmd; /*模型上传准备reply结构体*/

	MODE_DATA *upLoadMode;	  /*模型更新信息结构体*/
	MODE_DATA *replyModeData; /*模型更新回复命令数据内容结构体*/

	MODE_REC_QUERY replyModeRes; /*模型更新结果查询命令reply数据内容结构体*/

	INFO_REC *modelInfoNum; /* 模型信息编号获取 */
	runFlag0 = 1;			//默认运行

	cpu_set_t           mask;

	CPU_ZERO(&mask);
    CPU_SET(3,&mask);
    if(sched_setaffinity(0,sizeof(mask),&mask) == -1)
    {
        ERROR_LOG("************************************************set cpu affinity failed!");
    }

	udpSocket0 = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket0 <= 0)
	{
		ERROR_LOG("create socked faild with %d", udpSocket0);
		goto EXIT0;
	}

	on = 512 * 1024;
	rtn = setsockopt(udpSocket0, SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof(int)); //设置套接字选项

	on = 512 * 1024;
	rtn = setsockopt(udpSocket0, SOL_SOCKET, SO_RCVBUF, (char *)&on, sizeof(int));

	on = 1;
	setsockopt(udpSocket0, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(int));

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(udp_port_);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	rtn = bind(udpSocket0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rtn < 0)
	{
		ERROR_LOG("socket bind failed!");
		goto EXIT0;
	}
#if 1
	while (runFlag0)
	{
		rtn = recvfrom(udpSocket0, (char *)&recPackTemp, sizeof(CMD_SEND_FRAME), 0, (struct sockaddr *)&r_addr, &addrLen); /*udp接收*/
		if (rtn == sizeof(CMD_SEND_FRAME))
		{
			//  printf("%s %d recPackTemp.cmd:0x%x\r\n",__FUNCTION__,__LINE__,recPackTemp.cmd);
			if (recPackTemp.head == 0x55AA55AA)
			{
				if (fistPack)/* 首包初始化 */
				{
					packNum = recPackTemp.cmdCnt;
					fistPack = 0;
				}

				//判断包计数正确开始处理数据包，或者重传的数据包
				if ((packNum == recPackTemp.cmdCnt) || (packNum == (recPackTemp.cmdCnt + 1)))
				{
					switch (recPackTemp.cmd)
					{
					/***************自检***************/
					case SYSTEM_CHECK_CMD: /*自检命令0x4501*/
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);
						/*再回复reply帧*/
						memset((char *)&replySystemCheck,0,sizeof(SYSTEM_CHECK_REPLY));
						getSysInfo();
						replySystemCheck.AISysVer = osVer;	
						replySystemCheck.softVer = SoftVer; 
						replySystemCheck.runtime = gRunTimeMs;		 /* 运行时间 */
						replySystemCheck.checkSelf = gSoftStat;				/* 自检结果 */
						replySystemCheck.temper = gNpuTemper;			/* NPU温度 */
						replySystemCheck.resMem = gMemTotal - gMemUse; /* 剩余内存空间 */
						replySystemCheck.modelTotalNum = gModTotalNum;/* 识别模型总数 */

						for(i=0;i<gModTotalNum;i++)/* 填写模型编号信息 */
						{
							replySystemCheck.modinfo[i].modIndex=gModeInfo[i].modIndex;
							replySystemCheck.modinfo[i].modNum=gModeInfo[i].modNum;
						}

						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&replySystemCheck,sizeof(SYSTEM_CHECK_REPLY));
						break;
					/***************图像***************/
					case IMAGE_PREPARE_CMD: /*图像准备命令0x4502*/
						INFO_LOG("DSP SEND 0x4502\r\n");
						recCnt = 0;
						preParePack = (PREPARE_FRAME *)&recPackTemp.data[0];
						imageIndex = preParePack->imageIndex;							   /*获取图像编号*/
						imageWidth = preParePack->width;								   /*获取图像宽度*/
						imageHeight = preParePack->height;								   /*获取图像高度*/
						alignYuv420_size_ = imageWidth * imageHeight * 3 /2;			   /* 对齐YUV420后的大小 */
						totalPackNum = ((imageWidth) * (imageHeight)) / image_payload_len; /*获取图像数据总包数,如不是整除后续+1*/
						totalImageLen = (preParePack->width) * (preParePack->height);	   /*获取图像数据总长度*/
						if (((preParePack->width) * (preParePack->height)) % image_payload_len)
							totalPackNum++;
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);

						/*再回复reply帧*/
						memset((char *)&recPackLog[0],0,lostMaxCnt);/* 将丢包记录清空 */
						memset(picStore[stPtr].picAddr,0x80,alignYuv420_size_);/* 将即将要存储的地址按照对齐后的图像大小清空 */

						memset((char *)&preReply,0,sizeof(PREPARE_REPLY));
						getNpuTemp();
						preReply.npuTemp = gNpuTemper;
						preReply.imageIndex = imageIndex;

						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&preReply,sizeof(PREPARE_REPLY));
						break;
					case IMAGE_DATA_SEND: /*图像上传命令0x4503*/
						rtn = imageRec(imageIndex, (IMAGE_FRAME *)&recPackTemp.data[0]);
						if (rtn != 0)
						{
							ERROR_LOG("IMAGE_DATA_SEND imageRec is error :%d", rtn);
						}
						break;
					case IMAGE_REC_QUERY: /*图像结果查询命令0x4504*/
						INFO_LOG("DSP SEND 0x4504\r\n");
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);
						/*再回复reply帧*/
						if (totalPackNum != recCnt)
						{
							ERROR_LOG("4504 image%d Total = %d,recCnt= %d\r\n",imageIndex,totalPackNum,recCnt);
						}
						memset((char *)&replyQuery,0,sizeof(REC_QUERY));
						replyQuery.imageIndex = imageIndex;
						replyQuery.packTotalNum = totalPackNum;
						for (i = 0; i < totalPackNum; i++) //记录丢包数据
						{
							if (recPackLog[i] == 0)
							{
								ERROR_LOG("recCnt:%d,recPackLog[%d]:%d",recCnt,i,recPackLog[i]);
								replyQuery.lostIndex[replyQuery.lostPackCnt] = i + 1;
								replyQuery.lostPackCnt++;
							}
						}
						 
						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&replyQuery,sizeof(REC_QUERY));
						break;
					/***************识别***************/
					case DETECT_CMD_QUERY: /*启动识别命令0x4505*/
						INFO_LOG("DSP SEND 0x4505\r\n");
						detecCmd = (DSP_DETECTION *)&recPackTemp.data[0];
						deImageIndex = detecCmd->imageIndex; /*获取需要识别的图像编号*/
						deModeOrd = detecCmd->sModeOrdinal;	 /*获取需要识别的模型序号*/
						//准备回传数据包
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);

						fSeekPic = checkImgExist(deImageIndex);/* 查找该编号图片是否存在 */
						fSeekOm  = checkOmExist(deModeOrd);/* 查找该序号模型是否存在 */

						memset((char *)&ackDetecCmd,0,sizeof(DETECTION_ACK));
						if(fSeekPic != 0 || fSeekOm != 0)/* 如果不满足识别条件 */
						{
							/*再回复reply帧*/
							ackDetecCmd.imageIndex = deImageIndex;
							ackDetecCmd.sModeOrdinal = deModeOrd;
							ackDetecCmd.status = UNALLOW_DETECT; //文件不存在，不允许识别
							ackDetecCmd.inferStat = DETECT_FAILED; //文件不存在，反馈推理失败
							if (fSeekPic!=0)
							{
								ackDetecCmd.reason |= fSeekPic;/* 图片不存在 */
								ackDetecCmd.failReason |= fSeekPic;/* 图片不存在 */
							}
							if (fSeekOm!=0)
							{
								ackDetecCmd.reason |= fSeekOm;/* 模型序号的文件不存在 */
								ackDetecCmd.failReason |= fSeekOm;/* 模型序号的文件不存在 */
							}	
						}
						else/* 满足识别条件 */
						{
							sem_trywait(&semInfer);
							sem_trywait(&semInferEnd);
							
							sem_post(&semInfer);/* 启动推理进程 */
#ifdef gTestTime
							clock_gettime(CLOCK_REALTIME,&tRecordStart);/*获取开始推理时间*/
#endif
							clock_gettime(CLOCK_REALTIME,&tpoint);/*获取开始推理时间*/
							if (tpoint.tv_nsec > (999999999 - (inferTimeWaitMs * 1000 * 1000)))
							{
								tpoint.tv_nsec = (inferTimeWaitMs * 1000 * 1000) - (999999999 - (tpoint.tv_nsec));/*时间换算为ms*/
								tpoint.tv_sec += 1;
							}
							else{
								tpoint.tv_nsec += inferTimeWaitMs * 1000 * 1000;/*时间换算为ms*/
							}
							
							rtn = sem_timedwait(&semInferEnd,&tpoint);/*等待结束信号量*/

#ifdef gTestTime
							clock_gettime(CLOCK_REALTIME,&tRecordEnd);/*获取结束推理时间*/
							testInferTimeMs = ((tRecordEnd.tv_nsec - tRecordStart.tv_nsec)/1000/1000 ) \
											+ ((tRecordEnd.tv_sec - tRecordStart.tv_sec)*1000 );
							INFO_LOG("infer use time = %lld ms",testInferTimeMs);
#endif
							ackDetecCmd.imageIndex = deImageIndex;
							ackDetecCmd.sModeOrdinal = deModeOrd;
							ackDetecCmd.status = ALLOW_DETECT; //文件存在，允许识别
							ackDetecCmd.detectNum = gDetectNum;
							ackDetecCmd.failReason = gInferFailseReason;
							if (rtn != 0) /*如果超时了*/
							{
								ERROR_LOG("img%d model%d infer time_out,cmdCnt =%d",\
												deImageIndex,deModeOrd,recPackTemp.cmdCnt);		
								ackDetecCmd.inferStat=DETECT_ING;/* 超时返回推理中，后续须发送查询命令查询结果 */
							}
							else
							{
								/*填写结果*/
								res = checkDetectExist(deImageIndex);
								if (res == SUCCESS)/* 如果成功 则结果赋值*/
								{
									ackDetecCmd.inferStat=DETECT_SUCCESS;/* 推理成功 */
									ackDetecCmd.detectNum = gDetectRes[deGetResPtr].dTarNum;
									memcpy((char *)&ackDetecCmd.objInfo[0],\
										(char *)&gDetectRes[deGetResPtr].objInfo[0],(sizeof(OBJECT_INFO) * 40));/* 拷贝正确结果 */
									INFO_LOG("x:%d,y:%d",ackDetecCmd.objInfo[0].obj_x ,ackDetecCmd.objInfo[0].obj_y);
								}
								else
								{
									deteResRely.inferStat  = DETECT_FAILED;
									deteResRely.failReason = gInferFailseReason;
								}
							}
						}
						/*再回复reply帧*/
						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&ackDetecCmd,sizeof(DETECTION_ACK));
						break;
					case OBJECT_QUERY: /*识别结果命令0x4506*/
						detecCmd = (DSP_DETECTION *)&recPackTemp.data[0];
						deImageIndex = detecCmd->imageIndex; /*获取需要识别的图像编号*/
						deModeOrd = detecCmd->sModeOrdinal;	 /*获取需要识别的模型编号*/
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);

						/*再回复reply帧*/
						memset((char *)&deteResRely,0,sizeof(DETECTION_QUERY));
						deteResRely.imageIndex = deImageIndex;
						deteResRely.sModeOrdinal = deModeOrd;

						res = checkDetectExist(deImageIndex);
						if (res == SUCCESS)/* 如果成功 则结果赋值*/
						{
							deteResRely.inferStat = DETECT_SUCCESS;
							deteResRely.detectNum = gDetectRes[deGetResPtr].dTarNum;
							memcpy((char *)&ackDetecCmd.objInfo[0],\
										(char *)&gDetectRes[deGetResPtr].objInfo[0],(sizeof(OBJECT_INFO) * 40));
						}
						else
						{
							deteResRely.inferStat  = DETECT_FAILED;
							deteResRely.failReason = gInferFailseReason;
						}

						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&deteResRely,sizeof(DETECTION_QUERY));
						break;

					/***************模型***************/
					case MODE_PRE_QUERY:	/*模型准备命令0x4507*/
						INFO_LOG("DSP SEND 0x4507\r\n");
						upDataType = 0;/* 升级文件类型 */
						upSoftLanguage = 0; /*升级程序语言*/
						upModelOrd = 0;/* 模型文件序号 */
						crcVal = 0;		 /*实际计算的CRC校验值*/
						dataInfoLen = 0;	 /*数据信息长度*/
						modeInfoLen = 0;	 /*模型信息长度*/
						upErrBit = 0; /*错误原因Bit*/
						upDataLen = 0;/* 升级文件总长度 */
						upDataRlLen = 0;/* 数据文件长度 */
						recUpFilePackCnt=0;/* 数据装订接收计数清0 */

						upLoadCmd = (MODE_INFO *)&recPackTemp.data[0];
						upDataLen = upLoadCmd->dataLen;	  /*获取整个装订数据的长度*/
						totalPackNum = (upDataLen) / update_payload_len;
						if ((upDataLen) % update_payload_len)
							totalPackNum++;
						//准备回传数据包
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);
						/*再回复reply帧*/
						memset(updateFileAddr,0,upDataLen);		/* 把升级文件地址按照接收数据大小清空 */
						memset(modelInfoAddr,0,sizeof(GET_INFO));/* 把模型信息文件地址按照INFO大小清空 */

						memset((char *)&ackModeCmd,0,sizeof(MODE_INFO));
						ackModeCmd.dataLen = upDataLen;
						ackModeCmd.packLen = upLoadCmd->packLen;
						ackModeCmd.packTotalLen = upLoadCmd->packTotalLen;

						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&ackModeCmd,sizeof(MODE_INFO));
						break;
					case MODE_UPLOAD_QUERY: /*模型数据上传命令0x4508*/
						checkSum = 0;
						checkSum = getSumCheck((char *)&recPackTemp.cmd, 1012);/* 计算接收的每包数据的校验和 */

						upLoadMode = (MODE_DATA *)&recPackTemp.data[0];						
						memset(&ackFrame, 0, sizeof(CMD_ACK_FRAME));
						ackFrame.head = 0xAA55AA55;
						ackFrame.len = recPackTemp.len;
						ackFrame.cmd = recPackTemp.cmd;
						if (checkSum == recPackTemp.checkSum) /*判断校验和是否正确*/
						{
							ackFrame.state = 0xAA00; /*校验成功*/
							rtn = updateFileRec((MODE_DATA *)&recPackTemp.data[0]);/* 校验成功接收信息 */
							if (rtn != 0)
							{
								ERROR_LOG("mode rec pack%d err,rtn =%d", upLoadMode->packIndex, rtn);
							}
						}
						else
						{
							ERROR_LOG("check sum err\r\n");
							ackFrame.state = 0xAA44; /*校验失败*/
						}
						ackFrame.cmdCnt = recPackTemp.cmdCnt;		
						replyModeData = (MODE_DATA *)&ackFrame.data[0];
						replyModeData->packTotalNum = upLoadMode->packTotalNum;
						replyModeData->packIndex = upLoadMode->packIndex;
						replyModeData->packlen = upLoadMode->packlen;
						replyModeData->errstate = rtn;

						ackFrame.checkSum = getSumCheck((char *)&ackFrame.cmd, 1012);
						sendto(udpSocket0, (char *)&ackFrame, sizeof(CMD_ACK_FRAME), 0,
								(const struct sockaddr *)&r_addr, sizeof(r_addr));
						
						if (recUpFilePackCnt == replyModeData->packTotalNum) /*接收最后一包正常后写成文件*/
						{
							INFO_LOG("rec success,begin crc\r\n");
							crcVal = crc16((uint8_t*)&updateFileAddr[0], upDataLen);/* AI计算crc校验 */
							if (crcVal == 0)/* 校验和正确 */
							{
								res = updateFile(upDataType);
								if (res !=0)
								{
									upErrBit |= (uint32_t)res;/* 记录错误原因文件操作失败*/
								}
							}
							else
							{
								ERROR_LOG("crcVal = 0x%x",crcVal);
								upErrBit |= (uint32_t)CRC_ERR;/* 记录错误原因crc校验 */
							}
						}

						break;
					case MODE_QUERY: /*模型查询结果命令0x4509*/
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);
						/*再回复reply帧*/
						memset((char *)&replyModeRes,0,sizeof(MODE_REC_QUERY));
						if (upErrBit == 0x0)
						{
							replyModeRes.status = SOLIDIFIED_OK;
						}
						else
						{
							replyModeRes.status = SOLIDIFIED_FAILED;
							replyModeRes.reason = upErrBit; // 数据装订错误
						}

						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&replyModeRes,sizeof(MODE_REC_QUERY));
						break;

					/***************模型信息***************/
					case INFO:/*模型信息查询命令0x450A*/
						modelInfoNum = (INFO_REC *)&recPackTemp.data[0];
						//准备回传数据包
						/*先回复ask帧*/
						aiAsk(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt);
						memset((char *)&modelInfoAddr[0],0,sizeof(GET_INFO));

						infoName = ObjectDetect::get_ModelPath() + to_string(modelInfoNum->modelNum) + ".info";
						fdSeekInfo = access(infoName.c_str(), F_OK); /*查询该路径下info是否存在*/
						if (fdSeekInfo == 0)/* 如果有这个文件才进行读取，没有就直接返回全0 */
						{
							// INFO_LOG("read %d info begin",modelInfoNum->modelNum);
							fdInfo = open(infoName.c_str(), O_RDWR, 0777);
							if (fdInfo < 0)
							{
								ERROR_LOG("%s open error", infoName.c_str());
								break;
							}
							rtn = read(fdInfo, (char *)&modelInfoAddr[0], sizeof(GET_INFO));
							if (rtn != sizeof(GET_INFO))
							{
								ERROR_LOG("read %s error!!",infoName.c_str());
								close(fdInfo);
							}
						}
						else
						{
							ERROR_LOG("%s seek error", infoName.c_str());
						}
						close(fdInfo);
						/*再回复reply帧*/
						aiReply(recPackTemp.len,recPackTemp.cmd,recPackTemp.cmdCnt,(char *)&modelInfoAddr[0],sizeof(GET_INFO));
							break;

						default:
							break;
					}

					//如果数据包是重传的数据包就包计数不做累加
					if (packNum == (recPackTemp.cmdCnt + 1))
					{
						packNum--; //接收到重传数据包,包计数不错累加，因为每次循环包计数都加一，所以这个地方减个1
					}
				}
				else
				{
					//包计数不对，获取当前数据包重新开始计数，这一包就不处理了
					packNum = recPackTemp.cmdCnt;
				}

				//包计数加一，下次期望的包计数值
				packNum++;
			}
		}
	}
#endif
EXIT0:
	close(udpSocket0);
	udpSocket0 = 0;
	return (0);
}

/***************************推理相关函数***************************/
/*******************************************************
 * 函 数 Result StoreDetectResult(Image& image, aclmdlDataset*& modelOutput)
 *
 * 函数说明:	获取并存储推理结果 
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: StoreDetectResult(Image& image, aclmdlDataset*& modelOutput)
{
	Result ret = ObjectDetect::Postprocess(image, modelOutput,gDetectRes[stDetectPtr]);
	if (ret != SUCCESS) {
		ERROR_LOG("Process model inference output data failed");
		return NO_DETECT_RES;
	}

	stDetectPtr = (stDetectPtr + 1) % stMaxSize;
	return SUCCESS;
}
/*******************************************************
 * 函 数 int inferThread0(void)
 *
 * 函数说明:	推理进程
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
int trans :: inferThread0(void)
{
	aclmdlDataset* inferenceOutput;
	Image image;
	aclError rtn;
	Result ret = ObjectDetect::Init(gModTotalNum);/* 初始化acl及加载模型 */
    if (ret != ACL_SUCCESS) {
        ERROR_LOG("Init resource failed %d", ret); 
        return FAILED;               
    }
	inferStart = 1;
	printf("init success\r\n");
	while (runFlag0)
	{
		sem_wait(&semInfer);
		INFO_LOG("get semInfer ,begin infer\r\n");
#ifdef gTestTime
			clock_gettime(CLOCK_REALTIME,&tRecordStart1);/*线程开始时间*/
#endif
		rtn = acldvppMalloc((void **)&inputImgAddr_,alignYuv420_size_);/* 将普通地址下的内容拷贝到dvppmallloc下使用 */
		if (rtn != ACL_SUCCESS) {
			ERROR_LOG("acldvppMalloc failed");
			continue;
		}
		memset(inputImgAddr_, 0x80, alignYuv420_size_);/* 将整幅图清理成灰度图 */
		rtn = aclrtMemcpy(inputImgAddr_,alignYuv420_size_,(char*)&picStore[deGetImgStPtr].picAddr[0],\
							alignYuv420_size_,ACL_MEMCPY_DEVICE_TO_DEVICE);
		if (rtn != ACL_SUCCESS) {
			ERROR_LOG("aclrtMemcpy failed");
			continue;
		}
#ifdef gTestTime
			clock_gettime(CLOCK_REALTIME,&tRecordEnd1);/*获取搬移时间*/
			testTimeCpyUs = ((tRecordEnd1.tv_nsec - tRecordStart1.tv_nsec)/1000 ) \
							+ ((tRecordEnd1.tv_sec - tRecordStart1.tv_sec)*1000 *1000 );
			INFO_LOG("copy use time = %lld us",testTimeCpyUs);
#endif
		// close(fdTest1);
		printf("start infer....\r\n");
		image.deImgIndex = deImageIndex;
		image.imgWidth   = imageWidth;
		image.imgHeight  = imageHeight;
		image.data       = inputImgAddr_;/*图像存储地址*/
		image.dataSize   = alignYuv420_size_;
		image.detecId    = ObjectDetect::checkModelId(deModeOrd);/* 检查模型对应是否id */
		if (image.detecId < 0)
		{
			ERROR_LOG("check Id failed,no this model,input index = %d",deModeOrd);
			// return NO_MODEL_INDEX;
			gInferFailseReason = NO_MODEL_INDEX;
			continue;
		}

		ret = ObjectDetect::inferBeforeCreatDesc(image.detecId,image.imgWidth,image.imgHeight);/* 创建输入输出设备 */
		if (ret != SUCCESS) {
			ERROR_LOG("inferBeforeCreatDesc failed");
			ObjectDetect :: inferEndDestroyDesc();
			continue;
		}

		ret = ObjectDetect::Inference(inferenceOutput, image, image.detecId);/* 执行推理 */
		if (ret != SUCCESS) {
			ERROR_LOG("Inference model inference output data failed");
			ObjectDetect :: inferEndDestroyDesc();
			acldvppFree(inputImgAddr_);
			continue;
		}

		ret = StoreDetectResult(image, inferenceOutput);/* 从输出流中获取结果 */
		if (ret != SUCCESS) {
			ERROR_LOG("Process model inference output data failed");
			ObjectDetect :: inferEndDestroyDesc();
			acldvppFree(inputImgAddr_);
			continue;
		}

		ObjectDetect :: inferEndDestroyDesc();/* 推理结束释放相关空间 */
		acldvppFree(inputImgAddr_);
#ifdef gTestTime
			clock_gettime(CLOCK_REALTIME,&tRecordEnd1);/*获取结束推理时间*/
			testPthTimeMs = ((tRecordEnd1.tv_nsec - tRecordStart1.tv_nsec)/1000/1000 ) \
							+ ((tRecordEnd1.tv_sec - tRecordStart1.tv_sec)*1000 );
			INFO_LOG("pth run use time = %lld ms",testPthTimeMs);
#endif
		sem_post(&semInferEnd);/* 通知net线程推理结束 */
	}

    // detect.DestroyResource();
	return 0;
}
/***************************初始化相关函数***************************/
/*******************************************************
 * 函 数 Result ThreadCreate(void)
 *
 * 函数说明:	线程创建
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
Result trans :: ThreadCreate(void)
{
	int res;
	Result ret;

	getModeInfo();

	ret = MemCreat(stMaxSize);
	if (ret != SUCCESS)
	{
		ERROR_LOG("MemCreat init err");
		return FAILED;
	}

	res = sem_init(&semInfer,0,0);
	if(res!=0) 
	{ 
		ERROR_LOG("semInfer init err");
		return FAILED; 
	}

	res = sem_init(&semInferEnd,0,0);
	if(res!=0) 
	{ 
		ERROR_LOG("semInferEnd init err"); 
		return FAILED;
	}
	infer_thread_ = std:: thread(std::bind(&trans::inferThread0,this));
#ifdef testDebug
		sleep(2);
		inferPthTest();
#endif
    net_thread_ = std:: thread(std::bind(&trans::netThread0,this));
	return SUCCESS;
}
/*******************************************************
 * 函 数 void ThreadClose(void)
 *
 * 函数说明:	线程关闭
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
void trans :: ThreadClose(void)
{
	runFlag0 = 0;

	close(udpSocket0);
	
	pthread_cancel(net_thread_.native_handle()); //删除线程
	pthread_join(net_thread_.native_handle(), NULL);

	sem_destroy(&semInfer);
	sem_destroy(&semInferEnd);
	pthread_cancel(infer_thread_.native_handle()); //删除线程
	pthread_join(infer_thread_.native_handle(), NULL);

	MemDestroy();
}
/*******************************************************
 * 函 数 void waitThreadClose(void)
 *
 * 函数说明:	等待线程关闭
 * 参数说明:
 * 输入参数: 	
 * 输出参数:
 * 返 回 值: 
 *
 * 修改记录：
 * --------------------
 *	  2023/03/02  jianjikai
 *
 ********************************************************/
void trans :: waitThreadClose(void)
{
	pthread_join(infer_thread_.native_handle(), NULL);
    pthread_join(net_thread_.native_handle(), NULL);
}


#ifdef testDebug
int trans ::  inferPthTest(void)
{
	uint32_t rtn=0;
	
	string testName;
	imageWidth = 640;
	imageHeight = 640;
	alignYuv420_size_ = (imageWidth*imageHeight * 3 /2);
	// testSize =259584;
	printf("start test ,size  =%d......\r\n",alignYuv420_size_);
	
	memset((char*)&picStore[deGetImgStPtr].picAddr[0], 0x80, 4*1024*1024);

	testName ="/home/HwHiAiUser/jjk/testOutWrite.yuv";
	printf("%s begin read\r\n",testName.c_str());
	fdTest0 = open(testName.c_str(), O_RDWR , 0777);
	if (fdTest0 < 0)
	{
		printf("open %s file err\r\n",testName.c_str());
		return -1;
	}
	

	rtn = read(fdTest0, (char*)&picStore[deGetImgStPtr].picAddr[0], alignYuv420_size_);
	if (rtn != alignYuv420_size_)
	{
		printf("read %s file err:%d\r\n",testName.c_str(),rtn);
		close(fdTest0);
		return -2;
	}

	close(fdTest0);
	printf("read over send to infer\r\n");
	deModeOrd = 0;
	sem_post(&semInfer);
	clock_gettime(CLOCK_REALTIME,&tpoint);/*获取开始推理时间*/
	tpoint.tv_nsec += inferTimeWaitMs * 1000 * 1000;/*时间换算为ms*/
	rtn = sem_timedwait(&semInferEnd,&tpoint);/*等待结束信号量*/
	if (rtn !=0)
		printf("infer time out \r\n");
	else
		printf("use %d.om infer ok \r\n",deModeOrd);

	deModeOrd = 1;
	sem_post(&semInfer);
	clock_gettime(CLOCK_REALTIME,&tpoint);/*获取开始推理时间*/
	tpoint.tv_nsec += inferTimeWaitMs * 1000 * 1000;/*时间换算为ms*/
	rtn = sem_timedwait(&semInferEnd,&tpoint);/*等待结束信号量*/
	if (rtn !=0)
		printf("infer time out \r\n");
	else
		printf("use %d.om infer ok \r\n",deModeOrd);
	
	// deModeOrd = 3;
	// sem_post(&semInfer);
	// clock_gettime(CLOCK_REALTIME,&tpoint);/*获取开始推理时间*/
	// tpoint.tv_nsec += inferTimeWaitMs * 1000 * 1000;/*时间换算为ms*/
	// rtn = sem_timedwait(&semInferEnd,&tpoint);/*等待结束信号量*/
	// if (rtn !=0)
	// 	printf("infer time out \r\n");
	// else
	// 	printf("use %d.om infer ok \r\n",deModeOrd);
	while (1);
	// acldvppFree(test);

	return 0;
}
#endif