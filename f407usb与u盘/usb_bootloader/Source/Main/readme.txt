/*
 *	使用资源：	c8t6+标准库+W25Q128+Fatfs+USB
 *	实现效果：	(1)在WQ系列存储介上，挂载Fatfs文件系统，并通过文件系统，去创建文件夹，写文件等操作。
 *			 	(2)通过初始化板子上自带的usb外设,将WQ系列存储器 模拟成u盘，可视化查看里面的内容
 *	
 *  注意：		对于扇区配置，只有这一套成功(扇区大小=4096，扇区数量=4096，一次擦除的扇区数量=1)
 *				其他的配置还未成功，应该是我个人问题。
 *
 *	设计到扇区分配的文件：	fatfs_system.c		diskio.c		mass_mal.c		memory.c
 *
 *
 *
*/






csdn移植教程
 https://blog.csdn.net/asher__zhou/article/details/105519209?fromshare=blogdetail&sharetype=blogdetail&sharerId=105519209&sharerefer=PC&sharesource=let_we_go_&sharefrom=from_link