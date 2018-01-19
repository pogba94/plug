#ifndef _USERCONFIG_H
#define _USERCONFIG_H

/*-------Includes-----------------------------------------------------*/
#include "mbed.h"
#include "EthernetInterface.h"
#include "flashLayout.h"

/*-------Function Configure-------------------------------------------*/
#define   FuncCfg_WatchDog         		(0)                           
#define   FuncCfg_Led              		(1)  
#define   FuncCfg_Ethernet         		(1)
#define   FuncCfg_DebugImfo           (1)
#define   DEBUG_MODE                  (0)
/*-------Parameter Configure------------------------------------------*/
#define    RESET_TIMER                 120
#define    USED_PORT_NUM               16
#define    MAX_PORT_NUM                16   
#define    HEARTBEAT_PERIOD            30
#define    CHECK_TIME_OVER_PERIOD      5
#define    MSG_RESEND_PERIOD           5
#define    SOCKET_IN_BUFFER_SIZE       (4096+256)
#define    SOCKET_OUT_BUFFER_SIZE      (256)
#define    RING_BUFF_SIZE              (16)
#define    MAX_CHARGING_TIME           (480)    //Unit:min
/* OTA pack definition */
#define OTA_BLOCKOFFSET_POS 6
#define OTA_BLOCKSIZE_POS   10
#define OTA_CHECKSUM_POS    12
#define OTA_BINDATA_POS     14
#define OTA_MAX_PACK_SIZE   (1024)     //OTA pack size

#define OTA_REWRITE_TIMES              (5)
#define DEFAULT_VERSION_SN             "34c18825699a9ca2f1ae3bc4257ade5f"
#define DEFAULT_CHARGING_TIME                   (60)
/*-------Macro Definition---------------------------------------------*/
#if FuncCfg_Led
#define    SERVER_CONNECT_LED_ON     (server = 1)
#define    SERVER_CONNECT_LED_OFF    (server = 0)
#endif

/*-------Variable declaration-----------------------------------------*/

extern Serial pc;
extern DigitalOut relay0;
extern DigitalOut relay1;
extern DigitalOut relay2;
extern DigitalOut relay3;
extern DigitalOut relay4;
extern DigitalOut relay5;
extern DigitalOut relay6;
extern DigitalOut relay7;
extern DigitalOut relay8;
extern DigitalOut relay9;
extern DigitalOut relay10;
extern DigitalOut relay11;
extern DigitalOut relay12;
extern DigitalOut relay13;
extern DigitalOut relay14;
extern DigitalOut relay15;
extern TCPSocketConnection tcpsocket;

extern char tempBuffer[256];

/*--------Prototypes--------------------------------------------------*/

#endif
