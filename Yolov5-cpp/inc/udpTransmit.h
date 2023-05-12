#ifndef UDPTRANSMIT_H
#define UDPTRANSMIT_H
#include <thread>
#include <utility>
#include <functional>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "dsmi_common_interface.h"
#include "inference.h"
#include "utils.h"

#define ALLOW_DETECT 	0xAA/* 允许推理 */
#define UNALLOW_DETECT 	0xBB/* 不许推理 */
#define DETECT_SUCCESS 	0xAA/* 推理成功 */
#define DETECT_FAILED 	0xBB/* 推理失败 */
#define DETECT_ING 		0xCC/* 推理中 */

#define SOLIDIFIED_OK 	    0xAA/* 数据装订成功 */
#define SOLIDIFIED_FAILED 	0xBB/* 数据装订失败 */
typedef struct _cmd_send_frame_
{
	unsigned int head;
	unsigned int len;
	unsigned int cmd;
	unsigned int reserve;
	unsigned int cmdCnt;
	unsigned char data[1000];
	unsigned int checkSum;
}CMD_SEND_FRAME;

typedef struct _cmd_ack_frame_
{
	unsigned int head;
	unsigned int len;
	unsigned int cmd;
	unsigned int state;
	unsigned int cmdCnt;
	char data[1000];
	unsigned int checkSum;
}CMD_ACK_FRAME;
/* 系统状态查询 0x4501*/
#define SYSTEM_CHECK_CMD 0x4501
typedef struct _system_check_
{
	char           res[1000]; 
}SYSTEM_CHECK;

typedef struct _system_check_reply_
{
	unsigned int    AISysVer;               /*操作系统版本号*/
	unsigned int    softVer;               /* 软件版本号 */
	unsigned int    runtime;                /* 启动后运行时间 */
	unsigned int    checkSelf;              /* 自检结果 */
	unsigned int    temper;                 /* NPU温度 */
	unsigned int    resMem;				/* 剩余内存容量 */
	unsigned char    res[20];				/* 保留 */
	unsigned int  modelTotalNum; /* 识别模型总数 */
	MOD_INDEX_INFO modinfo[20];    /* 模型编号/序号信息 */ 
	unsigned char  	res1[792];      		    //保留
}SYSTEM_CHECK_REPLY;
/* 图像上传准备0x4502 */
#define IMAGE_PREPARE_CMD	0x4502
typedef struct _prepare_image_
{
	unsigned int    imageIndex; /* 图像编号 */           
	unsigned short  width;         /* 图像宽度 */
	unsigned short  height;       /* 图像高度 */
	unsigned short  imageType;  /* 图像类型 */
	unsigned short	normalLen;/* 常规包字节数 */
	unsigned short packTotal;/* 图像包总数 */
	unsigned char   res[986];
}PREPARE_FRAME;

typedef struct _prepare_reply_
{
	int                        npuTemp;     /* NPU温度 */
	unsigned int    imageIndex; /* 图像编号 */           
	unsigned char   res[986];
}PREPARE_REPLY;
/* 图像数据0x4503 */
#define IMAGE_DATA_SEND	0x4503
typedef struct _image_frame_
{
	unsigned int imageIndex; /* 图像编号 */
	unsigned short packTotalNum;/* 图像包总数 */
	unsigned short packIndex; /* 图像包编号 */
	unsigned short packlen; /* 当前包发送字节数 */
	unsigned char imageData[990];
}IMAGE_FRAME;
/* 图像结果查询0x4504*/
#define IMAGE_REC_QUERY 0x4504
typedef struct _image_rec_query_
{
	unsigned int imageIndex; 		/* 图像编号 */
	unsigned short packTotalNum;	/* 图像包总数 */
	unsigned short lostPackCnt; 	/* 丢包数 */
	unsigned short lostIndex[496];/* 丢包号 */
}REC_QUERY;



