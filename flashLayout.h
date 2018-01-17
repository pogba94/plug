#ifndef FLASHLAYOUT_H
#define FLASHLAYOUT_H
#include "FreescaleIAP.h"
#include "UserConfig.h"
/*******************************************************
    Flash Layout
    Bootloader(32K)     0x00000~0x07FFF
    Application(128K)   0x08000~0x27FFF
    OTA (128K)          0x28000~0x47FFF
    Checksum (4B)       0x48000~0x48003   ota checksum & code checksum
    Version Id(2B)      0x48004^0x48005   ota version id & code version id
    Param (4K)          0x7F000~0x7FFFF
		ServerIP (4K)				0x7E000~0x7F000    IP1|IP2|IP2|IP3|PORT1|PORT2|CHECKSUM1|CHECKSUM2 Total 8 bytes
********************************************************/

#define BOOTLOADER_START_ADDRESS    0
#define BOOTLOADER_SIZE             0x8000   // 32K  
#define CODE_SECTOR_NUM             32   // 32*4K Bytes    8*4K for bootloader total 160KB
#define MIN_CODE_SECTOR_NUM         16 // 16*4K Bytes minimal code size
#define CODE_START_ADDRESS          (BOOTLOADER_SIZE)//0
#define CODE_SIZE                   (CODE_SECTOR_NUM * SECTOR_SIZE)
#define MAX_CODE_SIZE                CODE_SIZE
#define MIN_CODE_SIZE                (16*SECTOR_SIZE)
#define OTA_CODE_START_ADDRESS      (CODE_START_ADDRESS + CODE_SIZE)

#define VERSION_STR_ADDRESS         (OTA_CODE_START_ADDRESS + CODE_SIZE)
#define VERSION_STR_LEN             40 //total 40 bytes,2 bytes for code checksum,2 bytes for OTA checksum,36 bytes for version SN
#define VERSION_CHECKSUM_LEN        4  

#define PARAM_START_ADDRESS         (0x80000-0x1000)   //  last 4k flash for store param config,in mk64fx512
#define SERVER_IP_ADDRESS           (PARAM_START_ADDRESS - 0x1000)

#endif
