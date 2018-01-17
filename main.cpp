/*--------------------------------------------------------------------------*/
/*                            Shared Plugs Solution                         */
/*--------------------------------------------------------------------------*/
/*-----Arthor-----------Date------------Version-----------------------------*/
/*     Orange         2018-1-14         V1.0.0  
	Description:
							1. Mbed OS 2.0
							2. JSON
							3. Wechat Control (base on socket)
							4. OTA
							5. Time Control
							6. MD5 Hash & crc16 checksum 
*/

/*--------Includes-----------------------------------------------------------*/
#include "UserConfig.h"
#include "lib_crc16.h"
#include "cJSON.h"
#include "flashLayout.h"
#include "ota.h"
#include "plug.h"
#include "ringBuffer.h"
#include "md5Std.h"

#if FuncCfg_WatchDog
	#include "WatchDog.h"
#endif

/*--------Variables Definition------------------------------------------------*/
#if !DEBUG_MODE
const char* DEFAULT_SERVER_IP = "112.74.170.197";
const int DEFAULT_SERVER_PORT = 44441;
#else
const char* DEFAULT_SERVER_IP = "192.168.0.100";
const int DEFAULT_SERVER_PORT = 49999;
#endif

const char* VERSION_DESCRIBTION = "Firmware of shared plug,run with mbed os 2.0\r\nVersion:V1.0.0\r\nDate:2018-01-17\r\n";
/* Debug UART */
Serial pc(USBTX, USBRX);
/* GPIO Definition */
DigitalOut relay0(PTE24); 
DigitalOut relay1(PTE25);
DigitalOut relay2(PTE26);
DigitalOut relay3(PTB2);
DigitalOut relay4(PTB3);
DigitalOut relay5(PTB9);
DigitalOut relay6(PTB10);
DigitalOut relay7(PTB11);
DigitalOut relay8(PTB20);
DigitalOut relay9(PTB21);
DigitalOut relay10(PTB22);
DigitalOut relay11(PTB23);
DigitalOut relay12(PTC0);
DigitalOut relay13(PTC1);
DigitalOut relay14(PTC2);
DigitalOut relay15(PTC3);
#if FuncCfg_Led
DigitalOut server(PTC4);  //LED that indicates server condition
#endif
EthernetInterface eth;
TCPSocketConnection tcpsocket;

char tempBuffer[256];
char plugModelNO[16];         // set mac as the modle NO
uint8_t versionId;
char ECHO_SERVER_ADDRESS[20];
int ECHO_SERVER_PORT;
volatile uint32_t systemTimer = 0;
volatile uint32_t msgSendTimeCounter = 0; 
volatile Plug_t plug;
volatile EventHandle_t eventHandle;
volatile MsgHandle_t msgHandle = {invalidId,invalidId,RESP_DEFAULT,true};
SocketInfo_t socketInfo;
OTAInfo_t OTAInfo;
/*----------Prototype--------------------------------------------------------*/
void oneSecondThread(void const *argument);
/*----------Macro Definition-------------------------------------------------*/
#define  CODE_SIZE_CALCULATE_THRE    (30)
/*----------Functions Definition---------------------------------------------*/

/*!
 * @brife: Get code size,for calculating code MD5
 * @input Start address of the code
 * @return Size of code,in bytes
*/

uint32_t getCodeSize(char* startAddr,char* endAddr)
{
	int size = 0;
	int count = 0;
	char* p = startAddr;
	while(count <= CODE_SIZE_CALCULATE_THRE && p<=endAddr){
		if(*p != 0xff){
			size++;
			size += count;
			count = 0;
		}else{
			count++;
		}
		p++;
	}
	return size;
}

/*!
 * @brife: Get MD5 value of the string
 * @input Address of the string & lengths of the string & the array for storing md5 value
 * @return Null 
*/
void getMD5Value(unsigned char* address,unsigned int len,unsigned char md5[16])
{
	MD5_STD_CTX md5Std;
	md5Init(&md5Std);
	md5Update(&md5Std,address,len);
	md5Final(&md5Std,md5);
}

/*!
 * @brife: Initialize ethernet interface,get ip and mac
 * @input Null
 * @return Null
*/
void initETH(void)
{
    uint8_t MAC_ADDRESS[6];   
	  #if !DEBUG_MODE
    eth.init();//DHCP
	  #else
	  eth.init("192.168.0.101", "255.255.255.0", "192.168.0.1");
	  #endif
    eth.connect();         //connect ethernet
    pc.printf("MAC Addr:%s\r\nIP Addr:%s\r\n",eth.getMACAddress(),eth.getIPAddress());    //get client IP address
    if(strcmp(eth.getIPAddress(),NULL) == NULL) {
			#if FuncCfg_DebugImfo
      pc.printf("RJ45 error! system will be reset now, please wait...\r\n");
			#endif
      NVIC_SystemReset();
    }
    sscanf(eth.getMACAddress(),"%02x:%02x:%02x:%02x:%02x:%02x",
		&MAC_ADDRESS[0],&MAC_ADDRESS[1],&MAC_ADDRESS[2],&MAC_ADDRESS[3],&MAC_ADDRESS[4],&MAC_ADDRESS[5]);
    sprintf(plugModelNO,"%02x%02x%02x%02x%02x%02x",
		MAC_ADDRESS[0],MAC_ADDRESS[1],MAC_ADDRESS[2],MAC_ADDRESS[3],MAC_ADDRESS[4],MAC_ADDRESS[5]);
}