/*AI启动识别0x4505*/
#define DETECT_CMD_QUERY 0x4505
typedef struct _dsp_detection_/*DSP->AI*/
{
    unsigned int imageIndex; /* 图像编号 */
    unsigned int sModeOrdinal;/* 识别模型序号 */
    unsigned char  resData[992];
}DSP_DETECTION;

typedef struct _detection_ack_
{
    unsigned int  imageIndex; /* 图像编号 */
    unsigned int  sModeOrdinal;/* 识别模型序号 */
    unsigned int  status;/* 是否满足启动推理条件 */
    unsigned int  reason;/* 启动失败原因 */
	unsigned short detectNum; /* 识别目标个数 */ 
	unsigned short  inferStat; /*推理状态  */
	unsigned int  failReason;/* 推理失败原因 */
	unsigned char  res[8];/* 保留位 */
	OBJECT_INFO  objInfo[40];/* 识别结果信息 */
	unsigned char  res1[168];/* 保留位 */
}DETECTION_ACK;

/*识别结果查询0x4506*/
#define OBJECT_QUERY 0x4506
/*AI识别结果查询reply0x4506*/
typedef struct _detection_query_
{
    unsigned int  imageIndex; /* 图像编号 */
    unsigned int  sModeOrdinal;/* 识别模型序号 */
    unsigned short detectNum; /* 识别目标个数 */ 
	unsigned short  inferStat; /*推理状态  */
	unsigned int  failReason;/* 推理失败原因 */
	unsigned char   res[8];/* 保留 */
    OBJECT_INFO 	objInfo[40];/* 坐标n信息 */
	unsigned char   res1[176];/* 保留 */
}DETECTION_QUERY;

/* 模型/数据装订上传准备0x4507 */
#define MODE_PRE_QUERY 0x4507
typedef struct _mode_info_
{
	unsigned int  dataLen;              /* 数据长度 */
	unsigned int  packLen;              /* 常规包数据长度 */
	unsigned int  packTotalLen;    /* 包总数 */
	unsigned char res[988];            /* 保留 */
}MODE_INFO;

#define MODE_UPLOAD_QUERY 0x4508
/*数据装订指令0x4508*/
typedef struct _ack_mode_data_
{
	unsigned int  packTotalNum;/* 数据包总数 */
	unsigned int  packIndex; /* 数据包编号 */
	unsigned int  packlen; /* 当前包发送字节数 */
	         int  errstate;/*错误码*/
    unsigned char res[4];/* 保留 */
	unsigned char modeData[980];/* 装订数据,长度980 */
}MODE_DATA;

/*数据装订信息结构体*/
typedef struct _get_info_
{
	unsigned int  modeIndex;/*识别模型编号*/
	unsigned int  modeOrdinal;/*识别模型序号*/
	unsigned int  modeType;/*识别模型类型*/
	unsigned int  labelTotalNum;/*识别模型标签总数*/
	unsigned int  labelClassCode[81];/*识别标签类别编码*/
	unsigned int  inferWidth;/*推理宽度*/
	unsigned int  inferHeight;/*推理高度*/
	float  		  thresholdValue;/*置信度阈值*/
	float         iouValue;/*训练IOU阈值*/
	unsigned int  modelAccuracy;/*模型转换精度*/
	unsigned char res1[20];
}GET_INFO;

/*数据装订信息结构体*/
typedef struct _update_info_
{
    unsigned int  updateType;/*升级文件类型*/
    unsigned int  updateLen; /*升级文件长度*/
    unsigned int  softVer;/*软件版本号*/
	unsigned int  softTime;/*软件生产日期*/ 
	unsigned int  softLanguage; /*程序语言*/
	unsigned int  softOrdinal; /*程序序号*/
	unsigned char res[8];
	GET_INFO 	  modelInfo;/* 模型信息 */
	unsigned int  dataLen;
}UPDATE_INFO;

#define MODE_QUERY 0x4509
/*数据装订结果查询4509*/
typedef struct _mode_rec_query_
{
	unsigned int  status; /* 数据装订状态 */
	unsigned int  reason;/* 数据装订错误原因 */
	unsigned char    res[992];
}MODE_REC_QUERY;

