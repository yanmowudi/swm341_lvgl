/****************************************************************************************************************************************** 
* 文件名称: SWM341_rtc.c
* 功能说明:	SWM341单片机的RTC驱动库
* 技术支持:	http://www.synwit.com.cn/e/tool/gbook/?bid=1
* 注意事项: 
* 版本日期:	V1.0.0		2016年1月30日
* 升级记录:  
*
*
*******************************************************************************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH CODING INFORMATION 
* REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME. AS A RESULT, SYNWIT SHALL NOT BE HELD LIABLE 
* FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT 
* OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION CONTAINED HEREIN IN CONN-
* -ECTION WITH THEIR PRODUCTS.
*
* COPYRIGHT 2012 Synwit Technology
*******************************************************************************************************************************************/
#include "SWM341.h"
#include "SWM341_rtc.h"


static uint32_t calcWeekDay(uint32_t year, uint32_t month, uint32_t date);
/****************************************************************************************************************************************** 
* 函数名称:	RTC_Init()
* 功能说明:	RTC初始化
* 输    入: RTC_TypeDef * RTCx	指定要被设置的RTC，有效值包括RTC
*			RTC_InitStructure * initStruct    包含RTC相关设定值的结构体
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_Init(RTC_TypeDef * RTCx, RTC_InitStructure * initStruct)
{		
	if(initStruct->clksrc == RTC_CLKSRC_XTAL32K)
	{
		PORT_Init(PORTA, PIN7, PORTA_PIN7_XTAL32_IN,  0);
		PORT_Init(PORTA, PIN6, PORTA_PIN6_XTAL32_OUT, 0);
		SYS->XTALCR |= (1 << SYS_XTALCR_32KON_Pos) | (7 << SYS_XTALCR_32KDRV_Pos);
	}
	else
	{
		SYS->LRCCR = (1 << SYS_LRCCR_ON_Pos);
	}
	SYS->CLKSEL &= ~SYS_CLKSEL_RTC_Msk;
	SYS->CLKSEL |= (initStruct->clksrc << SYS_CLKSEL_RTC_Pos);
	
	SYS->CLKEN1 |= (1 << SYS_CLKEN1_RTC_Pos);
	
	RTC_Stop(RTCx);
	
	while(RTCx->CFGABLE == 0);
	
	RTCx->MINSEC = (initStruct->Second << RTC_MINSEC_SEC_Pos) |
				   (initStruct->Minute << RTC_MINSEC_MIN_Pos);
	
	RTCx->DATHUR = (initStruct->Hour << RTC_DATHUR_HOUR_Pos) |
				   (initStruct->Date << RTC_DATHUR_DATE_Pos);
	
	RTCx->MONDAY = (calcWeekDay(initStruct->Year, initStruct->Month, initStruct->Date) << RTC_MONDAY_DAY_Pos) |
				   (initStruct->Month << RTC_MONDAY_MON_Pos);
	
	RTCx->YEAR = initStruct->Year;
	
	RTCx->LOAD = (1 << RTC_LOAD_TIME_Pos);
	
	RTCx->IF = 0x1F;
	RTCx->IE = (initStruct->SecondIEn << RTC_IE_SEC_Pos) |
			   (initStruct->MinuteIEn << RTC_IE_MIN_Pos);
	
	if(initStruct->SecondIEn | initStruct->MinuteIEn)
	{
		NVIC_EnableIRQ(RTC_IRQn);
	}
	else
	{
		NVIC_DisableIRQ(RTC_IRQn);
	}
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_Start()
* 功能说明:	启动RTC
* 输    入: RTC_TypeDef * RTCx	指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_Start(RTC_TypeDef * RTCx)
{
	RTCx->EN = 1;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_Stop()
* 功能说明:	停止RTC
* 输    入: RTC_TypeDef * RTCx	指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_Stop(RTC_TypeDef * RTCx)
{
	RTCx->EN = 0;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_GetDateTime()
* 功能说明:	获取当前的时间和日期
* 输    入: RTC_TypeDef * RTCx	指定要被设置的RTC，有效值包括RTC
*			RTC_DateTime * dateTime    获取到的时间、日期值存入此指针指向的结构体
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_GetDateTime(RTC_TypeDef * RTCx, RTC_DateTime * dateTime)
{
	dateTime->Year = RTCx->YEAR;
	dateTime->Month = (RTCx->MONDAY & RTC_MONDAY_MON_Msk) >> RTC_MONDAY_MON_Pos;
	dateTime->Date = (RTCx->DATHUR & RTC_DATHUR_DATE_Msk) >> RTC_DATHUR_DATE_Pos;
	dateTime->Day = 1 << ((RTCx->MONDAY & RTC_MONDAY_DAY_Msk) >> RTC_MONDAY_DAY_Pos);
	dateTime->Hour = (RTCx->DATHUR & RTC_DATHUR_HOUR_Msk) >> RTC_DATHUR_HOUR_Pos;
	dateTime->Minute = (RTCx->MINSEC & RTC_MINSEC_MIN_Msk) >> RTC_MINSEC_MIN_Pos;
	dateTime->Second = (RTCx->MINSEC & RTC_MINSEC_SEC_Msk) >> RTC_MINSEC_SEC_Pos;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_AlarmSetup()
* 功能说明:	RTC闹钟设定
* 输    入: RTC_TypeDef * RTCx	指定要被设置的RTC，有效值包括RTC
*			RTC_AlarmStructure * alarmStruct    包含RTC闹钟设定值的结构体
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_AlarmSetup(RTC_TypeDef * RTCx, RTC_AlarmStructure * alarmStruct)
{
	while(RTCx->CFGABLE == 0);
	
	RTCx->MINSECAL = (alarmStruct->Second << RTC_MINSECAL_SEC_Pos) |
					 (alarmStruct->Minute << RTC_MINSECAL_MIN_Pos);
	
	RTCx->DAYHURAL = (alarmStruct->Hour << RTC_DAYHURAL_HOUR_Pos) |
					 (alarmStruct->Days << RTC_DAYHURAL_SUN_Pos);
	
	RTCx->LOAD = (1 << RTC_LOAD_TIME_Pos) | (1 << RTC_LOAD_ALARM_Pos);
	
	RTCx->IF = (1 << RTC_IF_ALARM_Pos);
	RTCx->IE &= ~RTC_IE_ALARM_Msk;
	RTCx->IE |= (alarmStruct->AlarmIEn << RTC_IE_ALARM_Pos);
	
	if(alarmStruct->AlarmIEn)
	{
		NVIC_EnableIRQ(RTC_IRQn);
	}
	else
	{
		NVIC_DisableIRQ(RTC_IRQn);
	}
}

/****************************************************************************************************************************************** 
* 函数名称:	calcWeekDay()
* 功能说明:	计算指定年、月、日是星期几
* 输    入: uint32_t year		年
*			uint32_t month		月
*			uint32_t date		日
* 输    出: uint32_t			0 星期日    1 星期一    ... ...    6 星期六
* 注意事项: 无
******************************************************************************************************************************************/
static uint32_t calcWeekDay(uint32_t year, uint32_t month, uint32_t date)
{
    uint32_t i, cnt = 0;
    const uint32_t daysOfMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	for(i = 1; i < month; i++) cnt += daysOfMonth[i];
	
    cnt += date;
	
    if((year%4 == 0) && ((year%100 != 0) || (year%400 == 0)) && (month >= 3)) cnt += 1;
	
    cnt += (year - 1901) * 365;
	
    for(i = 1901; i < year; i++)
    {
        if((i%4 == 0) && ((i%100 != 0) || (i%400 == 0))) cnt += 1;
    }
	
    return (cnt+1) % 7;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntSecondEn()
* 功能说明:	秒中断使能
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntSecondEn(RTC_TypeDef * RTCx)
{	
	RTCx->IE |= (1 << RTC_IE_SEC_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntSecondDis()
* 功能说明:	秒中断禁止
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntSecondDis(RTC_TypeDef * RTCx)
{	
	RTCx->IE &= ~(1 << RTC_IE_SEC_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntSecondClr()
* 功能说明:	秒中断标志清除
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntSecondClr(RTC_TypeDef * RTCx)
{	
	RTCx->IF = (1 << RTC_IF_SEC_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntSecondStat()
* 功能说明:	秒中断状态
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: uint32_t				1 秒中断发生    0 秒中断未发生
* 注意事项: 无
******************************************************************************************************************************************/
uint32_t RTC_IntSecondStat(RTC_TypeDef * RTCx)
{
	return (RTCx->IF & RTC_IF_SEC_Msk) ? 1 : 0;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntMinuteEn()
* 功能说明:	分中断使能
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntMinuteEn(RTC_TypeDef * RTCx)
{	
	RTCx->IE |= (1 << RTC_IE_MIN_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntMinuteDis()
* 功能说明:	分中断禁止
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntMinuteDis(RTC_TypeDef * RTCx)
{	
	RTCx->IE &= ~(1 << RTC_IE_MIN_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntMinuteClr()
* 功能说明:	分中断标志清除
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntMinuteClr(RTC_TypeDef * RTCx)
{	
	RTCx->IF = (1 << RTC_IF_MIN_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntMinuteStat()
* 功能说明:	分中断状态
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: uint32_t				1 分中断发生    0 分中断未发生
* 注意事项: 无
******************************************************************************************************************************************/
uint32_t RTC_IntMinuteStat(RTC_TypeDef * RTCx)
{
	return (RTCx->IF & RTC_IF_MIN_Msk) ? 1 : 0;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntHourEn()
* 功能说明:	时中断使能
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntHourEn(RTC_TypeDef * RTCx)
{	
	RTCx->IE |= (1 << RTC_IE_HOUR_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntHourDis()
* 功能说明:	时中断禁止
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntHourDis(RTC_TypeDef * RTCx)
{	
	RTCx->IE &= ~(1 << RTC_IE_HOUR_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntHourClr()
* 功能说明:	时中断标志清除
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntHourClr(RTC_TypeDef * RTCx)
{	
	RTCx->IF = (1 << RTC_IF_HOUR_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntHourStat()
* 功能说明:	时中断状态
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: uint32_t				1 时中断发生    0 时中断未发生
* 注意事项: 无
******************************************************************************************************************************************/
uint32_t RTC_IntHourStat(RTC_TypeDef * RTCx)
{
	return (RTCx->IF & RTC_IF_HOUR_Msk) ? 1 : 0;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntDateEn()
* 功能说明:	日中断使能
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntDateEn(RTC_TypeDef * RTCx)
{	
	RTCx->IE |= (1 << RTC_IE_DATE_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntDateDis()
* 功能说明:	日中断禁止
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntDateDis(RTC_TypeDef * RTCx)
{	
	RTCx->IE &= ~(1 << RTC_IE_DATE_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntDateClr()
* 功能说明:	日中断标志清除
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntDateClr(RTC_TypeDef * RTCx)
{	
	RTCx->IF = (1 << RTC_IF_DATE_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntDateStat()
* 功能说明:	日中断状态
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: uint32_t				1 日中断发生    0 日中断未发生
* 注意事项: 无
******************************************************************************************************************************************/
uint32_t RTC_IntDateStat(RTC_TypeDef * RTCx)
{
	return (RTCx->IF & RTC_IF_DATE_Msk) ? 1 : 0;
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntAlarmEn()
* 功能说明:	闹钟中断使能
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntAlarmEn(RTC_TypeDef * RTCx)
{	
	RTCx->IE |= (1 << RTC_IE_ALARM_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntAlarmDis()
* 功能说明:	闹钟中断禁止
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntAlarmDis(RTC_TypeDef * RTCx)
{	
	RTCx->IE &= ~(1 << RTC_IE_ALARM_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntAlarmClr()
* 功能说明:	闹钟中断标志清除
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: 无
* 注意事项: 无
******************************************************************************************************************************************/
void RTC_IntAlarmClr(RTC_TypeDef * RTCx)
{	
	RTCx->IF = (1 << RTC_IF_ALARM_Pos);
}

/****************************************************************************************************************************************** 
* 函数名称:	RTC_IntAlarmStat()
* 功能说明:	闹钟中断状态
* 输    入: RTC_TypeDef * RTCx		指定要被设置的RTC，可取值包括RTC
* 输    出: uint32_t				1 闹钟中断发生    0 闹钟中断未发生
* 注意事项: 无
******************************************************************************************************************************************/
uint32_t RTC_IntAlarmStat(RTC_TypeDef * RTCx)
{
	return (RTCx->IF & RTC_IF_ALARM_Msk) ? 1 : 0;
}