/*!
 * @brife: Initialize the variable plug
 * @input Null
 * @return Null
*/

void initPlug(void)
{	
	char *cdata = (char*)VERSION_STR_ADDRESS;
  uint32_t codeSize;
	unsigned char codeMD5[16];
	char* md5String = (char*)malloc(33);
	
	for(int i=0;i<MAX_PORT_NUM;i++){
		plug.portStatus[i] = OFF;      // Save port status
		plug.duration[i] = 0;
		plug.setDuration[i] = 0;
	}
	/* calculate the verdionSN,md5 of code */
	codeSize = getCodeSize((char*)CODE_START_ADDRESS,(char*)(OTA_CODE_START_ADDRESS-1));
	#if FuncCfg_DebugImfo
	pc.printf("Code size =%d bytes\r\n",codeSize);
	#endif
	getMD5Value((unsigned char*)CODE_START_ADDRESS,codeSize,codeMD5);
	sprintf(md5String,(char*)"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					codeMD5[0],codeMD5[1],codeMD5[2],codeMD5[3],
					codeMD5[4],codeMD5[5],codeMD5[6],codeMD5[7],
					codeMD5[8],codeMD5[9],codeMD5[10],codeMD5[11],
					codeMD5[12],codeMD5[13],codeMD5[14],codeMD5[15]);
	md5String[33] = '\0';
	#if FuncCfg_DebugImfo
	pc.printf("Code MD5:%s\r\n",md5String);
	#endif
	for(int i=0;i<4;i++){
		tempBuffer[i] = cdata[i];
	}
	sprintf(tempBuffer+4,md5String);
	erase_sector(VERSION_STR_ADDRESS);
  program_flash(VERSION_STR_ADDRESS,tempBuffer, SECTOR_SIZE);
//	pc.printf("Save version SN to flash!\r\n");
	
	plug.versionSN = (char*)md5String;
	plug.deviceStatus = OFFLINE;
	plug.mac = plugModelNO;
	plug.connCnt= 0;
	plug.serverConnectFlag = false;
	plug.isChargingFlag = false;
  ringBuffInit((RingBuff_t*)(&plug.portStatusChange),RING_BUFF_SIZE);
	
	#if FuncCfg_DebugImfo
	pc.printf("Version SN:%s\r\nPlugModelNO:%s\r\n",plug.versionSN,plugModelNO);
	#endif
}

/*!
 * @brife: Initialize server IP adress
 * @input Null
 * @return Null
*/
void initServer(void)
{
    char* cdata = (char*)SERVER_IP_ADDRESS;
    uint16_t crc16,tempcrc16 = 0;
    crc16 = calculate_crc16(cdata, 6);
    tempcrc16 = (cdata[6] << 8) | cdata[7];
    if(crc16 == tempcrc16) {
        sprintf(ECHO_SERVER_ADDRESS,"%d.%d.%d.%d",cdata[0],cdata[1],cdata[2],cdata[3]);
        ECHO_SERVER_PORT = (cdata[4]<<8)|cdata[5];
    } else {
        sprintf(ECHO_SERVER_ADDRESS,"%s",DEFAULT_SERVER_IP);
        ECHO_SERVER_PORT = DEFAULT_SERVER_PORT;
    }
		#if FuncCfg_DebugImfo
    pc.printf("Initialize Server: %s:%d\r\n",ECHO_SERVER_ADDRESS,ECHO_SERVER_PORT);
		#endif
}

/*!
 * @brife: Eventhandle initialization
 * @input Null
 * @return Null
*/
void initEventHandle(void)
{
	eventHandle.onlineRequestFlag = true;
	eventHandle.deviceStatusUpdateFlag = false;	
	eventHandle.sendHeartbeatFlag = false;
	eventHandle.updateRequestFlag = false;
	eventHandle.checkTimeOverFlag  = false;
	eventHandle.portStatusUpdateFlag = false;
}

/*!
 * @brife: Thread whose period is 1s 
 * @input Null
 * @return Null
*/
void oneSecondThread(void const *argument)
{
	while(1){
		Thread::wait(1000);
		systemTimer++;
		#if FuncCfg_DebugImfo
		pc.printf("----SystemTimer:%d----\r\n",systemTimer);
		#endif
		for(int i=0;i<USED_PORT_NUM;i++){
			if(plug.portStatus[i] == ON){
				plug.duration[i]++;
			}
		}
		if(systemTimer%HEARTBEAT_PERIOD == 0){
			eventHandle.sendHeartbeatFlag = true;
		}
		if(systemTimer%CHECK_TIME_OVER_PERIOD == 0){
			eventHandle.checkTimeOverFlag = true;
		}
	}
}

