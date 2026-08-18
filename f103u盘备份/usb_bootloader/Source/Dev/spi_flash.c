#include "spi_flash.h"
 
 
u16 W25QXX_TYPE = W25Q128;
 
void delay_us(u16 nus)
{
	u16 i;
	for (i=0; i<nus; i++)
	{
		
	}
}
 
 
/**
 * @brief			SPI引脚的GPIO初始化
 * @detail
 * @param
 * @return
 * @note
*/
void W25QXX_Init(void)   
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
  FLASH_SPI_CS_APBxClock_FUN(FLASH_SPI_CS_CLK|FLASH_SPI_SCK_CLK|     //开启spi和gpio的时钟
							FLASH_SPI_MISO_PIN|FLASH_SPI_MOSI_PIN, ENABLE);
	
	
	/* 配置SPI的 CS引脚，普通IO即可 */
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_CS_PIN;     //初始化gpio口 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  //速度
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    //推挽
	GPIO_Init(FLASH_SPI_CS_PORT, &GPIO_InitStructure);   //应用配置
 
	/* 配置SPI的 SCK引脚*/
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_SCK_PIN;   //修改引脚号
	GPIO_Init(FLASH_SPI_SCK_PORT, &GPIO_InitStructure);  //其他没改，沿用上面的值
	
	/* 配置SPI的 MOSI引脚*/
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MOSI_PIN;
	GPIO_Init(FLASH_SPI_MOSI_PORT, &GPIO_InitStructure);
 
 
	/* 配置SPI的 MISO引脚*/
	GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  //改成了上拉输入，MISO(主收从发)
	GPIO_Init(FLASH_SPI_MISO_PORT, &GPIO_InitStructure);
 
	
	/* 停止信号 FLASH: CS引脚高电平*/
	SPI_FLASH_CS_HIGH();
	
	W25QXX_TYPE = W25QXX_ReadID();
//	if (W25QXX_TYPE == W25Q128)
//	{
//		printf("FlashID: %#x\r\n", W25QXX_TYPE);
//		printf("w25q128 ok\r\n");
//	}
}
 

/**
 * @brief 		交换一个字节(模式0)
 * @detail		
 * @param			dat：传入的数据，要同通过交换一个字节的时序，发送出去
 * @return		ByteReceive：通过交换一个字节，接收到的数据

 * @note			CPOL=0：空闲状态时，SCK位低电平
							CPHA=0：SCK第一个边沿(相当于上升沿)移入数据，第二个边沿(相当于下降沿)移出数据

 * @note	  	因为我们是软件SPI，不可能同时完成两个动作。所以我们是先SS下降沿，再移出数据。再SCK上升沿，再移入数据*/
uint8_t SPI1_ReadWriteByte(u8 dat)
{
	u8 i, read_data = 0;

	SPI_FLASH_SCLK_HIGH();

	for (i = 0; i< 8; i++)
	{
		read_data <<= 1;

		if (dat & 0x80)
		{
			SPI_FLASH_MOSI_HIGH();
		}
		else
		{
			SPI_FLASH_MOSI_LOW();
		}
		dat <<= 1;

		SPI_FLASH_SCLK_LOW();
		SPI_FLASH_SCLK_HIGH();

		if (GPIO_ReadInputDataBit(FLASH_SPI_MISO_PORT, FLASH_SPI_MISO_PIN))
		{
			read_data |= 0x01;
		}
	}

	SPI_FLASH_SCLK_HIGH();	
	
	return read_data;
}




/**
 * @brief 		读取W25QXX的状态寄存器
 * @detail
 * @param
 * @return
 * @note			BIT7  6   5   4   3   2   1   0
							SPR   RV  TB BP2 BP1 BP0 WEL BUSY
							SPR:默认0,状态寄存器保护位,配合WP使用
							TB,BP2,BP1,BP0:FLASH区域写保护设置
							WEL:写使能锁定
							BUSY:忙标记位(1,忙;0,空闲)
							默认:0x00
*/
uint8_t W25QXX_ReadSR(void)
{
	u8 byte = 0;
	SPI_FLASH_CS_LOW();		/* 选择FLASH: CS低电平 */
	SPI1_ReadWriteByte(W25X_ReadStatusReg);
	byte = SPI1_ReadWriteByte(0xff);
	SPI_FLASH_CS_HIGH();
	
	return byte;
}
 



