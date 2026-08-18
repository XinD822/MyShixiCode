/**
 * @file main.c
 * @brief USB拖拽升级应用程序主文件
 * 
 * 功能说明：
 * 1. 通过USB MSC（大容量存储类）将W25Q128模拟为U盘
 * 2. 用户将firmware.bin拖入U盘后，APP自动检测并写入Slot A
 * 3. 设置升级标志后重启，由Bootloader完成最终烧写
 * 
 * 技术架构：
 * USB 2.0 Full-Speed + MSC-BOT + SCSI协议栈
 * 本质是一个SCSI硬盘，通过BOT协议在Bulk端点传输数据
 * 底层SCSI命令操作W25Q128 Flash
 * 
 * 参考资料：
 * CSDN移植教程：
 * https://blog.csdn.net/asher__zhou/article/details/105519209
 * 
 * FatFS与USB同时访问冲突解决方案：
 * https://www.cnblogs.com/cage666/p/9219458.html
 * 
 * 升级流程：
 *   PC拖入firmware.bin → FatFS写入数据区 → APP搬到Slot A → 设标志 → 重启
 *   Bootloader读标志 → 备份当前APP到Slot B → 拷贝Slot A到内部Flash → 重启
 */

#include "config.h"

/**
 * @brief 主函数 - 系统入口
 * 
 * 执行流程：
 * 1. System_Init() - 初始化系统时钟、外设、USB等
 * 2. LED_Init() - 初始化LED指示灯
 * 3. 进入主循环：
 *    - LED闪烁指示系统运行状态
 *    - Task_Run()处理后台任务（USB、升级检测、文件系统等）
 * 
 * @return 无（嵌入式系统永不返回）
 */
int main()
{
    /* 系统初始化：时钟(72MHz)、GPIO、USART、SPI、USB等 */
    System_Init();
    
    /* LED初始化：配置LED引脚为输出模式 */
    LED_Init();
    
    /* 调试用：擦除配置区（解决卡在rollback循环的问题，用完注释掉） */
    // W25QXX_Erase_Sector(CONFIG_AREA_ADDR);
    // printf("[SYS] Config area erased\r\n");
    
    /* 调试用：需要格式化Flash时取消注释，格式化完后注释掉 */
    // Format_Flash();
    
    /* 主循环 */
    while(1)
    {
        /* LED闪烁：每500ms翻转一次，指示系统正常运行 */
        LED1 = !LED1;
        Delay_ms(500);
        
        /* 任务调度器：处理所有后台任务 */
        /* 包括：USB通信、固件文件检测、升级处理、FatFS操作等 */
        Task_Run();
    }
}