/*!
 * @brife: Check the status of network
 * @input Null
 * @return Null
*/
void checkNetworkStatus(void)
{
	char heartbeatBuffer[64];
	#if FuncCfg_Ethernet
		if(plug.serverConnectFlag == true){
			if(eventHandle.sendHeartbeatFlag == true){
				eventHandle.sendHeartbeatFlag = false;
				sprintf(heartbeatBuffer,PACK_REQ_HEARTBEAT,HEARTBEAT_PUSH);
				if(tcpsocket.send(heartbeatBuffer,strlen(heartbeatBuffer)) <= 0){
					pc.printf("Send heartbeat pack fail!\r\n");
					plug.serverConnectFlag = false;
					plug.deviceStatus = OFFLINE;
				}else{
					#if FuncCfg_DebugImfo
					pc.printf("Send heartbeat pack successfully!\r\n");
					pc.printf("Send %d bytes,%s\r\n",strlen(heartbeatBuffer),heartbeatBuffer);
					#endif
				}
			}
		}else{
			  tcpsocket.close();
			  if(tcpsocket.connect(ECHO_SERVER_ADDRESS,ECHO_SERVER_PORT)<0){
				#if FuncCfg_DebugImfo
				pc.printf("Cannot connect to server!\r\n");
				#endif
			}else{
				#if FuncCfg_DebugImfo
				pc.printf("Connect to server!\r\n");
				pc.printf("Num of connection:%d\r\n!",++plug.connCnt);
				#endif
				plug.serverConnectFlag = true;
				eventHandle.onlineRequestFlag = true;  // online request,first msg to server
			}
		}
	#endif
		
	#if FuncCfg_Led
		if(plug.serverConnectFlag == true)
			SERVER_CONNECT_LED_ON;
		else
			SERVER_CONNECT_LED_OFF;
	#endif
}

/*!
 * @brife: Check whether time is over for each port
 * @input Null
 * @return Null
*/
void checkTimeOver(void)
{
	for(int i =0;i<USED_PORT_NUM;i++){
		if(plug.portStatus[i] == ON){
			pc.printf("Port[%d]---Duration:%d secs---setDuration:%d secs---\r\n",i,plug.duration[i],plug.setDuration[i]);
			if(plug.duration[i] >= plug.setDuration[i]){
				setPortStatus(i,OFF);//time over,close port
				plug.portStatus[i] = OFF;// save status of port
				#if FuncCfg_DebugImfo
					pc.printf("Time over in port[%d]:----duration:%d mins----setDuration:%d mins\r\n",
					i,plug.duration[i]/60,plug.setDuration[i]/60);
				#endif
				plug.duration[i] = 0;//initialize parameter
				plug.setDuration[i] = 0;
				if(ringBuffIsFull((pRingBuff_t)(&plug.portStatusChange)) == 0)
					ringBuffPush((pRingBuff_t)(&plug.portStatusChange),i);   //record the index of port that status changed
				eventHandle.portStatusUpdateFlag = true; //notify msg to server when port status change
			}
		}
	}
}

/*!
 * @brife:parse text to ota bincode,
 *        bincode format:  HEADER--------BLOCK OFFSET------BLOCK SIZE------CHECKSUM-------BIN DATE---------
 *                        "OTABIN"         4 bytes          2 bytes        2 bytes       maxmum 4k bytes
 * @input: text,string format
 * @return: Null
*/

void parseBincodeBuffer(char *text)
{
	char* buf;
	int crc16;
	int blockOffset = 0;
	int blockSize = 0;
	int checksum = 0;
	int curSectorSize;
	int reWriteCnt = 0;
	
	if(strncmp(text,SOCKET_OTA_HEADER,strlen(SOCKET_OTA_HEADER))== NULL){
		blockOffset = ((text[OTA_BLOCKOFFSET_POS]<<24) | text[OTA_BLOCKOFFSET_POS+1]<<16 | 
		                text[OTA_BLOCKOFFSET_POS+2]<<8 | text[OTA_BLOCKOFFSET_POS+3]);
		blockSize = ((text[OTA_BLOCKSIZE_POS]<<8) | text[OTA_BLOCKSIZE_POS+1]);
		checksum = ((text[OTA_CHECKSUM_POS]<<8) | text[OTA_CHECKSUM_POS+1]);
		#if FuncCfg_DebugImfo
		pc.printf("OTA files imfo:blockoffset:%d----blocksize:%d-----checksum:%x\r\n",blockOffset,blockSize,checksum);
		#endif
		buf = (char*)malloc(blockSize);
		if(NULL == buf){
			pc.printf("Cannot malloc enough memory!\r\n");
		}else{
			if(msgHandle.sendMsgId == reqOTA){
				msgHandle.sendMsgId = invalidId;
			}
			memcpy(buf,text+OTA_BINDATA_POS,blockSize);
			crc16 = calculate_crc16(buf,blockSize);
			#if FuncCfg_DebugImfo
			pc.printf("crc16 = %x checksum = %x\r\n",crc16,checksum);
			#endif
			if(OTAInfo.curSector+1<OTAInfo.sectorNum)
				curSectorSize = SECTOR_SIZE;
			else
				curSectorSize = OTAInfo.lastSectorSize;
			
		 if(curSectorSize == blockSize && OTAInfo.curSector*SECTOR_SIZE == blockOffset && crc16 == checksum){
					IAPCode iapCode;
					#if FuncCfg_DebugImfo
					pc.printf("Write sector %d %d bytes to ota partition\r\n",OTAInfo.curSector,blockSize);
					#endif
					erase_sector(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE);
					iapCode = program_flash(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,buf,blockSize);
					crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,blockSize);
					while(crc16 != checksum && reWriteCnt < OTA_REWRITE_TIMES){
						erase_sector(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE);
						iapCode = program_flash(OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,buf,blockSize);
						#if FuncCfg_DebugImfo
						pc.printf("Rewrite to sector %d\r\n",OTAInfo.curSector);
						pc.printf("IapCode=%d\r\n",iapCode);
						#endif
						crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS+OTAInfo.curSector*SECTOR_SIZE,blockSize);
						reWriteCnt++;
						wait(0.1);
					}
					#if FuncCfg_DebugImfo
					pc.printf("reWriteCnt=%d\r\n",reWriteCnt);
					#endif
					if(crc16 == checksum){
						#if FuncCfg_DebugImfo
						pc.printf("Write and verify sector %d successfully\r\n",OTAInfo.curSector);
						#endif
						OTAInfo.curSector++;
					}else{
						#if FuncCfg_DebugImfo
						pc.printf("Write sector %d failed!\r\n",OTAInfo.curSector);
						#endif
						plug.deviceStatus = OTA_FAIL;  //device status change!
						eventHandle.updateRequestFlag = false; //stop request for update
						eventHandle.deviceStatusUpdateFlag = true;
						initOTA();  //recover OTA partition
					}
				}else{//parameter error!
					#if FuncCfg_DebugImfo
						pc.printf("Parameters error!OTA Fail!\r\n");
					#endif
					plug.deviceStatus = OTA_FAIL;
					eventHandle.updateRequestFlag = false;
					eventHandle.deviceStatusUpdateFlag =true;
					initOTA();
				}
		 free(buf);
		}
	}
	
}
/*!
 * @brife: send the respond of command message
 * @input: msgid
 * @return: Null
*/