#define INFO           0x450A
typedef struct _info_rec__
{
    unsigned int  modelNum;/* 模型编号 */
	unsigned char    res[996];
}INFO_REC;

typedef struct _pic_indexes_
{
	uint32_t picIndex;
	char *picAddr; 
}PIC_INDEXES;

class trans : public ObjectDetect
{
public:
	trans(const std::string& modelPath,const std::string& cplusPath):ObjectDetect(modelPath),cplusPath_(cplusPath){}
	~trans(){
		MemDestroy();
		ThreadClose();
	}

	Result ThreadCreate(void);
	void ThreadClose(void);
	void waitThreadClose(void);

	Result MemCreat(uint32_t creatNum);
	void MemDestroy();
	void aiAsk(unsigned int len,unsigned int cmd, unsigned int cmdCnt);
	void aiReply(unsigned int len,unsigned int cmd, unsigned int cmdCnt,char * replyData,int structLen);

	int getModeInfo(void);
	void getSysInfo(void);
	void getMemInfo(void);
	void getRunTime(void);
	void getNpuTemp(void);

	int inferThread0(void);/* 推理线程运行函数 */
    int netThread0(void);/* udp通信处理线程运行函数 */
private:/* 系统和传输相关 */
	int inferStart = 0; /*py启动成功标志*/
	int32_t deviceId = 0;/* 310获取系统信息设备号0 */
	int32_t udp_port_ = (0x9091); /*udp端口号*/

	int32_t udpSocket0;	/*socket*/
	struct sockaddr_in r_addr;    
	struct sockaddr_in server_addr;
	
	struct dsmi_memory_info_stru memInfo;/* 310内存信息结构体 */

	sem_t semInfer;/*推理操作发起信号量*/
	sem_t semInferEnd;/*推理状态信号量，用于管控推理是否完成或超时*/
	
	uint32_t SoftVer=0x83580100;/*软件版本号[31:16]为版本标识0x8358,[15:0]为C++端版本:0x0100为C++端V1.0*/
	uint32_t osVer= 0x1804;/* 操作系统版本号18.04 */
	uint32_t gSoftStat=0;/* 获取软件运行状态*/
	int32_t gNpuTemper = 0; /* 获得的系统温度 */
	uint32_t gMemUse = 0;	 /* 获得的系统使用大小 */
	uint32_t gMemTotal = 0;	 /* 获得的系统总大小 */
	uint32_t gRunTimeMs = 0;/* 获得的系统运行时间 */

	uint32_t totalPackNum = 0; /*图像或模型数据传输时的总包数*/
	
private:/*线程相关变量*/
	int runFlag0 = 1; /*运行标志*/
	std :: thread infer_thread_; /* 推理线程 */
    std :: thread net_thread_; /* udp通信处理线程 */
	// int inferPthTest(void);

private:/* 图像相关 */
	uint16_t imageWidth=0;	/*图像宽度*/
	uint16_t imageHeight=0;	/*图像高度*/
	uint32_t totalImageLen = 0;	/*由图像上传准备命令中获取的图像长宽所计算的图像总大小*/
	uint32_t alignYuv420_size_=0;/* 图像对齐YUV420的大小 */
	uint32_t image_payload_len = (990);		 /*图像命令包中图像数据长度*/
	uint32_t imgBufferMaxSize = (6*1024*1024);
	uint32_t lostMaxCnt = (imgBufferMaxSize / image_payload_len);/*图像丢包计数最大丢包长度*/

	uint32_t recCnt = 0;/*记录接收到的图像数据包数量*/
	uint16_t *recPackLog;/* 记录丢包编号的空间 */
	
