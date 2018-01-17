#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

/*--------------Includes-------------------------------------------*/
#include "mbed.h"
/*------------- Type definition------------------------------------*/
typedef struct _ringBuff{
	int front;
	int rear;
	uint8_t size;
	uint8_t* data;
}RingBuff_t,*pRingBuff_t;

/*-------------Prototype-------------------------------------------*/
void ringBuffInit(pRingBuff_t,uint8_t size);
int ringBuffIsEmpty(pRingBuff_t rb);
int ringBuffIsFull(pRingBuff_t rb);
uint8_t ringBuffPop(pRingBuff_t rb);
void ringBuffPush(pRingBuff_t,uint8_t element);

#endif
