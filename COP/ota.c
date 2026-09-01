/**
  ******************************************************************************
  * @file    ota.c
  * @author 
  * @version V1.0
  * @date
  * @brief   OTA and IAP upgrade implementation.
  ******************************************************************************
  */
//#include "config.h"
//_OTA_Date OTA_Date;
///**
// * @author  niu
// * @date    2026.02.26
// * @version 1.0
// * 
// */
///***************************************************************************************************************************************************************/ 
///**
// * @return  none
// * @note    none
// * 
// */
//void FLASH_ErasePage(uint32_t addr)
//{
//    uint32_t PageError = 0;
//    
//    FLASH_EraseInitTypeDef  FlashEraseInit = 
//    {
//        .TypeErase = FLASH_TYPEERASE_PAGES,
//        .PageAddress = addr,
//        .NbPages = 1,
//    };
//    HAL_FLASHEx_Erase(&FlashEraseInit,&PageError);
//}
///**
// * @param   none      
// * @return  none
// * @note    none
// * 
// */
//void recv_bin_success(void)
//{
//	uint8_t i;
//	uint16_t	flashBUF[2];
//	
//	flashBUF[0] = 0x1a1a;
//	flashBUF[1] = 0x2b2b;
//	HAL_FLASH_Unlock();

//	}
//	HAL_FLASH_Lock();
//}
///**
// * @param   none      
// * @return  none
// * @note    none
// * 
// */
//void download_success(void)
//{
//	recv_bin_success();
//}
///**
// * @param   none      
// * @return  none
// * @note    none
// * 
// */
//void SoftReset(void)
//{  
//}
///**
// * @return  none
// * @note    none
// * 
// */
//void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint32_t appsize)
//{
//	uint16_t t;
//	uint16_t i=0,j;
//  static uint16_t iapbuf[1024];
//  static uint32_t fwaddrShade=FLASH_BAK_ADDR;
//	for(t=0;t<appsize;t+=2)
//	{						    
//				iapbuf[i++]=((uint16_t)appbuf[1]<<8)+(uint16_t)appbuf[0];
//	}
//	{
//				HAL_FLASH_Unlock();
//		    if(((fwaddr-fwaddrShade)==2048)||(fwaddr==FLASH_BAK_ADDR))
//				{
//					  fwaddrShade=fwaddr;
//				    FLASH_ErasePage(fwaddr);
//				}
//				{
//				}
//				HAL_FLASH_Lock();
//	}
//}
///*****************************************************************************************/
///*****************************************************************************************/
//#define FILE_MD5_MAX_LEN  32
//#define SSL_MAX_LEN (FILE_MD5_MAX_LEN/2)
// uint8_t g_ota_sta=0;
// uint32_t g_ota_recv_sum=0;    
// uint16_t g_ota_pg_numid=0;
// uint16_t g_ota_pg_ok_cnt=0;
// uint16_t g_ota_pg_nums=0;
// uint32_t g_ota_bin_size=0;
// uint8_t  g_ota_mcu_end=0;
// uint32_t USART_RX_STA_BAK=0;
// uint8_t g_direct_iap=0;
// uint8_t g_ota_esp_start=0;
// uint16_t g_ota_esp_cnt=0;
// uint8_t md5_pass = 1;
// uint8_t  OtaUpDateFail=0;
// MD5_CTX g_ota_md5_ctx;
// MD5_CTX package_md5_ctx;
// uint8_t g_ota_bin_md5[SSL_MAX_LEN] = {0};
// uint8_t g_ota_package_md5[SSL_MAX_LEN] = {0};
// uint8_t bin_md5_calc[SSL_MAX_LEN];
// uint8_t package_md5_calc[SSL_MAX_LEN] = {0};
//// FE 40 04 + 1B-RESULT + 1B-PackageNum + FF
//// RESULT: 1-pass, 2-fail
//void UART1_ReportOtaPackageSta(uint8_t md5_res)
//{
//	uint8_t buf[8] = {0xFE, 0x40, 0x04, 0x01, 0x01, 0xFF};// Pass

//	if (md5_res != 1) {
//		buf[3] = 2;// Fail
//	}

//	buf[4] = g_ota_pg_numid;

