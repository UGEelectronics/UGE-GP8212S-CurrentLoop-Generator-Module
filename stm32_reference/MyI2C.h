#ifndef __MYI2C_H
#define __MYI2C_H

#include "stm32f10x.h"                  // Device header



void MyI2C_Init(void);
void MyI2C_Start(void);
void MyI2C_Stop(void);
void MyI2C_SendByte(uint8_t Byte);
uint8_t MyI2C_ReceiveByte(void);
void MyI2C_SendAck(uint8_t AckBit);
uint8_t MyI2C_ReceiveAck(void);

void GP8212_Init(void);
void GP8212_Start(void);
void GP8212_Stop(void);
void GP8212_SendByte(uint8_t Byte);
uint8_t GP8212_ReceiveByte(void);
void GP8212_SendAck(uint8_t AckBit);
uint8_t GP8212_ReceiveAck(void);






#endif
