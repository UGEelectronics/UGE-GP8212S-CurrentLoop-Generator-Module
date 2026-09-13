#include "stm32f10x.h"                  // Device header
#include "delay.h"

void MyI2C_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_2, (BitAction)BitValue);
	Delay_us(10);
}

void MyI2C_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_3, (BitAction)BitValue);
	Delay_us(10);
}

uint8_t MyI2C_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_3);
	Delay_us(10);
	return BitValue;
}

void MyI2C_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, GPIO_Pin_2 | GPIO_Pin_3);
}

void MyI2C_Start(void)
{
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(0);
}

void MyI2C_Stop(void)
{
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(1);
}

void MyI2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)
	{
		MyI2C_W_SDA(Byte & (0x80 >> i));
		MyI2C_W_SCL(1);
		MyI2C_W_SCL(0);
	}
}

uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;
	MyI2C_W_SDA(1);
	for (i = 0; i < 8; i ++)
	{
		MyI2C_W_SCL(1);
		if (MyI2C_R_SDA() == 1){Byte |= (0x80 >> i);}
		MyI2C_W_SCL(0);
	}
	return Byte;
}

void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SDA(AckBit);
	MyI2C_W_SCL(1);
	MyI2C_W_SCL(0);
}

uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t AckBit;
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	AckBit = MyI2C_R_SDA();
	MyI2C_W_SCL(0);
	return AckBit;
}



void GP8212_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_9, (BitAction)BitValue);
	Delay_us(10);
}

void GP8212_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_8, (BitAction)BitValue);
	Delay_us(10);
}

uint8_t GP8212_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8);
	Delay_us(10);
	return BitValue;
}

void GP8212_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9);
}

void GP8212_Start(void)
{
	GP8212_W_SDA(1);
	GP8212_W_SCL(1);
	GP8212_W_SDA(0);
	GP8212_W_SCL(0);
}

void GP8212_Stop(void)
{
	GP8212_W_SDA(0);
	GP8212_W_SCL(1);
	GP8212_W_SDA(1);
}

void GP8212_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)
	{
		GP8212_W_SDA(Byte & (0x80 >> i));
		GP8212_W_SCL(1);
		GP8212_W_SCL(0);
	}
}

uint8_t GP8212_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;
	GP8212_W_SDA(1);
	for (i = 0; i < 8; i ++)
	{
		GP8212_W_SCL(1);
		if (GP8212_R_SDA() == 1){Byte |= (0x80 >> i);}
		GP8212_W_SCL(0);
	}
	return Byte;
}

void GP8212_SendAck(uint8_t AckBit)
{
	GP8212_W_SDA(AckBit);
	GP8212_W_SCL(1);
	GP8212_W_SCL(0);
}

uint8_t GP8212_ReceiveAck(void)
{
	uint8_t AckBit;
	GP8212_W_SDA(1);
	GP8212_W_SCL(1);
	AckBit = MyI2C_R_SDA();
	GP8212_W_SCL(0);
	return AckBit;
}

