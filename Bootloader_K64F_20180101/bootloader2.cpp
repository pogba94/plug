#include "mbed.h"
#include "FreescaleIAP.h"
#include "flashLayout.h"
#include "lib_crc16.h"
extern "C"{
#include "fsl_rcm_hal.h"
}
//Could be nicer, but for now just erase all preceding sectors
#define NUM_SECTORS        15
#define TIMEOUT            10000000
#define BUFFER_SIZE        16

#define APPLICATION_START_SECTOR 8
#define APPLICATION_END_SECTOR 87

/*    
    bootloader(32K)     0x00000~0x07FFF
    application(320K)   0x08000~0x57FFF
    OTA (320K)          0x58000~0xA7FFF
    Checksum (4B)       0xA8000~0xA8003   code checksum & ota code checksum: xx xx xx xx 
    OTA CODE VER(60B)   0xA8004~0xA803F
    Param (4K)          0xFF000~0xFFFFF
*/
 
#define ApplicationAddress    CODE_START_ADDRESS//0x8000

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
unsigned int JumpAddress;
extern void __set_MSP(unsigned int topOfMainStack);

void Jump_IAP(void)
{
  //NVIC_SETFAULTMASK();  //关总中断
    //    NVIC_DisableIRQ;
    /* Test if user code is programmed starting from address "ApplicationAddress" */
//    if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    { /* Jump to user application */
      JumpAddress = *(volatile unsigned int*) (ApplicationAddress + 4);
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(volatile unsigned int*) ApplicationAddress);
      Jump_To_Application();
    }
}

static void setupserial();
static void write(char *value);
static void checkResetSrc(void);
char tempBuffer[128];

void bootloader(void)
{
		char* cdata = (char*)VERSION_STR_ADDRESS;
		char* codePartition = (char*) CODE_START_ADDRESS;
		char* OTACodePartition = (char*) OTA_CODE_START_ADDRESS;
		int i = 0;
		int codecrc16,otacodecrc16,codechecksum,otacodechecksum;
	
    setupserial();
	
    write("\r\n*******Bootloader v1.03@20180101**********\r\n");
    checkResetSrc();
	
		codecrc16 = calculate_crc16(codePartition, CODE_SIZE);
		otacodecrc16 = calculate_crc16(OTACodePartition, CODE_SIZE);
	  codechecksum = (cdata[0]<<8)|cdata[1];
		otacodechecksum = (cdata[2]<<8)|cdata[3];
	
		if(strncmp("WPI",cdata+4,3) != NULL) // first time boot, write code to ota partition by app
		{
			write("first boot...\r\n");
		}
		else 
		{	
			if((otacodecrc16 == otacodechecksum))
			{	
				if((codechecksum != otacodechecksum) ||(codecrc16 != codechecksum)) // update;
				{
					write("update...\r\n");
				
					for(i=0; i<CODE_SECTOR_NUM; i++) 
					{
							erase_sector(CODE_START_ADDRESS+i*SECTOR_SIZE);
							program_flash(CODE_START_ADDRESS+i*SECTOR_SIZE,OTACodePartition+i*SECTOR_SIZE, SECTOR_SIZE);
					}
					write("code done!\r\n");
					codecrc16 = calculate_crc16(codePartition, CODE_SIZE);
					if(codecrc16 != otacodecrc16)
					{
						write("checksum error,restart...\r\n·");
						NVIC_SystemReset();
					}
					tempBuffer[0] = tempBuffer[2]= cdata[2];
					tempBuffer[1] = tempBuffer[3]= cdata[3];
	
					for(i=4;i<VERSION_STR_LEN;i++)
						tempBuffer[i] = cdata[i];
					erase_sector(VERSION_STR_ADDRESS);
					program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
					write("completed!\r\n");
				}
				else
				{
					write("Flash Code Check OK!\r\n");
				} 
			}
			else
			{
				if((codecrc16 == codechecksum)) // update ota;
				{
					write("update ota...\r\n");
					for(i=0; i<CODE_SECTOR_NUM; i++) 
					{
						erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
						program_flash(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,codePartition+i*SECTOR_SIZE, SECTOR_SIZE);
					}
					
					otacodecrc16 = calculate_crc16(OTACodePartition, CODE_SIZE);
					if(codecrc16 != otacodecrc16)
					{
						write("ota checksum error,restart...\r\n");
						NVIC_SystemReset();
					}
					tempBuffer[0] = tempBuffer[2]= cdata[0];
					tempBuffer[1] = tempBuffer[3]= cdata[1];
	
					for(i=4;i<VERSION_STR_LEN;i++)
						tempBuffer[i] = cdata[i];
					erase_sector(VERSION_STR_ADDRESS);
					program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
					write("ota completed!\r\n");
				}
				else  // code checksum and ota checksum error
				{
					write("otherwise...\r\n");
				}
			}
		} 
    Jump_IAP();
    write("Done erasing, send file!\r\n");
    
}
    

static void setupserial(void) {
        //Setup USBTX/USBRX pins (PTB16/PTB17)
        SIM->SCGC5 |= 1 << SIM_SCGC5_PORTB_SHIFT;
        PORTB->PCR[16] = (PORTB->PCR[16] & 0x700) | (3 << 8);
        PORTB->PCR[17] = (PORTB->PCR[17] & 0x700) | (3 << 8);

        //Setup UART (ugly, copied resulting values from mbed serial setup)
        SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;

       // UART0->BDH = 3;
       // UART0->BDL = 13;
			 UART0->BDH = 0;
       UART0->BDL = 0x41;
        UART0->C4 = 8;
        UART0->C2 = 12;  //Enables UART
    }

static void write(char *value)
{
        int i = 0;
        //Loop through string and send everything
        while(*(value+i) != '\0') {
            while(!(UART0->S1 & UART_S1_TDRE_MASK));
            UART0->D = *(value+i);
            i++;
        }
}

static void checkResetSrc(void)
{
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmWakeup))
			write("low-leakage wakeup reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLowVoltDetect))
			write("low voltage detect reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLossOfClk))
			write("loss of clock reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLossOfLock))
			write("loss of lock reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmWatchDog))
			write("watch dog reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmExternalPin))
			write("external pin reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmPowerOn))
			write("power on reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmJtag))
			write("JTAG generated reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmCoreLockup))
			write("core lockup reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmSoftware))
			write("software reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmSystem))
			write("Esystem reset request bit set reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmEzport))
			write("zPort reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmStopModeAckErr))
			write("stop mode ack error reset\r\n");
}