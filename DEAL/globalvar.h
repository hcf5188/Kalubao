#ifndef __GLOBALVAR_H__
#define __GLOBALVAR_H__

#include "includes.h"
typedef enum
{
	LinkOK = 0,
	LinkFault
}LinkStatus;

typedef struct
{
	uint8_t ipAddr[18];//用来存储IP地址
	uint8_t ipPotr[6]; //存储IP端口号
	uint8_t imei[18];  //存储IMEI号
	LinkStatus link;   //链路连接状态

}CDMAInformation;

























#endif

