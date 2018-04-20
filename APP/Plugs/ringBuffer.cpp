/*----------Includes--------------------------------------------------*/
#include "ringBuffer.h"
#include "mbed.h"

extern Serial pc;

/*----------Function Definition---------------------------------------*/

/*!
 * @brife: Initialize RingBuff_t structure
 * @input RingBuff_t structure pointer & size of the ring buffer
 * @return Null
*/

void ringBuffInit(pRingBuff_t rb,uint8_t size)
{
	if(size != 0){
		rb->front = 0;
		rb->rear = 0;
		rb->size = size;
		rb->data = (uint8_t*)malloc(size);
	}else{
		pc.printf("ring buffer initialize fail!\r\n");
	}
}

/*!
 * @brife: Judge if the ring buffer is empty
 * @input RingBuff_t structure pointer
 * @return -1 if the parameter is illegal,
 *          1 if the ring buffer is empty,
 *          0 if the ring buffer is not empty
*/

int ringBuffIsEmpty(pRingBuff_t rb)
{
	if(rb->size == 0){
		pc.printf("illegal size!\r\n");
		return -1;
	}
	return rb->front == rb->rear;
}
/*!
 * @brife: Judge if the ring buffer is full
 * @input RingBuff_t structure pointer
 * @return -1 if the parameter is illegal,
 *          1 if the ring buffer is full,
 *          0 if the ring buffer is not full
*/
int ringBuffIsFull(pRingBuff_t rb)
{
	if(rb->size == 0){
		pc.printf("illegal size!\r\n");
		return -1;
	}
	return rb->front == (rb->rear +1)%rb->size;
}
/*!
 * @brife: Pop element from the ring buffer
 * @input RingBuff_t structure pointer
 * @return the value the element poped
*/
uint8_t ringBuffPop(pRingBuff_t rb)
{
	uint8_t tmp;
	tmp = rb->data[rb->front];
	rb->front = (rb->front+1)%rb->size;
	return tmp;
}
/*!
 * @brife: Push member to the ring buffer
 * @input RingBuff_t structure pointer & the value of new element
 * @return Null
*/
void ringBuffPush(pRingBuff_t rb,uint8_t element)
{
	rb->data[rb->rear] = element;
	rb->rear = (rb->rear+1)%rb->size;
}