//	HAL_UART_Transmit_DMA(&huart3,buf,6);


//}

//void UART1_ReportOtaBinSta(uint8_t md5_res)
//{
//	uint8_t buf[5] = {0xFE, 0x40, 0x05, 0x02, 0xFF};// Pass

////	if (md5_res != 1) {
////		buf[3] = 2;// Fail
////	}
//  buf[3] = md5_res;
//	HAL_UART_Transmit_DMA(&huart3,buf,5);
//}
//void bin_update(void)
//{
//	  system_data.ota_prcent=g_ota_pg_numid*100/g_ota_pg_nums;
//		if((g_ota_mcu_end == 0) && (g_ota_sta == 3))
//		{
//					{
//						// MD5 for total bin
//						GAgent_MD5Init(&g_ota_md5_ctx);
//						memset(bin_md5_calc, 0, SSL_MAX_LEN);
//					}
//					{  // 0-IDEL, 1-pass, 2-fail, 3-in progress
//						//DisplayString("OTA STOP BY APP",0,0,0,0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					{
//						//DisplayString("OTA TOTALSIZE OVERFLOW",0,0,0,0);
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					{
//						//DisplayString("OTA PGNUM OVERFLOW",0,0,0,0);
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					
//					GAgent_MD5Init(&package_md5_ctx);
//					GAgent_MD5Update(&package_md5_ctx, COM3.rxBuf, (USART_RX_STA_BAK&0xFFFF));
//					
//					{
//						USART_RX_STA_BAK=0;
//						md5_pass = 0;
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						//DisplayString("OTA FAIL",0,0,0,0);
//					} 
//					{
//						md5_pass = 1;
//						g_ota_sta = 3;                  //OTA In process
//					}
//					{
//						 
//						iap_write_appbin(FLASH_BAK_ADDR+g_ota_recv_sum, COM3.rxBuf, (USART_RX_STA_BAK&0xFFFF));
//			 
//						g_ota_recv_sum += USART_RX_STA_BAK&0xFFFF;
//						UART1_ReportOtaPackageSta(1);
//					}
//					USART_RX_STA_BAK=0;
//		}			     
//		{
//				{
//							//DisplayString("OTA 1ERR",0,0,0,0);
//							g_ota_sta = 1;
//							UART1_ReportOtaBinSta(2);// Fail (fe 40 5 2 ff)
//				} 
//				else 
//				{
//							{
//										md5_pass = 0;
//										g_ota_sta = 1;
//										OtaUpDateFail=1;
//										UART1_ReportOtaBinSta(2);// Fail (fe 40 5 2 ff)
//										//DisplayString("OTA 2ERR",0,0,0,0);
//							} 
//							else
//							{
//										md5_pass = 1;
//										g_ota_esp_start=1;
//										g_ota_sta = 2;
//										UART1_ReportOtaBinSta(1);// Pass
//										//delay_ms(1000);
//										g_ota_mcu_end = 0;
//										download_success();// Mark Flag in Flash
//										SoftReset();
//							}
//				}
//	 }
//}
//void OTA_MCU_Task(uint8_t *data)
//{
//	uint8_t snd_len = 0;
//	uint32_t num=0;
//  uint8_t snd_buf[95];
//	if(work_process == idle){return;}
//	if (COM3.rxFlag ==0) {return;}
//	COM3.rxFlag=0;
//	num=COM3.rxLen;
//	snd_len = num;
//	USART_RX_STA_BAK=num;
//	if((data[0] == 0xfe)&&(data[1] == 0x40)&&(data[num-1]==0xFF)&&(num<=30)){
//				switch(data[2])
//				{
//					// FE 40 01 01 FF
//					case 0x01:
//						switch(data[3])
//						{
//							case 0x01:// OTA Start

//								if (g_direct_iap >= 2) {
//									g_direct_iap = 3;
//								} else {
//									g_direct_iap = 1;
//								}
//								work_process=ota;system_data.ota_process=0;								
//								g_ota_sta = 3;
//								g_ota_mcu_end = 0;
//								g_ota_recv_sum = 0;
//								g_ota_pg_numid = 0;
//								g_ota_pg_nums = 0;
//								g_ota_bin_size = 0;
//								g_ota_pg_ok_cnt = 0;
//								memcpy(snd_buf, data, num);
//								break;
//							case 0x02://OTA End
//								g_ota_mcu_end = 1;
//								memcpy(snd_buf, data, num);
//								break;
//							default:
//								//g_ota_mcu_end = 0;	
//								snd_len = 0;
//							break;
//						}
//						break;
//					// FE 40 02 + 3B-Size + 1B-TotalPackagesNum + MD5 + FF
//					case 0x02:// Total Packages & MD5
//						g_ota_pg_nums = data[6];
//						g_ota_bin_size = (data[3]<<16) + (data[4]<<8) + data[5];
//						memcpy(g_ota_bin_md5, data+7, SSL_MAX_LEN);
//						memcpy(snd_buf, data, num);
//						break;
//					// After Recved 03 ACK, APP will send the package data
//					case 0x03:// Divided Package & MD5    FE 40 03 + 1B-PackageNum + MD5 + FF
//						g_ota_pg_numid = data[3];
////					if(g_ota_pg_numid==2)
////					OTA_Date.PackageDateReceiveFlash=0;	
//						memcpy(g_ota_package_md5, data+4, SSL_MAX_LEN);
//						g_ota_esp_cnt	= (uint16_t)data[20]; //UpdatePercent=(u16)data[20];
//						OTA_Date.PackageDateReceiveReady=1;
//						memcpy(snd_buf, data, num);
//						break;
//					
//					case 0x06:// Query OTA Sta   FE 40 06 01 FF
//						memcpy(snd_buf, data, num);
//						snd_buf[3] = g_ota_sta;// 0-idle, 1-pass, 2-fail, 3-in progress
//						break;
//					
//					case 0x07:// Force to Stop OTA   FE 40 07 01 FF
//						g_ota_sta = 2;
//						if (g_direct_iap >= 2) {
//							g_direct_iap = 4;
//						}
//						g_ota_recv_sum = 0;
//						g_ota_pg_numid = 0;
//						g_ota_pg_nums = 0;
//						g_ota_bin_size = 0;
//						g_ota_pg_ok_cnt = 0;
//						memcpy(snd_buf, data, num);
//						break;
//					default:
//						snd_len = 0;
//						break;
//				}
//																													
//		snd_len = 0;data[1] =0;data[num-1]=0;COM3.rxLen=0;

//			OTA_Date.PackageDateReceiveReady=0;COM3.rxLen=0;
//			bin_update();
//		}
//	else if((work_process==ota)&&(system_data.ota_process==0)){COM3.rxLen=0;}
//}

//void OTA_screen_Task(uint8_t *data)
//{
//		if(work_process== idle){return;}
//		if(COM3.rxFlag==0) {return;}
//		COM3.rxFlag=0;
//		{
//				work_process=ota;system_data.ota_process=1;
//			 
//				HAL_UART_Transmit(&huart3, data, COM3.rxLen, 1000);
//	 
//				data[1] = 0x00;COM3.rxLen=0;
//		}
//		{
//				 COM3.rxFlag=0;
//				if((COM3.rxBuf[0] == 0xfe)&&(COM3.rxBuf[1] == 0x50)&&(COM3.rxBuf[COM3.rxLen-1]==0xFF)&&(COM3.rxLen<=30))
//				{
//					   HAL_UART_Transmit_DMA(&huart3,COM3.rxBuf,COM3.rxLen);
//						 COM3.rxBuf[0]=0;
//						 COM3.rxLen=0;
//				}
//		}
//		//if((TouchScreenState==ota_mode)&&(SystemData.ota_process==1)){COM5.RX_Cnt=0;}
//}
//void OTA_ESP32(uint8_t *data)
//{
// if(COM3.rxFlag==0) {return;}
// COM3.rxFlag=0;
// COM3.rxLen=0;
// if(data[1]!=0x34) return;
// if(data[2]!=0XFF) {system_data.ota_prcent = data[2];}
//}


//void ota_control(void)
//{
//     OTA_MCU_Task(COM3.rxBuf);	   
//	   OTA_screen_Task(COM3.rxBuf);
//	   OTA_ESP32(COM3.rxBuf);	
//}






