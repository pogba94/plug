/*
 *  Description£º
 *     
 *
*/

/*-----------Includes-----------------------------------------------------*/
#include "UserConfig.h"
#include "plug.h"

/*-----------Variable declaration-----------------------------------------*/
extern Plug_t plug;
/*-----------Global Variable Definition-----------------------------------*/


/*-----------Prototype----------------------------------------------------*/


/*-----------Function Definition------------------------------------------*/
/*!
 * @brife: Set status of expected port
 * @input Index of port & port status expected
 * @return 0 if OK,-1 if
*/
int setPortStatus(uint8_t index,PortStatus_t status){
	if(index <= USED_PORT_NUM-1 && status <= 1){
		switch(index){
			case 0: relay0 = status;plug.portStatus[0]=status;break;
			case 1: relay1 = status;plug.portStatus[1]=status;break;
			case 2: relay2 = status;plug.portStatus[2]=status;break;
			case 3: relay3 = status;plug.portStatus[3]=status;break;
			case 4: relay4 = status;plug.portStatus[4]=status;break;
			case 5: relay5 = status;plug.portStatus[5]=status;break;
			case 6: relay6 = status;plug.portStatus[6]=status;break;
			case 7: relay7 = status;plug.portStatus[7]=status;break;
			case 8: relay8 = status;plug.portStatus[8]=status;break;
			case 9: relay9 = status;plug.portStatus[9]=status;break;
			case 10: relay10 = status;plug.portStatus[10]=status;break;
			case 11: relay11 = status;plug.portStatus[11]=status;break;
			case 12: relay12 = status;plug.portStatus[12]=status;break;
			case 13: relay13 = status;plug.portStatus[13]=status;break;
			case 14: relay14 = status;plug.portStatus[14]=status;break;
			case 15: relay15 = status;plug.portStatus[15]=status;break;
			default:;
		}
		#if FuncCfg_DebugImfo
		pc.printf("set the status of port[%d] to %d\r\n",index,status);
		#endif
		return 0;
	}else{
		#if FuncCfg_DebugImfo
		pc.printf("illegal index or status!\r\n");
		#endif
		return -1;
	}
}

/*!
 * @brife: get the status of specified port
 * @input index of port
 * @return the status of specified port
*/

uint8_t getPortStatus(uint8_t index)
{
	if(index <= USED_PORT_NUM-1){
		switch(index){
			case 0:return relay0.read();break;
			case 1:return relay1.read();break;
			case 2:return relay2.read();break;
			case 3:return relay3.read();break;
			case 4:return relay4.read();break;
			case 5:return relay5.read();break;
			case 6:return relay6.read();break;
			case 7:return relay7.read();break;
			case 8:return relay8.read();break;
			case 9:return relay9.read();break;
			case 10:return relay10.read();break;
			case 11:return relay11.read();break;
			case 12:return relay12.read();break;
			case 13:return relay13.read();break;
			case 14:return relay14.read();break;
			case 15:return relay15.read();break;
		}
	}
}

/*!
 * @brife: initialize port status as zero
 * @input Null
 * @return Null
*/
void initPort(void)
{
	relay0 = OFF;
	relay1 = OFF;
	relay2 = OFF;
	relay3 = OFF;
	relay4 = OFF;
	relay5 = OFF;
	relay6 = OFF;
	relay7 = OFF;
	relay8 = OFF;
	relay9 = OFF;
	relay10 = OFF;
	relay11 = OFF;
	relay12 = OFF;
	relay13 = OFF;
	relay14 = OFF;
	relay15 = OFF;
}