void cmdMsgRespHandle(MsgId_t msgId)
{
	int len;
	if(msgId == invalidId || msgId >= unknownMsgId)
		return;
	memset(socketInfo.outBuffer,0,sizeof(socketInfo.outBuffer));
	if(msgId == cmdSetPortOn)
		sprintf(socketInfo.outBuffer,PACK_RESP_SET_PORT_ON,msgHandle.msgIdStr,PORT_ON_RECV,msgHandle.respCode);
	else if(msgId == cmdSetPortOff)
		sprintf(socketInfo.outBuffer,PACK_RESP_SET_PORT_OFF,msgHandle.msgIdStr,PORT_OFF_RECV,msgHandle.respCode);
	else if(msgId == cmdGetPortStatus){
		if(msgHandle.respBuf.index==100)
			sprintf(socketInfo.outBuffer,PACK_RESP_GET_ALL_PORT_STATUS,msgHandle.msgIdStr,GET_PORT_STATUS_RECV,msgHandle.respCode,
				MAP_TO_LOGIC(plug.portStatus[0]),MAP_TO_LOGIC(plug.portStatus[1]),MAP_TO_LOGIC(plug.portStatus[2]),MAP_TO_LOGIC(plug.portStatus[3]),
				MAP_TO_LOGIC(plug.portStatus[4]),MAP_TO_LOGIC(plug.portStatus[5]),MAP_TO_LOGIC(plug.portStatus[6]),MAP_TO_LOGIC(plug.portStatus[7]),
				MAP_TO_LOGIC(plug.portStatus[8]),MAP_TO_LOGIC(plug.portStatus[9]),MAP_TO_LOGIC(plug.portStatus[10]),MAP_TO_LOGIC(plug.portStatus[11]),
				MAP_TO_LOGIC(plug.portStatus[12]),MAP_TO_LOGIC(plug.portStatus[13]),MAP_TO_LOGIC(plug.portStatus[14]),MAP_TO_LOGIC(plug.portStatus[15])
			);
		else
		sprintf(socketInfo.outBuffer,PACK_RESP_GET_PORT_STATUS,msgHandle.msgIdStr,GET_PORT_STATUS_RECV,msgHandle.respCode,msgHandle.respBuf.index,MAP_TO_LOGIC(msgHandle.respBuf.statu));
	}
	else if(msgId == cmdUpdateVersion)
		sprintf(socketInfo.outBuffer,PACK_RESP_UPDATE_VERSION,msgHandle.msgIdStr,UPDATE_RECV,msgHandle.respCode);
	else if(msgId == cmdMultiPortOnOff)
		sprintf(socketInfo.outBuffer,PACK_RESP_MULTI_SET_PORT_ON_OFF,msgHandle.msgIdStr,MULTI_PORTS_ON_OFF_RECV,msgHandle.respCode);
	else 
		return;
	len = strlen(socketInfo.outBuffer);
	tcpsocket.send(socketInfo.outBuffer,len);
	#if FuncCfg_DebugImfo
	pc.printf("CMD respond handle,send %d bytes:%s\r\n",len,socketInfo.outBuffer);
	#endif
	
}

