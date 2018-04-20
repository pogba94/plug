#ifndef FLASHLAYOUT_H
#define FLASHLAYOUT_H
#include "FreescaleIAP.h"
/*******************************************************
    Flash Layout
    bootloader(32K)     0x00000~0x07FFF
    application(128K)   0x08000~0x27FFF
    OTA (128K)          0x28000~0x47FFF
    Checksum (4B)       0x48000~0x48003   code checksum & ota code checksum: xx xx xx xx 
    OTA CODE VER(60B)   0x48004~0x4803F
    Param (4K)          0xFF000~0xFFFFF
********************************************************/
#define BOOTLOADER_START_ADDRESS    0
#define BOOTLOADER_SIZE             0x8000   // 32K  
#define CODE_SECTOR_NUM             32   // 32*4K Bytes    8*4K for bootloader total 160KB
#define MIN_CODE_SECTOR_NUM         16 // 16*4K Bytes minimal code size,  code size always > 90 KB
#define CODE_START_ADDRESS          (BOOTLOADER_SIZE)//0
#define CODE_SIZE                   (CODE_SECTOR_NUM * SECTOR_SIZE)
#define OTA_CODE_START_ADDRESS      (CODE_START_ADDRESS + CODE_SIZE)

#define VERSION_STR_ADDRESS         (OTA_CODE_START_ADDRESS + CODE_SIZE)
#define VERSION_STR_LEN             64  // version string 32 bytes  For ex: "WPI-DEMO VERSION 1.0"

#define PARAM_START_ADDRESS (0x100000-0x1000)   //  last 4k flash for store param config
#define CHARGER_INFO_ADDRESS PARAM_START_ADDRESS
#endif
