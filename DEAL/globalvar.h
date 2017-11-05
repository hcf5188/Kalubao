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
	uint8_t ipAddr[18];//�����洢IP��ַ
	uint8_t ipPotr[6]; //�洢IP�˿ں�
	uint8_t imei[18];  //�洢IMEI��
	LinkStatus link;   //��·����״̬

}CDMAInformation;

























#endif

