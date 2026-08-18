#ifndef __TIM2_H__
#define __TIM2_H__

#include "config.h"

#define MAXTIMER				15

#define Delay_Timer			Task_Timer[0]
#define ATaskTimer			Task_Timer[1]
#define BTaskTimer			Task_Timer[2]
#define CTaskTimer			Task_Timer[3]
#define DTaskTimer			Task_Timer[4]



extern u32 Task_Timer[MAXTIMER];



void TIM2_Init(void);
void Task_Time_Init(void);

#endif