	uint8_t stPtr = 0;/* 存储位置 */
	uint8_t stMaxSize=10;/* 图像最大存储个数 */
	PIC_INDEXES picStore[10];/* 存储图片信息索引表*/
	Result checkImgExist(uint32_t checkImgIndex);/* 检查该编号图像是否存在 */
	int imageRec(unsigned int imageIndex, IMAGE_FRAME *recImage);/* 图像接收 */

private:/* 识别相关 */
	char* inputImgAddr_;/* 输入图像地址 */

	uint32_t deModeOrd = 0;  /*通过启动识别命令获取到的需识别的模型序号*/
	uint32_t deImageIndex = 0; /*通过启动识别命令获取到的需识别的图像编号*/

	uint32_t gDetectNum=0;/*识别出的目标个数*/
	int32_t inferTimeWaitMs = 300;/*设置推理超时时间为300 ms*/
	uint32_t gInferFailseReason = 0;/*推理失败原因*/
	
	uint8_t  stDetectPtr=0;/* 存储推理结果位置 */
	uint8_t  deGetResPtr=0;/* 获取到的准备推理的图像所存储到picStore信息索引位置 */
	uint8_t  deGetImgStPtr=0;/* 获取到的准备推理的图像所存储到picStore信息索引位置 */
	DETECT_RESULT gDetectRes[10];/* 存储推理结果的索引表 */
	Result StoreDetectResult(Image& image, aclmdlDataset*& modelOutput);/* 推理并存储结果 */
	Result checkDetectExist(uint32_t checkImgIndex);/* 查找是否存在该编号图片的结果 */

private:/* 模型和文件升级及信息相关 */
	std :: string cplusPath_; /* c++程序升级地址 */

	uint32_t upDataLen=0;				   /*上传数据长度,字节*/
	uint32_t upSoftVer=0;                  /* 程序版本号 */
	uint32_t upDataType=0;				   /*上传数据类型:0xAA程序,0xBB模型*/
	uint32_t upSoftLanguage = 0;		   /*升级程序语言*/
	
	uint32_t upDataRlLen = 0;			   /*升级文件实际长度*/
	uint32_t dataInfoLen = 0;				   /*数据包信息长度*/
	uint32_t modeInfoLen = 0;				   /*模型信息长度*/

	int8_t *updateFileAddr;/* 升级时存储地址 */
	int8_t *modelInfoAddr;/* 升级时模型信息存储 */
	
	uint32_t upModelOrd = 0;			   /*模型文件序号*/
	uint32_t recUpFilePackCnt = 0;			   /*记录接收到的升级数据包数量*/
	
	uint32_t update_payload_len = (980);	 /*升级命令包中模型数据长度*/
	uint32_t modInfoMaxSize   = (1 * 1000);	 /*模型信息大小:最大1K字节*/
	uint32_t updateFileMaxSize = (25 * 1024 * 1024); /*升级文件大小:最大25M字节*/

	GET_INFO gModeInfoBuf;			/*读取文件列表的模型信息存储空间*/
	uint32_t gModTotalNum = 0;/* 文件列表读取的模型总数 */
	MOD_INDEX_INFO gModeInfo[20];/* 解析后的模型信息存储数组 */
	uint32_t gModeNoInfo[20];/* 查找到没有Info的模型文件的编号 */

	uint32_t crcVal = 0;/*实际计算的CRC校验值*/
	uint32_t upErrBit = 0;/*数据装订错误标志*/
	Result checkOmExist(uint32_t checkOmIndex);/* 查询模型编号是否存在于上电读取的列表中 */
	int updateFileRec(MODE_DATA *recMode);/* 接收升级文件 */
	Result updateFile(unsigned int fileType);/* 升级文件 */

private:/* 文件描述符 */
    int32_t fdApp = 0; /*C++程序文件描述符*/
    int32_t fdMode = 0;	/*模型文件描述符*/
    int32_t fdInfo = 0;	/*模型信息文件描述符*/

    int32_t fdSeekApp = 0;	/*py文件启动后写的文件存在结果*/
    int32_t fdSeekOm = 0;	/*om文件查找结果*/
    int32_t fdSeekInfo = 0;	/*info文件查找结果*/

};

#endif