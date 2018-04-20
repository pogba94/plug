/*------------------------------------------------------------------------------
 * Description:
 *	
-------------------------------------------------------------------------------*/

/*---------Includes------------------------------------------------------------*/
#include "flashLayout.h"
#include "UserConfig.h"
#include "lib_crc16.h"
#include "ota.h"

/*--------Prototypes-----------------------------------------------------------*/

void updateCode(void);

/*--------Function Definition---------------------------------------------------*/

/*!
 * @brife OTA Initialization
 * @input Null
 * @return Null
*/
void initOTA(void)
{
    char* cdata = (char*)VERSION_STR_ADDRESS;
    char* codePartition = (char*) CODE_START_ADDRESS;
    char* OTACodePartition = (char*) OTA_CODE_START_ADDRESS;
    int i = 0;
    int codecrc16,otacodecrc16,codechecksum;

    codecrc16 = calculate_crc16(codePartition, CODE_SIZE);
    codechecksum = (cdata[0]<<8)|cdata[1];

    if((codechecksum==0xFFFF)||(codechecksum==0x0)||(codecrc16 != codechecksum)) {
			#if FuncCfg_DebugImfo
        pc.printf("OTA version unavaible, Inialize OTA partition!\r\n");
			#endif
        for(i=0; i<CODE_SECTOR_NUM; i++) {
            erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
            program_flash(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,codePartition+i*SECTOR_SIZE, SECTOR_SIZE);
        }
        otacodecrc16 = calculate_crc16(OTACodePartition, CODE_SIZE);
        if(otacodecrc16 == codecrc16) {
            memset(tempBuffer,0x0,VERSION_STR_LEN);
            tempBuffer[0] = tempBuffer[2] = (codecrc16 >> 8) & 0xff;
            tempBuffer[1] = tempBuffer[3] = codecrc16 & 0xff;
					  memcpy(tempBuffer+4,cdata+4,33);  //version SN
            erase_sector(VERSION_STR_ADDRESS);
            program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
					  #if FuncCfg_DebugImfo
						pc.printf("Initialize OTA partition successfully\r\n");
						#endif
        }else{
					#if FuncCfg_DebugImfo
					pc.printf("Error! Fail to initialize OTA partition! Reset system now!\r\n");
					#endif
					NVIC_SystemReset();
				}
    } else {
				#if FuncCfg_DebugImfo
				pc.printf("Code checksum: %2X%2X,OTA checksum:%2X%2X\r\n",
									cdata[0],cdata[1],cdata[2],cdata[3]);
        #endif
			  if(cdata[0]!= cdata[2] || cdata[1] != cdata[3]){ // if version change, update code flash
        #if FuncCfg_DebufImfo   
					pc.printf("Start to update firmware...\r\n");
        #endif 
					updateCode();
        } else{
					#if FuncCfg_DebufImfo
            pc.printf("The firmware is already the newest!\r\n");
          #endif
				}
    }
}
/*!
 * @brife Reset system ,the function will be call when the code need to update
 * @input Null
 * @return Null
*/
void updateCode(void)
{
    pc.printf("Update, system will be reset now, please wait...\r\n");
    NVIC_SystemReset();
}
