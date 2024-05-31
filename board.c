/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */
 
#include <rthw.h>
#include <rtthread.h>
#include <msp430.h>

// Holds the system core clock, which is the system clock 
// frequency supplied to the SysTick timer and the processor 
// core clock.
rt_uint32_t SystemCoreClock;

// Updates the variable SystemCoreClock and must be called
// whenever the core clock is changed during program execution.
void SystemCoreClockUpdate(void)
{
    // IO初始化
    P1OUT = 0x00;
    P2OUT = 0x00;
    P3OUT = 0x00;
    P4OUT = 0x00;
    P5OUT = 0x00;
    P6OUT = 0x00;
    PAOUT = 0x0000;
    PBOUT = 0x0000;
    PCOUT = 0x0000;
    PJOUT = 0x0000;
    P1DIR = 0xFF;
    P2DIR = 0xFF;
    P3DIR = 0xFF;
    P4DIR = 0xFF;
    P5DIR = 0xFF;
    P6DIR = 0xFF;
    PADIR = 0xFFFF;
    PBDIR = 0xFFFF;
    PCDIR = 0xFFFF;
    PJDIR = 0xFFFF;
    PM5CTL0 &= ~LOCKLPM5;

    // 时钟设置
    // XT1引脚
    P2SEL1 |= BIT6;
    P2SEL0 &= ~BIT6;
    P2SEL1 |= BIT7;
    P2SEL0 &= ~BIT7;
    CSCTL5 = VLOAUTOOFF_1 | SMCLKOFF_0 | DIVM__1;  // 自动关闭VLO振荡器；开启\关闭SMCLK；MCLK 1分频
    CSCTL6 = XT1FAULTOFF_0 | DIVA__1 | XT1DRIVE_0 | XTS_0 | XT1AUTOOFF_0;  // XT1错误监测切换；ACLK 1分频；XT1最低电流消耗模式；XT1低频模式；XT1开启
    do  // 发生故障就循环，直到故障被清除退出循环，也可以写为：while(SFRIFG1 & OFIFG)监测所有振荡器错误
    {
        CSCTL7 &= ~(XT1OFFG|DCOFFG); // 清除时钟错误存在标志位
        SFRIFG1 &= ~OFIFG; // 清除时钟错误中断标志位
    }while(SFRIFG1 & OFIFG);
    CSCTL4 = SELA__XT1CLK | SELMS__DCOCLKDIV;  // ACLK、SMCLK、MCLK时钟源设置

    SystemCoreClock = 32768;  // ACLK
}

static rt_uint32_t _SysTick_Config(rt_uint32_t ticks)
{
    if(ticks < 128)
        WDTCTL = WDTPW|WDTTMSEL_1|WDTSSEL__ACLK|WDTCNTCL|WDTIS__64;
    else if(ticks < 1024)
        WDTCTL = WDTPW|WDTTMSEL_1|WDTSSEL__ACLK|WDTCNTCL|WDTIS__512;
    else
        WDTCTL = WDTPW|WDTTMSEL_1|WDTSSEL__ACLK|WDTCNTCL|WDTIS__8192;

    SFRIFG1 &= ~WDTIFG;
    SFRIE1 |= WDTIE;

    return 0;
}

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 2048
static rt_uint8_t rt_heap[RT_HEAP_SIZE];     // heap default size: 4K(1024 * 4)
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{
    /* System Clock Update */
    SystemCoreClockUpdate();
    
    /* System Tick Configuration */
    _SysTick_Config(SystemCoreClock/RT_TICK_PER_SECOND);

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}
