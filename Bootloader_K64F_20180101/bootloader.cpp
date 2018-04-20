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

#define APPLICATION_START_SECTOR    8
#define APPLICATION_END_SECTOR      87

/*    
    bootloader(32K)     0x00000~0x07FFF
    application(128K)   0x08000~0x27FFF
    OTA (128K)          0x28000~0x47FFF
    Checksum (4B)       0x48000~0x48003   code checksum & ota code checksum: xx xx xx xx 
    Version SN(36B)     0x48004~0x48025   version SN 32 bytes hash string
    Param (4K)          0x7E000~0x7FFFF
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
  // if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    { /* Jump to user application */
      JumpAddress = *(volatile unsigned int*) (ApplicationAddress+4);
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
	
    write((char*)"\r\n******* MK64 Bootloader V1.0(For shared plug) ******\r\n");
    checkResetSrc();
	
		codecrc16 = calculate_crc16(codePartition, CODE_SIZE);
		otacodecrc16 = calculate_crc16(OTACodePartition, CODE_SIZE);
	  codechecksum = (cdata[0]<<8)|cdata[1];
		otacodechecksum = (cdata[2]<<8)|cdata[3];
	
		if(codechecksum == 0x0 || codechecksum == 0xFFFF) // first time boot, write code to ota partition by app
		{
			write((char*)"First boot...\r\n");
		}
		else 
		{	
			if(otacodecrc16 == otacodechecksum)
			{
				if((codechecksum != otacodechecksum) ||(codecrc16 != codechecksum)) // update;
				{
					write((char*)"update...\r\n");
				
					for(i=0; i<CODE_SECTOR_NUM; i++) 
					{
							erase_sector(CODE_START_ADDRESS+i*SECTOR_SIZE);
							program_flash(CODE_START_ADDRESS+i*SECTOR_SIZE,OTACodePartition+i*SECTOR_SIZE, SECTOR_SIZE);
					}
					write((char*)"Code done!\r\n");
					codecrc16 = calculate_crc16(codePartition, CODE_SIZE);
					if(codecrc16 != otacodecrc16)
					{
						write((char*)"Checksum error,Restart...\r\n·");
						NVIC_SystemReset();
					}
					tempBuffer[0] = tempBuffer[2] = cdata[2];
					tempBuffer[1] = tempBuffer[3] = cdata[3];
					
					erase_sector(VERSION_STR_ADDRESS);
					program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
					write((char*)"Completed!\r\n");
				}
				else
				{
					write((char*)"Flash Code Check OK!\r\n");
				} 
			}
			else
			{
				if(codecrc16 == codechecksum) // update ota;
				{
					write((char*)"Update OTA partition!...\r\n");
					for(i=0; i<CODE_SECTOR_NUM; i++) 
					{
						erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
						program_flash(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE,codePartition+i*SECTOR_SIZE, SECTOR_SIZE);
					}
					otacodecrc16 = calculate_crc16(OTACodePartition, CODE_SIZE);
					if(codecrc16 != otacodecrc16)
					{
						write((char*)"OTA checksum error,Restart...\r\n");
						NVIC_SystemReset();
					}
					tempBuffer[0] = tempBuffer[2] = cdata[0];
					tempBuffer[1] = tempBuffer[3] = cdata[1];
	
					erase_sector(VERSION_STR_ADDRESS);
					program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
					write((char*)"OTA completed!\r\n");
				}
				else  // code checksum and ota checksum error
				{
					write((char*)"Code and OTA partition was destroyed! Cannot recover!...\r\n");
					for(;;);
				}
			}
		}
    write((char*)"Jump to user code!\r\n");
    Jump_IAP();
		write((char*)"Done erasing, send file!\r\n");//
}



static void setupserial(void) 
{
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
  while(*(value+i) != '\0') {
   while(!(UART0->S1 & UART_S1_TDRE_MASK)){}
     UART0->D = *(value+i);
     i++;
   }
}

static void checkResetSrc(void)
{
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmWakeup))
			write((char*)"low-leakage wakeup reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLowVoltDetect))
			write((char*)"low voltage detect reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLossOfClk))
			write((char*)"loss of clock reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmLossOfLock))
			write((char*)"loss of lock reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmWatchDog))
			write((char*)"watch dog reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmExternalPin))
			write((char*)"external pin reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmPowerOn))
			write((char*)"power on reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmJtag))
			write((char*)"JTAG generated reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmCoreLockup))
			write((char*)"core lockup reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmSoftware))
			write((char*)"software reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmSystem))
			write((char*)"Esystem reset request bit set reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmEzport))
			write((char*)"zPort reset\r\n");
	if(RCM_HAL_GetSrcStatusCmd(RCM_BASE,kRcmStopModeAckErr))
			write((char*)"stop mode ack error reset\r\n");
}