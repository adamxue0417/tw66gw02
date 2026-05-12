//#include "config.h"
//_OTA_Date OTA_Date;
///**
// * @file    ota.c
// * @brief   ota升级文件
// * @author  niu
// * @date    2026.02.26
// * @version 1.0
// * 
// * 详细描述:该文件实现了ota升级的一些功能函数，用于设备的在线升级固件。
// */
///***************************************************************************************************************************************************************/ 
///**
// * @brief   FLASH_ErasePage函数功能简述
// * @param   addr 要擦除的地址      
// * @return  none
// * @note    none
// * 
// * 详细说明：此函数为设备内存FLASH页擦除函数。
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
// * @brief   recv_bin_success函数功能简述
// * @param   none      
// * @return  none
// * @note    none
// * 
// * 详细说明：此函数为设备数据接收成功函数。
// */
//void recv_bin_success(void)
//{
//	uint8_t i;
//	uint16_t	flashBUF[2];
//	
//	flashBUF[0] = 0x1a1a;
//	flashBUF[1] = 0x2b2b;
//	HAL_FLASH_Unlock();

//	FLASH_ErasePage(UPGRADEaddr1);// 先擦后写
//	for (i=0;i<2;i++) {// u8变成u16后的字节数据					   
//		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,(UPGRADEaddr1+i*2),flashBUF[i]);// flash  为一个字节存储，16位数据必须地址加2
//	}
//	HAL_FLASH_Lock();
//}
///**
// * @brief   download_success函数功能简述
// * @param   none      
// * @return  none
// * @note    none
// * 
// * 详细说明：此函数为设备下载成功函数。
// */
//void download_success(void)
//{
//	__disable_irq();   // 关闭总中断
//	recv_bin_success();
//	__enable_irq();    // 开启总中断
//}
///**
// * @brief   SoftReset函数功能简述
// * @param   none      
// * @return  none
// * @note    none
// * 
// * 详细说明：此函数为软件复位函数。
// */
//void SoftReset(void)
//{  
//	__disable_irq();   // 关闭总中断
// 	NVIC_SystemReset();//复位
//}
///**
// * @brief   iap_write_appbin函数功能简述
// * @param   appxaddr：要写入更新的地址  appbuf：要写入的数据    appsize：要写入数据的大小   
// * @return  none
// * @note    none
// * 
// * 详细说明：此函数为数据写入flash更新ota数据函数。
// */
//void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint32_t appsize)
//{
//	uint16_t t;
//	uint16_t i=0,j;
//	uint32_t fwaddr=appxaddr;//当前写入的地址
//  static uint16_t iapbuf[1024];
//  static uint32_t fwaddrShade=FLASH_BAK_ADDR;
//	__disable_irq();   // 关闭总中断
//	for(t=0;t<appsize;t+=2)
//	{						    
//				iapbuf[i++]=((uint16_t)appbuf[1]<<8)+(uint16_t)appbuf[0];
//				appbuf+=2;//偏移2个字节		
//	}
//	if(i)//小于1024字节填入
//	{
//				HAL_FLASH_Unlock();
//		    if(((fwaddr-fwaddrShade)==2048)||(fwaddr==FLASH_BAK_ADDR))
//				{
//					  fwaddrShade=fwaddr;
//				    FLASH_ErasePage(fwaddr);
//				}
//				for(j=0;j<i;j++)											//u8变成u16后的字节数据					   
//				{
//					HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,(fwaddr+j*2),iapbuf[j]);//flash  为一个字节存储，16位数据必须地址加2
//				}
//				HAL_FLASH_Lock();
//	}
//	__enable_irq();    // 开启总中断
//}
///*****************************************************************************************/
///************************ OTA更新任务*****************************************************/
///*****************************************************************************************/
////ota任务
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
//					if (1 == g_ota_pg_numid) //初始化bin_md5_calc
//					{
//						// MD5 for total bin
//						GAgent_MD5Init(&g_ota_md5_ctx);
//						memset(bin_md5_calc, 0, SSL_MAX_LEN);
//					}
//					if (0 == g_ota_sta)      //被中止  MD5 for single package
//					{  // 0-IDEL, 1-pass, 2-fail, 3-in progress
//						//DisplayString("OTA STOP BY APP",0,0,0,0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					if ((g_ota_recv_sum+USART_RX_STA_BAK&0xFFFF) > g_ota_bin_size) //字节数大于bin总包字节数
//					{
//						//DisplayString("OTA TOTALSIZE OVERFLOW",0,0,0,0);
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					if (g_ota_pg_numid > g_ota_pg_nums)         //包数溢出
//					{
//						//DisplayString("OTA PGNUM OVERFLOW",0,0,0,0);
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						USART_RX_STA_BAK=0;
//						return;
//					}
//					
//					//计算MD5
//					GAgent_MD5Init(&package_md5_ctx);
//					GAgent_MD5Update(&package_md5_ctx, COM3.rxBuf, (USART_RX_STA_BAK&0xFFFF));
//					GAgent_MD5Final(&package_md5_ctx, package_md5_calc);//计算单包MD5
//					
//					if(memcmp(package_md5_calc, g_ota_package_md5, SSL_MAX_LEN) != 0) //对比单包MD5
//					{
//						USART_RX_STA_BAK=0;
//						md5_pass = 0;
//						g_ota_sta = 2;                  //OTA FAIL
//						UART1_ReportOtaPackageSta(0);
//						//DisplayString("OTA FAIL",0,0,0,0);
//					} 
//					else                                                              //通过MD5
//					{
//						md5_pass = 1;
//						g_ota_sta = 3;                  //OTA In process
//					}
//					GAgent_MD5Update(&g_ota_md5_ctx, COM3.rxBuf, (USART_RX_STA_BAK&0xFFFF));//计算总包MD5   MD5 for total bin
//					if (md5_pass)//更新第n包数据 
//					{
//						 
//						iap_write_appbin(FLASH_BAK_ADDR+g_ota_recv_sum, COM3.rxBuf, (USART_RX_STA_BAK&0xFFFF));
//			 
//						g_ota_recv_sum += USART_RX_STA_BAK&0xFFFF;
//						UART1_ReportOtaPackageSta(1);
//					}
//					USART_RX_STA_BAK=0;
//		}			     
//    //最后整包验证
//		if(g_ota_pg_nums == g_ota_pg_numid) //包数量相等
//		{
//				if (g_ota_recv_sum != g_ota_bin_size) //字节数量相等？
//				{
//							//DisplayString("OTA 1ERR",0,0,0,0);
//							g_ota_sta = 1;
//							UART1_ReportOtaBinSta(2);// Fail (fe 40 5 2 ff)
//				} 
//				else 
//				{
//							GAgent_MD5Final(&g_ota_md5_ctx, bin_md5_calc); //关闭测试0AT升级失败，正常开启    MD5 for total bin
//							if (memcmp(bin_md5_calc, g_ota_bin_md5, SSL_MAX_LEN) != 0) //对比计算总包MD5
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
//	//开头是FE 40,结尾是FF,总长度小于30字节，才算OTA指令，防止和OTA的固件重复
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
//		if ((snd_len != 0)&&(g_ota_bin_size<112640)) {HAL_UART_Transmit_DMA(&huart3,snd_buf,snd_len);}//不能超过110K的更新最大容量，否则不回复
//		snd_len = 0;data[1] =0;data[num-1]=0;COM3.rxLen=0;

//	}else if(OTA_Date.PackageDateReceiveReady){//固件,接收到单个固件的MD5后，下一包数据才认为是固件
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
//		//开头是FE 50,结尾是FF,总长度小于30字节，才算OTA指令，防止和OTA的固件重复
//		if(((data[0] == 0xfe)&&(data[1] == 0x50)&&(data[COM3.rxLen-1]==0xFF)&&(COM3.rxLen<=30))||((COM3.rxLen==4096)))//来自ESP32的数据转达给触摸屏   
//		{
//				work_process=ota;system_data.ota_process=1;
//			 
//				HAL_UART_Transmit(&huart3, data, COM3.rxLen, 1000);
//	 
//				data[1] = 0x00;COM3.rxLen=0;
//		}
//		if((COM3.rxFlag)&&(work_process==ota)&&(system_data.ota_process))//来自触摸屏的数据转达给ESP32
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
//	   fan_turn_on();        /*风扇开启*/
//	   motor_turn_off();     /*马达关闭*/	
//	   heat_control(1,off);  /*加热棒关闭*/
//	   SystemErrState=NoErr; /*清除错误*/	
//}