/*!
 * @brife:parse text to json
 * @input: text,string format
 * @return: Null
*/
void parseRecvMsgInfo(char* text)
{
	cJSON *json;
	json = cJSON_Parse(text);
	if(!json){
		#if FuncCfg_DebugImfo
		pc.printf("Not json string,start to parse another way!\r\n");
		parseBincodeBuffer(text);
		#endif
	}else{
		#if FuncCfg_DebugImfo
		pc.printf("Start parse json!\r\n");
		#endif	
		if((cJSON_GetObjectItem(json,"apiId") != NULL)){//check item "apiId"
			int id;
			id = cJSON_GetObjectItem(json,"apiId")->valueint;
			if(id > MAX_REQ_ID){//cmd pack
				if(cJSON_GetObjectItem(json,"msgId") != NULL){//check item "msgId"
					sprintf((char*)msgHandle.msgIdStr,cJSON_GetObjectItem(json,"msgId")->valuestring);
				if(id == PORT_ON_RECV){//port on request
					msgHandle.recvMsgId = cmdSetPortOn;
					if((cJSON_GetObjectItem(json,"index") != NULL) && (cJSON_GetObjectItem(json,"setDuration") != NULL)){
						int index;
						int setDuration;
						index = cJSON_GetObjectItem(json,"index")->valueint;
						setDuration = cJSON_GetObjectItem(json,"setDuration")->valueint;
						if((index >=0) && (index < USED_PORT_NUM) && (setDuration > 0) && (setDuration <= MAX_CHARGING_TIME)){//check parameters
							msgHandle.respCode = RESP_OK;
							if(plug.portStatus[index] == OFF){
								plug.setDuration[index] = setDuration * 60;//conver to second
								plug.duration[index] = 0;
								plug.isChargingFlag = true; 
								setPortStatus(index,ON);  //Set port on
								plug.portStatus[index] = ON;
								if(ringBuffIsFull((pRingBuff_t)(&plug.portStatusChange)) == 0)
									ringBuffPush((pRingBuff_t)(&plug.portStatusChange),index);
								eventHandle.portStatusUpdateFlag = true; //notify to server
								msgHandle.respCode = RESP_OK;
							}else{//illegal request!
								msgHandle.respCode = RESP_ILLEGAL_REQUEST;
								#if FuncCfg_DebugImfo
								pc.printf("******Request error!illegal reuquest******\r\n");
								#endif
							}
						}else{//parameters error!
							msgHandle.respCode = RESP_PARAM_ERROR;
							#if FuncCfg_DebugImfo
							pc.printf("******Request error!parameters error!******\r\n");
							#endif
						}
					}else{//item error!
						msgHandle.respCode = RESP_ITEM_ERROR;
						#if FuncCfg_DebugImfo
						pc.printf("******Request error!items error!******\r\n");
						#endif
					}
					cmdMsgRespHandle(msgHandle.recvMsgId);
				}else if(id == PORT_OFF_RECV){ //port off request!
					msgHandle.recvMsgId = cmdSetPortOff;
					if(cJSON_GetObjectItem(json,"index") != NULL){
						int index; 
						index = cJSON_GetObjectItem(json,"index")->valueint;
						if((index >= 0) && (index < USED_PORT_NUM)){
							if(plug.portStatus[index] == ON){
								int i =0;
								setPortStatus(index,OFF); // Set port off
								plug.portStatus[index] = OFF;
								#if FuncCfg_DebugImfo
								pc.printf("Port[%d] off!----Duration:%d secs----setDuration:%d secs\r\n",
										index,plug.duration[index],plug.setDuration[index]);
								#endif
								plug.setDuration[index] = 0;
								plug.duration[index] = 0;
								if(ringBuffIsFull((pRingBuff_t)(&plug.portStatusChange))==0)
									ringBuffPush((pRingBuff_t)(&plug.portStatusChange),index);
								while(plug.portStatus[i]==OFF && i<=USED_PORT_NUM){//check if other port is on
									i++;
								}
								if(i>USED_PORT_NUM)
									plug.isChargingFlag = false;
								eventHandle.portStatusUpdateFlag = true;//notify to server
								msgHandle.respCode = RESP_OK;
								}else{
									msgHandle.respCode = RESP_ILLEGAL_REQUEST;
									#if FuncCfg_DebugImfo
									pc.printf("******Request error!illegal reuquest******\r\n");
									#endif
								}
							}else{
								msgHandle.respCode = RESP_PARAM_ERROR;
								#if FuncCfg_DebugImfo
								pc.printf("******Request error!parameters error!******\r\n");
								#endif
							}
						}else{
							msgHandle.respCode = RESP_ITEM_ERROR;
							#if FuncCfg_DebugImfo
							pc.printf("******Request error!items error!******\r\n");
							#endif
						}
						cmdMsgRespHandle(msgHandle.recvMsgId);
					}else if(id == GET_PORT_STATUS_RECV){//get status request!
						msgHandle.recvMsgId = cmdGetPortStatus;
						if(cJSON_GetObjectItem(json,"index") != NULL){
							int index = cJSON_GetObjectItem(json,"index")->valueint;
							if((index>=0) && (index < USED_PORT_NUM)){
								msgHandle.respBuf.index = index;
								msgHandle.respBuf.statu = plug.portStatus[index];
								msgHandle.respCode = RESP_OK;
							}else if(index == 100){
								msgHandle.respCode = RESP_OK;
								msgHandle.respBuf.index = 100;
							}else{
								msgHandle.respCode = RESP_PARAM_ERROR;
								#if FuncCfg_DebugImfo
								pc.printf("******Request error!parameters error!******\r\n");
								#endif
							}
						}else{
							msgHandle.respCode = RESP_ITEM_ERROR;
							#if FuncCfg_DebugImfo
							pc.printf("******Request error!items error!******\r\n");
							#endif
						}
						cmdMsgRespHandle(msgHandle.recvMsgId);
					}else if(id == UPDATE_RECV){ //update!
						msgHandle.recvMsgId = cmdUpdateVersion;
						if((cJSON_GetObjectItem(json,"versionSN")!= NULL) && (cJSON_GetObjectItem(json,"versionSize") != NULL) &&
							(cJSON_GetObjectItem(json,"checksum") != NULL)){
								sprintf(OTAInfo.versionSN,cJSON_GetObjectItem(json,"versionSN")->valuestring);
								OTAInfo.totalSize = cJSON_GetObjectItem(json,"versionSize")->valueint;
								OTAInfo.checkSum = cJSON_GetObjectItem(json,"checksum")->valueint;
								if(OTAInfo.totalSize >= MIN_CODE_SIZE && OTAInfo.totalSize <= MAX_CODE_SIZE
									&& strcmp(OTAInfo.versionSN,plug.versionSN)!= NULL){//check parameters!
									#if FuncCfg_DebugImfo
									pc.printf("The new versinSN:%s\r\nTotal size:%d,Checksum:%x\r\n",OTAInfo.versionSN,OTAInfo.totalSize,OTAInfo.checkSum);
									#endif
									if(OTAInfo.totalSize % SECTOR_SIZE == 0){
										OTAInfo.sectorNum = OTAInfo.totalSize / SECTOR_SIZE;
										OTAInfo.lastSectorSize = SECTOR_SIZE;
									}else{
										OTAInfo.sectorNum = OTAInfo.totalSize / SECTOR_SIZE + 1;
										OTAInfo.lastSectorSize = OTAInfo.totalSize % SECTOR_SIZE;
									}
									msgHandle.respCode = RESP_OK;
									eventHandle.updateRequestFlag = true;//start to update!
								}else{
									msgHandle.respCode = RESP_PARAM_ERROR;
									#if FuncCfg_DebugImfo
									pc.printf("******Request error!parameters error!******\r\n");
									pc.printf("Note:please check version size and version SN\r\n");
									#endif
								}
							}else{
								msgHandle.respCode = RESP_ITEM_ERROR;
								#if FuncCfg_DebugImfo
								pc.printf("******Request error!item error!******\r\n");
								#endif
							}
						cmdMsgRespHandle(msgHandle.recvMsgId);
				}else if(id == MULTI_PORTS_ON_OFF_RECV){//batch set ports on or off,interface for debug!
					msgHandle.recvMsgId = cmdMultiPortOnOff;
					if(cJSON_GetObjectItem(json,"portStatus") != NULL){//check item!
							cJSON* array;
						  int itemNum;
							array = cJSON_GetObjectItem(json,"portStatus");
						  itemNum = cJSON_GetArraySize(array);
						  #if FuncCfg_DebugImfo
							pc.printf("Num of array:%d\r\n",itemNum);
							#endif
						  if(itemNum != USED_PORT_NUM){
								msgHandle.respCode = RESP_PARAM_ERROR;
							}else{
								int temp;
								msgHandle.respCode = RESP_OK;
								for(int i =0;i<USED_PORT_NUM;i++){
									temp = cJSON_GetArrayItem(array,i)->valueint;
									#if FuncCfg_DebugImfo
									pc.printf("Set port[%d]=%d\r\n",i,temp);
									#endif
									if(plug.portStatus[i] == ON){
										if(temp == 1){
											plug.setDuration[i] = DEFAULT_CHARGING_TIME;//Reset charging time
											plug.duration[i] = 0;
										}else if(temp == 0){
											plug.setDuration[i] = 0;
											plug.duration[i] = 0;
											setPortStatus(i,OFF);
											plug.portStatus[i] = OFF;
											if(ringBuffIsFull((pRingBuff_t)&plug.portStatusChange)==0){
												ringBuffPush((pRingBuff_t)&plug.portStatusChange,i);//notify to server when staus change
												eventHandle.portStatusUpdateFlag = true;
											}
										}
									}else if(plug.portStatus[i] == OFF){
										if(temp == 1){
											plug.setDuration[i] = DEFAULT_CHARGING_TIME;
											plug.duration[i] = 0;
											setPortStatus(i,ON);
											plug.portStatus[i] = ON;
											if(ringBuffIsFull((pRingBuff_t)&plug.portStatusChange)==0){
												ringBuffPush((pRingBuff_t)&plug.portStatusChange,i);//notify to server when staus change
												eventHandle.portStatusUpdateFlag = true;
											}
										}else if(temp == 0){
											//do nothing
										}
									}
								}
							}
						}else{
							msgHandle.respCode = RESP_ITEM_ERROR;
						}
						cmdMsgRespHandle(msgHandle.recvMsgId); //Send respond!
				}
			}
			}else{//respond pack!
				msgHandle.respCode = (RespondCode_t)cJSON_GetObjectItem(json,"respCode")->valueint;
				if(id == CONNECT_REQUEST_PUSH){
					pc.printf("Online request respond!\r\n");
					msgHandle.recvMsgId = reqConnect;
					plug.deviceStatus = ONLINE;//comfirm online!
				}else if(id == STATUS_CHANGE_PUSH){
					if(msgHandle.sendMsgId == reqNotifyDeviceStatus){
						msgHandle.recvMsgId = reqNotifyDeviceStatus;
						if(plug.deviceStatus == OTA_FAIL){
							plug.deviceStatus = ONLINE;//update device status;
							eventHandle.deviceStatusUpdateFlag = true; //notify to server
						}
					}else if(msgHandle.sendMsgId == reqNotifyPortStatus && msgHandle.respCode == RESP_OK){
						msgHandle.recvMsgId = reqNotifyPortStatus;
						msgHandle.portStatusNotifyDone = true;
					}
				}else{//ignore respond of heartbeat pack!
					msgHandle.recvMsgId = invalidId;
				}
				pc.printf("send=%d,rec=%d,resp=%d\r\n",msgHandle.sendMsgId,msgHandle.recvMsgId,msgHandle.respCode);
				if(msgHandle.sendMsgId >0 && msgHandle.sendMsgId == msgHandle.recvMsgId && msgHandle.respCode == RESP_OK){//send pack successfully!
					#if FuncCfg_DebugImfo
					pc.printf("Send msg %d successfully\r\n",msgHandle.sendMsgId);
					#endif
					msgHandle.sendMsgId = invalidId;
					msgHandle.recvMsgId = invalidId;
				}
			}
		}
		cJSON_Delete(json);
	}
}

