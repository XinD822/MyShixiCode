/**
 * @brief Bootloader系统初始化
 * 
 * Bootloader只需要：
 * - 串口调试
 * - SPI Flash驱动
 * - TIM2延时
 * - 读取升级状态并执行相应操作
 * - 跳转到APP
 */

#include "config.h"

void System_Init(void)
{
    SCB->VTOR = BOOTLOADER_ADDR;  // 必须加：向量表指向 bootloader
    CPU_INT_ENABLE();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    TIM2_Init();
    Task_Time_Init();
    UsartInit();
    W25QXX_Init();
}
