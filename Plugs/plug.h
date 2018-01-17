#ifndef  _PLUG_H
#define  _PLUG_H

/*-----------Incluedes--------------------------------------------------*/
#include "UserConfig.h"
#include "ringBuffer.h"
/*-----------Type Definition------------------------------------------*/

typedef enum _respondCode{
	RESP_DEFAULT = 0,
	RESP_OK = 100,
	RESP_PARAM_ERROR = 101,
	RESP_ILLEGAL_REQUEST = 102,
	RESP_ITEM_ERROR = 103,
	RESP_COMMAND_ERROR = 104,
}RespondCode_t;

typedef enum _portStatus{
	DEFAULT = -1,
	OFF = 1,
	ON = 0,
	FAULT = 2,
}PortStatus_t;

typedef enum _deviceStatus{
	OFFLINE = 0,
	ONLINE = 1,
	UPDATING = 10,
	OTA_FAIL = 11,
	DEVICE_FAULT = 20,
}DeviceStatus_t;

typedef enum _cmdId{
	CONNECT_REQUEST_PUSH = 1,
	HEARTBEAT_PUSH = 2,
	STATUS_CHANGE_PUSH = 3,
	UPDATE_REQUEST_PUSH = 4,
	MAX_REQ_ID = 10,
	PORT_ON_RECV = 21,
	PORT_OFF_RECV = 22,
	GET_PORT_STATUS_RECV = 23,
	UPDATE_RECV = 24,
	MULTI_PORTS_ON_OFF_RECV = 51,
}CmdId_t;

typedef enum _msgId{
	invalidId = 0,
	reqConnect,
	reqHeartbeat,
	reqNotifyDeviceStatus,
	reqNotifyPortStatus,
	reqOTA,
	cmdSetPortOn,
 	cmdMultiPortOnOff,
	cmdSetPortOff,
	cmdGetPortStatus,
	cmdUpdateVersion,
	unknownMsgId,
}MsgId_t;

typedef struct _plug{
	char* mac;
	char* versionSN;
	PortStatus_t portStatus[USED_PORT_NUM];
	uint32_t duration[USED_PORT_NUM];  //Unit:second
	uint32_t setDuration[USED_PORT_NUM]; //Unit:second
	uint8_t deviceStatus;
	RingBuff_t portStatusChange;  //record which port's status changed,need to notify to server
	bool serverConnectFlag;
	bool isChargingFlag;
  uint16_t connCnt;
}Plug_t;

typedef struct _eventHandle{
	bool onlineRequestFlag;
	bool deviceStatusUpdateFlag;
	bool portStatusUpdateFlag;
	bool updateRequestFlag;
	bool sendHeartbeatFlag;
	bool checkTimeOverFlag;
}EventHandle_t;

typedef struct _port_t{
	uint8_t index;
	PortStatus_t statu;
}Port_t;

typedef struct _msgHandle{
	MsgId_t recvMsgId;
  MsgId_t sendMsgId;
	RespondCode_t respCode;
	uint8_t portStatusNotifyDone;
	Port_t respBuf;
	Port_t sendBuf;	
  char msgIdStr[10];
}MsgHandle_t;

typedef struct _socketInfo {
  char inBuffer[SOCKET_IN_BUFFER_SIZE];
  char outBuffer[SOCKET_OUT_BUFFER_SIZE];
} SocketInfo_t;

typedef struct _OTAInfo{
	char versionSN[36];  //32 Bytes versionSN
	int curSector;// 0-32
	int sectorNum;
	int lastSectorSize;   //Unit:Bytes
	int totalSize;  //Unit:Bytes
	int checkSum;
}OTAInfo_t;
/*-----------Macro Definition----------------------------------------------*/
#define SOCKET_OTA_HEADER  "OTABIN"
/*
 *        JSON pack definition
*/

//#define  PACK_REQ_CONNECT       "{\"apiId\":%d,\"versionId\":%d,\"mac\":\"%s\"}"
#define  PACK_REQ_CONNECT       "{\"apiId\":%d,\"versionSN\":\"%s\",\"mac\":\"%s\",\"reconnect\":%d,\"portStatus\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}"
#define  PACK_REQ_HEARTBEAT     "{\"apiId\":%d}"
#define  PACK_REQ_NOTIFY_DEVICE_STATUS "{\"apiId\":%d,\"deviceStatus\":%d}"
#define  PACK_REQ_NOTIFY_PORT_STATUS   "{\"apiId\":%d,\"portStatus\":[%d,%d]}"
#define  PACK_REQ_NOTIFY_DEVICE_PORT_STATUS  "{\"apiId\":%d,\"deviceStatus\":%d,\"portStatus\":[%d,%d]}"
#define  PACK_REQ_UPDATE        "{\"apiId\":%d,\"versionSN\":\"%s\",\"blockOffset\":%d,\"blockSize\":%d}"

#define  PACK_RESP_STD          "{\"msgId\":\"%s\",\"apiId\":%d,\"respCode\":%d}"
#define  PACK_RESP_GET_PORT_STATUS "{\"msgId\":\"%s\",\"apiId\":%d,\"respCode\":%d,\"portStatus\":[%d,%d]}"
#define  PACK_RESP_GET_ALL_PORT_STATUS "{\"msgId\":\"%s\",\"apiId\":%d,\"respCode\":%d,\"portStatus\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}"
#define  PACK_RESP_SET_PORT_ON  PACK_RESP_STD
#define  PACK_RESP_SET_PORT_OFF   PACK_RESP_STD
#define  PACK_RESP_UPDATE_VERSION PACK_RESP_STD
#define  PACK_RESP_SET_PORT_STATUS_ALL  PACK_RESP_STD
#define  PACK_RESP_MULTI_SET_PORT_ON_OFF   PACK_RESP_STD

#define  MAP(x)     (x==ON?1:0)  
/*--------Function declaration----------------------------------------------*/
void initPort(void);
uint8_t getPortStatus(uint8_t index);
int setPortStatus(uint8_t index,PortStatus_t status);
#endif