/*!
 * @brife:Deal with the transmit of Json pack
 * @input: The msgId of the msg expected sending
 * @return: Null
*/
void msgSendHandle(MsgId_t sendMsgId)
{
	int crc16;
	int len;
	char buf[VERSION_STR_LEN];
	if(sendMsgId == invalidId || sendMsgId >= unknownMsgId){
		#if FuncCfg_DebugImfo
			pc.printf("Invalid msg ID!\r\n");
		#endif
		return;
	}
	memset(socketInfo.outBuffer,0,sizeof(socketInfo.outBuffer));  //initialize buffer
	if(sendMsgId == reqConnect){
		sprintf(socketInfo.outBuffer,PACK_REQ_CONNECT,CONNECT_REQUEST_PUSH,plug.versionSN,plugModelNO,(plug.connCnt>1?1:0),
				MAP_TO_LOGIC(plug.portStatus[0]),MAP_TO_LOGIC(plug.portStatus[1]),MAP_TO_LOGIC(plug.portStatus[2]),MAP_TO_LOGIC(plug.portStatus[3]),
				MAP_TO_LOGIC(plug.portStatus[4]),MAP_TO_LOGIC(plug.portStatus[5]),MAP_TO_LOGIC(plug.portStatus[6]),MAP_TO_LOGIC(plug.portStatus[7]),
				MAP_TO_LOGIC(plug.portStatus[8]),MAP_TO_LOGIC(plug.portStatus[9]),MAP_TO_LOGIC(plug.portStatus[10]),MAP_TO_LOGIC(plug.portStatus[11]),
				MAP_TO_LOGIC(plug.portStatus[12]),MAP_TO_LOGIC(plug.portStatus[13]),MAP_TO_LOGIC(plug.portStatus[14]),MAP_TO_LOGIC(plug.portStatus[15])
		);
	}else if(sendMsgId == reqNotifyDeviceStatus){
		sprintf(socketInfo.outBuffer,PACK_REQ_NOTIFY_DEVICE_STATUS,STATUS_CHANGE_PUSH,plug.deviceStatus);
	}else if(sendMsgId == reqNotifyPortStatus){
		if(msgHandle.portStatusNotifyDone == true){
			if(ringBuffIsEmpty((pRingBuff_t)(&plug.portStatusChange)) == 0){
				int index;
				index = ringBuffPop((pRingBuff_t)(&plug.portStatusChange));
				sprintf(socketInfo.outBuffer,PACK_REQ_NOTIFY_PORT_STATUS,STATUS_CHANGE_PUSH,index,MAP_TO_LOGIC(plug.portStatus[index]));
				msgHandle.portStatusNotifyDone = false;
				/* save index and statu of port to buffer */
				msgHandle.sendBuf.index = index;
				msgHandle.sendBuf.statu = plug.portStatus[index];
			}
		}else{
			sprintf(socketInfo.outBuffer,PACK_REQ_NOTIFY_PORT_STATUS,STATUS_CHANGE_PUSH,msgHandle.sendBuf.index,MAP_TO_LOGIC(msgHandle.sendBuf.statu));
		}
	}else if(sendMsgId == reqOTA){
		#if FuncCfg_DebugImfo
		pc.printf("SectorNum:%d,curSector:%d\r\n",OTAInfo.sectorNum,OTAInfo.curSector);
		#endif
		if(OTAInfo.curSector < OTAInfo.sectorNum){
			if(OTAInfo.curSector + 1 == OTAInfo.sectorNum){
				sprintf(socketInfo.outBuffer,PACK_REQ_UPDATE,UPDATE_REQUEST_PUSH,
				OTAInfo.versionSN,OTAInfo.curSector*SECTOR_SIZE,OTAInfo.lastSectorSize);
			}else{
				sprintf(socketInfo.outBuffer,PACK_REQ_UPDATE,UPDATE_REQUEST_PUSH,
				OTAInfo.versionSN,OTAInfo.curSector*SECTOR_SIZE,SECTOR_SIZE);
			}
		}else{ //recieve all sectors successfully,then write new version id to memory
			#if FuncCfg_DebugImfo
			pc.printf("Erase extra OTA partition!\r\n");
			#endif
			if(OTAInfo.sectorNum < CODE_SECTOR_NUM){
				for(int i = OTAInfo.sectorNum;i<CODE_SECTOR_NUM;i++){
					erase_sector(OTA_CODE_START_ADDRESS+i*SECTOR_SIZE);
				}
			}
			eventHandle.updateRequestFlag = false;//stop to request updating
			crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS,OTAInfo.checkSum);
			#if FuncCfg_DebugImfo
			pc.printf("OTA code crc:%x checksum:%x\r\n",crc16,OTAInfo.checkSum);
			#endif
			if(crc16 == OTAInfo.checkSum){//Check OK
				char* cData = (char*)VERSION_STR_ADDRESS;
				crc16 = calculate_crc16((char*)OTA_CODE_START_ADDRESS,CODE_SIZE);
				#if FuncCfg_DebugImfo
				pc.printf("Download successfully!,Update OTA checksum infomation!\r\n");
				#endif
				memset(buf,0,VERSION_STR_LEN);
				buf[0] = cData[2];
				buf[1] = cData[3];
				buf[2] = (crc16 >> 8) & 0xff; //Update OTA Checksum
				buf[3] = crc16 & 0xff;

				erase_sector(VERSION_STR_ADDRESS);
				program_flash(VERSION_STR_ADDRESS,buf,SECTOR_SIZE);
				tcpsocket.close();//notify to server!
				updateCode(); //begin to update code !
			}else{//check error
				#if FuncCfg_DebufImfo
				pc.printf("Checksum is different,Download fail!\r\n");
				#endif
				plug.deviceStatus = OTA_FAIL;
				eventHandle.deviceStatusUpdateFlag = true; //notify to server!
			}
		}
	}
	len = strlen(socketInfo.outBuffer);
	if(tcpsocket.send(socketInfo.outBuffer,len)>=0)
		#if FuncCfg_DebugImfo
	pc.printf("msgSendHandle,sendMsgId:%d,send %d bytes,%s\r\n",sendMsgId,len,socketInfo.outBuffer);
		#endif
	msgHandle.sendMsgId = sendMsgId;
	msgSendTimeCounter = systemTimer;
}
/*!
 * @brife:Deal with the reception of Json pack
 * @input: Null
 * @return: Null
*/
void msgRecvHandle(void)
{
	int len;
	len = sizeof(socketInfo.inBuffer);
	memset(socketInfo.inBuffer,0x0,len);
	int n = tcpsocket.receive_all(socketInfo.inBuffer,len);
	if(n>0){
		socketInfo.inBuffer[n] = '\0';
		#if FuncCfg_DebugImfo
		pc.printf("receive %d bytes,%s\r\n",n,socketInfo.inBuffer);
		parseRecvMsgInfo(socketInfo.inBuffer);
		#endif
	}
}

