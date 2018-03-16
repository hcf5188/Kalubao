#include "obd.h"
#include "globalvar.h"


//自识别 CAN 总线上的波特率
CANBAUD_Enum canBaudEnum[NUMOfCANBaud]= {
	CANBAUD_250K,
	CANBAUD_500K,
	CANBAUD_1M,};//波特率选择
//自识别 ECU 版本号的指令
const CmdVersion canIdExt[NUMOfCANID_EXT] = 
{
	{0x18DA00FA,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},//陕汽德龙  渣土车  
	{0x18DA00FB,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},
	{0x18DA00F1,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},
	{0x18DA10FA,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},
	{0x18DA10FB,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},
	{0x18DA10F1,{0x02,0x1A,0x94,0x00,0x00,0x00,0x00,0x00}},//
	
	{0x18DA00FA,{0x03,0x22,0xF1,0x94,0x00,0x00,0x00,0x00}},//重汽 
	{0x18DA00FB,{0x03,0x22,0xF1,0x94,0x00,0x00,0x00,0x00}},
	{0x18DA00F1,{0x03,0x22,0xF1,0x94,0x00,0x00,0x00,0x00}},
	
	{0x18DA00FA,{0x03,0x22,0xF1,0x90,0x00,0x00,0x00,0x00}},
	{0x18DA00F1,{0x03,0x22,0xF1,0x90,0x00,0x00,0x00,0x00}}
};




//在此数组下确定发动机转速范围， 确定中载下的扭矩限制
//const uint16_t  TrqlimM[2][15]= 
// {
//	{0,  700,800,900,950,1000,1500,1600,1650,1675,1700,1725,1750,1900,5000},
//	{205,205,205,205,205, 205, 205, 205, 205, 190, 175, 160, 145, 135, 125}
// };
////在此数组下确定发动机转速范围，确定轻载下的扭矩限制
//const uint16_t TrqlimL[2][15]=          //轻载比中载第二象限略小一些
// {
//	{0,  700,800,900,950,1000,1500,1600,1650,1675,1700,1725,1750,1900,5000},
//	{185,185,185,185,185, 185, 185, 185, 185, 178, 170, 158, 145, 135, 125}
// };

const uint16_t  TrqlimM[2][15]= 
 {
	{0,  700,800,900,950,1000,1500,1600,1650,1675,1700,1725,1750,1900,5000},
	{205,205,205,205,205, 205, 205, 205, 205, 190, 175, 160, 145, 135, 125}
 };
//在此数组下确定发动机转速范围，确定轻载下的扭矩限制
const uint16_t TrqlimL[2][15]=          //轻载比中载第二象限略小一些
 {
	{0,  700,800,900,950,1000,1500,1600,1650,1675,1700,1725,1750,1900,5000},
	{205,205,205,205,205, 205, 205, 205, 205, 190, 175, 160, 145, 135, 125}
 };


//ECU 增强动力读数据的地址 有新的ECU的时候，将新版本添加进去，数组的个数加1即可
//每加一个版本，一定要测试通过才行。
//uint8_t pAddr[10][5] = {
//				{0x03,0x05,0x60,0xF8,0xFF},  //潍柴    P949V732  oilADDR
//				{0x03,0x05,0x6C,0x60,0xFF},  //        P949V791
//				{0x03,0x05,0x6C,0x60,0xFF},  //        P949V792
//				
//				{0x03,0x1C,0x60,0x38,0xFF},  //        P532V46     
//				{0x03,0x1C,0x60,0x38,0xFF},  //        P579V47
//				
//				{0x03,0x1C,0x9E,0xDC,0xFF},  //        P813V72 
//				{0x03,0x05,0x50,0xE0,0xFF},  //        P1499V301    
//				{0x03,0x04,0xD4,0x4C,0xFF},  //        P1072V742   
//				{0x03,0x04,0xD0,0x94,0xFF},  //重汽    P1158V760
//				{0x03,0x04,0xD0,0x94,0xFF}   //锡柴    P903V762
//			};