/**	
 * @brief 		写W25QXX状态寄存器
 * @detail		只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
 * @param
 * @return
 * @note
*/
void W25QXX_Write_SR(u8 sr)
{
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_WriteStatusReg);
	SPI1_ReadWriteByte(sr);
	SPI_FLASH_CS_HIGH();
}
 

/**
 * @brief 		W25QXX写使能
 * @detail		将WEL置位
 * @param
 * @return
 * @note
*/
void W25QXX_Write_Enable(void)
{
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_WriteEnable);
	SPI_FLASH_CS_HIGH();
}


/**
 * @brief 		W25QXX写禁止
 * @detail		将WEL清零
 * @param
 * @return
 * @note
*/
void W25QXX_Write_Disable(void)
{
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_WriteDisable);
	SPI_FLASH_CS_HIGH();
}



/**
 * @brief 		读取芯片ID
 * @detail
 * @param
 * @return
 * @note			返回值如下:
							0XEF13,表示芯片型号为W25Q80
							0XEF14,表示芯片型号为W25Q16
							0XEF15,表示芯片型号为W25Q32
							0XEF16,表示芯片型号为W25Q64
							0XEF17,表示芯片型号为W25Q128
*/
u16 W25QXX_ReadID(void)
{
	u16 temp;
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_ManufactDeviceID);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	temp |= (SPI1_ReadWriteByte(0xFF)<<8);
	temp |= SPI1_ReadWriteByte(0xFF);
	SPI_FLASH_CS_HIGH();
	
	return temp;
}



/**
 * @brief 		读取SPI FLASH
 * @detail		在指定地址开始读取指定长度的数据
 * @param			pBuffer:数据存储区
 * @param			ReadAddr:开始读取的地址(24bit)
 * @param			NumByteToRead:要读取的字节数(最大65535)
 * @return
 * @note
*/
void W25QXX_Read(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
	u16 i;
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_ReadData);
	SPI1_ReadWriteByte((u8)(ReadAddr>>16));
	SPI1_ReadWriteByte((u8)(ReadAddr>>8));
	SPI1_ReadWriteByte((u8)ReadAddr);
	
	for (i=0; i<NumByteToRead; i++)
	{
		pBuffer[i] = SPI1_ReadWriteByte(0xff);
	}
	SPI_FLASH_CS_HIGH();
}



/**
 * @brief 		SPI在一页(0~65535)内写入少于256个字节的数据
 * @detail		在指定地址开始写入最大256字节的数据
 * @param			pBuffer:数据存储区
 * @param			WriteAddr:开始写入的地址(24bit)
 * @param			NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
 * @return
 * @note
*/
void W25QXX_Write_Page(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u16 i;
	
	if (NumByteToWrite > 256)	/* 参数检查 */
		NumByteToWrite = 256;
	
	W25QXX_Write_Enable();		//写使能
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_PageProgram);
	SPI1_ReadWriteByte((u8)(WriteAddr>>16));
	SPI1_ReadWriteByte((u8)(WriteAddr>>8));
	SPI1_ReadWriteByte((u8)WriteAddr);
	
	for (i=0; i<NumByteToWrite; i++)
	{
		SPI1_ReadWriteByte(pBuffer[i]);
	}
	SPI_FLASH_CS_HIGH();
	W25QXX_Wait_Busy();	//等待写入结束
	
}



/**
 * @brief 		无检验写SPI FLASH
 * @detail		必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
 * @param			pBuffer:数据存储区
 * @param			WriteAddr:开始写入的地址(24bit)
 * @param			NumByteToWrite:要写入的字节数(最大65535)
 * @return		CHECK OK

 * @note			具有自动换页功能
							在指定地址开始写入指定长度的数据,但是要确保地址不越界!
*/
void W25QXX_Write_NoCheck(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u16 pageremain;
	pageremain = 256 - WriteAddr%256;
	
	if (NumByteToWrite <= pageremain)
	{
		pageremain = NumByteToWrite;
	}
	
	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		
		if (pageremain == NumByteToWrite)
		{
			break;
		}
		else
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;
			
			NumByteToWrite -= pageremain;
			
			if (NumByteToWrite > 256)
			{
				pageremain = 256;
			}
			else
			{
				pageremain = NumByteToWrite;
			}
		}
	}
}