/*!
 * @brife: Message handle function that deal with the transceiver of Json pack
 * @input Null
 * @return Null
*/
void msgTransceiverHandle(void)
{
	#if FuncCfg_Ethernet
	if(plug.serverConnectFlag == true){
		if(msgHandle.sendMsgId != invalidId){
			if(systemTimer - msgSendTimeCounter >= MSG_RESEND_PERIOD)
				msgSendHandle(msgHandle.sendMsgId);  //resend msg if not received respond pack
			}else{
				if(eventHandle.onlineRequestFlag == true){
					msgSendHandle(reqConnect);
					eventHandle.onlineRequestFlag = false;
					#if FuncCfg_DebugImfo
						pc.printf("online request!\r\n");
					#endif
				}else if(eventHandle.deviceStatusUpdateFlag == true){
					msgSendHandle(reqNotifyDeviceStatus);
					eventHandle.deviceStatusUpdateFlag = false;
					#if FuncCfg_DebugImfo
						pc.printf("device status update request!\r\n");
					#endif
				}else if(eventHandle.portStatusUpdateFlag == true){
					msgSendHandle(reqNotifyPortStatus);
					if(ringBuffIsEmpty((pRingBuff_t)&plug.portStatusChange))
						eventHandle.portStatusUpdateFlag = false;
					#if FuncCfg_DebugImfo
						pc.printf("port status update request!\r\n");
					#endif
				}else if(eventHandle.updateRequestFlag == true){
					msgSendHandle(reqOTA);
					/* Not clear flag here !!!*/
					#if FuncCfg_DebugImfo
						pc.printf("Version files request!\r\n");
					#endif
				}
			}
		msgRecvHandle();
	}
	#endif
}
/*!
 * @brife: MD5 test function
 * @input Null
 * @return Null
*/
void md5Test(void)
{
	int i;  
	unsigned char encrypt[] ="admin";//21232f297a57a5a743894a0e4a801fc3  
	unsigned char decrypt[16];
  pc.printf("Start md5 test\r\n");	

	MD5_STD_CTX md5;
	md5Init(&md5);
	md5Update(&md5,encrypt,strlen((char*)encrypt));
	md5Final(&md5,decrypt);

	pc.printf("md5 encrypt:%s\r\n",encrypt);
	for(i=0;i<16;i++){
		pc.printf("%02x",decrypt[i]);
	}
	pc.printf("\r\nEnd of d5 test!\r\n");
}

/*!
 * @brife: Main function
 * @input Null
 * @return Null
*/
int main(void)
{
	#if FuncCfg_Led
		SERVER_CONNECT_LED_OFF;
	#endif
	pc.baud(115200); //initialize the PC interface
	pc.printf("%s\r\n",VERSION_DESCRIBTION);

	initPort();
	initETH();
	initServer();
	initPlug();
	initEventHandle();
	initOTA();
	
	tcpsocket.set_blocking(false,40);  //nonblocking mode,may not work???
	Thread th1(oneSecondThread,NULL,osPriorityNormal,512);
	#if FuncCfg_DebugImfo
	pc.printf("Finish initialization!\r\n");
	#endif
	while(1){
		checkNetworkStatus();
		if(eventHandle.checkTimeOverFlag == true){
			checkTimeOver();
			eventHandle.checkTimeOverFlag = false;
		}
		msgTransceiverHandle();
	}
}
