#ifndef __IIC_IMIT_H
#define __IIC_IMIT_H
#include "system.h"
	   		   
//IO方向设置
#define SDA_IN()  {GPIOB->MODER&=~(3<<(1*2));GPIOB->MODER|=0<<1*2;}	//PB1输入模式
#define SDA_OUT() {GPIOB->MODER&=~(3<<(1*2));GPIOB->MODER|=1<<1*2;} //PB1输出模式
//IO操作函数	 
#define IIC_SCL    PBout(0) //SCL
#define IIC_SDA    PBout(1) //SDA	 
#define READ_SDA   PBin(1)  //输入SDA 


//IIC所有操作函数
extern void imit_IIC_Init(void);                //初始化IIC的IO口				 
extern void imit_IIC_Start(void);				//发送IIC开始信号
extern void imit_IIC_Stop(void);	  			//发送IIC停止信号
extern void imit_IIC_Send_Byte(u8 txd);			//IIC发送一个字节
extern u8 imit_IIC_Read_Byte(unsigned char ack);//IIC读取一个字节
extern u8 imit_IIC_Wait_Ack(void); 				//IIC等待ACK信号
extern void imit_IIC_Ack(void);					//IIC发送ACK信号
extern void imit_IIC_NAck(void);				//IIC不发送ACK信号


#endif