/**
 * @brief 		写SPI FLASH
 * @detail		在指定地址开始写入指定长度的数据
 * @param			pBuffer:数据存储区
 * @param			WriteAddr:开始写入的地址(24bit)
 * @param			NumByteToWrite:要写入的字节数(最大65535)
 * @return
 * @note			该函数带擦除操作!
*/
u8 SPI_FLASH_BUFFER[4096];
void W25QXX_Write(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{ 
	u32 secpos;
	u16 secoff;
	u16 secremain;	   
 	u16 i;    
	u8 * W25QXX_BUF;	
	printf("[SPI] Enter W25QXX_Write\r\n");
#ifdef OS_CRITICAL_METHOD	 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
    OS_CPU_SR cpu_sr=0;
	OS_ENTER_CRITICAL();		//停止调度
#endif	  

#ifdef SRAMIN
	W25QXX_BUF=mymalloc(SRAMIN,4096);//申请内存
	if(W25QXX_BUF==NULL)
	{
#ifdef OS_CRITICAL_METHOD	 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
		OS_EXIT_CRITICAL();		//使能调度
#endif		
		return ;//申请失败
	}
#else
   	W25QXX_BUF=SPI_FLASH_BUFFER;
#endif	     
 	secpos=WriteAddr/4096;//扇区地址  
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//不大于4096个字节
	while(1)
	{
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			W25QXX_Erase_Sector(secpos);		//擦除这个扇区
			for(i=0;i<secremain;i++)	   		//复制
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);//写入整个扇区  
		}else W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumByteToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuffer+=secremain;  				//指针偏移
			WriteAddr+=secremain;				//写地址偏移	   
		   	NumByteToWrite-=secremain;			//字节数递减
			if(NumByteToWrite>4096)secremain=4096;//下一个扇区还是写不完
			else secremain=NumByteToWrite;		//下一个扇区可以写完了
		}	 
	};	 
#ifdef SRAMIN
	myfree(SRAMIN,W25QXX_BUF);	//释放内存	 	 
#endif

#ifdef OS_CRITICAL_METHOD	 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
	OS_EXIT_CRITICAL();			//准许调度
#endif
}	

//void W25QXX_Write(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
//{
//	u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
//	
//	/* 取余，若writeAddr是SPI_FLASH_PageSize整数倍，运算结果Addr值为0 */
//	Addr = WriteAddr % SPI_FLASH_PageSize;
//	count  = SPI_FLASH_PageSize - Addr;	//差count个数据值，刚好可以对齐到页地址
//	NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;	//计算出要写多少整数页/
//	NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;	//计算出剩余不满一页的字节数
//	
//	if (Addr == 0)	//WriteAddr刚好按页对齐
//	{
//		if (NumOfPage == 0)	//不足一页
//		{
//			W25QXX_Write_Page(pBuffer, WriteAddr, NumByteToWrite);
//		}
//		else	//数据比一页多
//		{
//			while (NumOfPage--)	//先把整数页写完
//			{
//				W25QXX_Write_Page(pBuffer, WriteAddr, SPI_FLASH_PageSize);
//				WriteAddr += SPI_FLASH_PageSize;
//				pBuffer += SPI_FLASH_PageSize;
//			}
//			/* 若有多余的不足一页的数据，将其写完 */
//			W25QXX_Write_Page(pBuffer, WriteAddr, NumOfSingle);
//		}
//	}
//	else	//WriteAddr没有按页对齐
//	{
//		if (NumOfPage == 0)	//不足一页
//		{
//			/* 当前页可写的数据数不足以将数据写完 */
//			if (NumOfSingle > count)
//			{
//				temp = NumOfSingle - count;
//				//先将当前页写完
//				W25QXX_Write_Page(pBuffer, WriteAddr, count);
//				
//				WriteAddr += count;
//				pBuffer += count;
//				//再将剩余的数据写完
//				W25QXX_Write_Page(pBuffer, WriteAddr, temp);
//			}
//			else	/* 当前页可写的数据数可以将数据写完 */
//			{
//				W25QXX_Write_Page(pBuffer, WriteAddr, NumByteToWrite);
//			}
//		}
//		else	//要写的数据数大于一页
//		{
//			/*地址不对齐多出的count分开处理，不加入这个运算*/
//			NumByteToWrite -= count;
//			NumOfPage = NumByteToWrite / SPI_FLASH_PageSize;
//			NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;
//			
//			/* 先写完count个数据，为的是让下一次要写的地址对齐 */
//			W25QXX_Write_Page(pBuffer, WriteAddr, count);
//			WriteAddr += count;
//			pBuffer += count;
//			
//			while (NumOfPage--)
//			{
//				W25QXX_Write_Page(pBuffer, WriteAddr, SPI_FLASH_PageSize);
//				WriteAddr += SPI_FLASH_PageSize;
//				pBuffer += SPI_FLASH_PageSize;
//			}
//			
//			if (NumOfSingle != 0)
//			{
//				W25QXX_Write_Page(pBuffer, WriteAddr, NumOfSingle);
//			}
//		}
//	}
//}	




/**
 * @brief 		擦除整个芯片
 * @detail		等待时间超长...
 * @param
 * @return
 * @note
*/
void W25QXX_Erase_Chip(void)
{
	/* 发送FLASH写使能命令 */
	W25QXX_Write_Enable();
	W25QXX_Wait_Busy();
 
	/* 整块 Erase */
	SPI_FLASH_CS_LOW();		//选择FLASH: CS低电平
	SPI1_ReadWriteByte(W25X_ChipErase);	// 发送整块擦除指令
	SPI_FLASH_CS_HIGH();	//停止信号 FLASH: CS 高电平
 
	/* 等待擦除完毕*/
	W25QXX_Wait_Busy();
}




/**
 * @brief 		擦除一个扇区 (4KB)
 * @detail
 * @param			Dst_Addr:扇区地址 根据实际容量设置
 * @return
 * @note			擦除一个扇区的最少时间:150ms
*/
void W25QXX_Erase_Sector(u32 Dst_Addr)
{
	W25QXX_Write_Enable();
	W25QXX_Wait_Busy();

	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_SectorErase);
	SPI1_ReadWriteByte((u8)(Dst_Addr >> 16));
	SPI1_ReadWriteByte((u8)(Dst_Addr >> 8));
	SPI1_ReadWriteByte((u8)Dst_Addr);
	SPI_FLASH_CS_HIGH();
	W25QXX_Wait_Busy();
}

/**
 * @brief 		擦除一个块 (64KB)
 * @detail
 * @param			Dst_Addr:块地址 (64KB对齐)
 * @return
 * @note			擦除一个64KB块的最少时间:400ms
*/
void W25QXX_Erase_Block(u32 Dst_Addr)
{
	W25QXX_Write_Enable();
	W25QXX_Wait_Busy();

	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_BlockErase);
	SPI1_ReadWriteByte((u8)(Dst_Addr >> 16));
	SPI1_ReadWriteByte((u8)(Dst_Addr >> 8));
	SPI1_ReadWriteByte((u8)Dst_Addr);
	SPI_FLASH_CS_HIGH();
	W25QXX_Wait_Busy();
}



/**
 * @brief 		等待空闲
 * @detail
 * @param
 * @return
 * @note
*/
void W25QXX_Wait_Busy(void)
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01);	// 等待BUSY位清空
}





/**
 * @brief 		进入掉电模式
 * @detail
 * @param
 * @return
 * @note
*/
void W25QXX_PowerDown(void)
{
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_PowerDown);	
	SPI_FLASH_CS_HIGH();
	delay_us(3);
}




/**
 * @brief 		唤醒
 * @detail
 * @param
 * @return
 * @note
*/
void W25QXX_WAKEUP(void)
{
	SPI_FLASH_CS_LOW();
	SPI1_ReadWriteByte(W25X_ReleasePowerDown);	
	SPI_FLASH_CS_HIGH();
	delay_us(3);
}
