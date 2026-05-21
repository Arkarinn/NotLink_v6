#include "stm32h7xx.h"
#include <stdint.h>

extern uint32_t _sidata; /* data段初始值所在地址 */
extern uint32_t _sdata;  /* data段开头 */
extern uint32_t _edata;  /* data段结尾 */
extern uint32_t _sbss;   /* bss段开头 */
extern uint32_t _ebss;   /* bss段结尾 */
extern uint32_t _estack; /* 堆栈结尾 */

extern uint32_t _siitcm; /* itcm段初始值所在地址 */
extern uint32_t _sitcm;  /* itcm段开头 */
extern uint32_t _eitcm;  /* itcm段结尾 */

extern void SystemInit(void);
extern void ExitRun0Mode(void);

extern void __libc_init_array(void);
extern int main(void);

/**
 * @brief 复制数据
 *
 * @param addr_init     初始值所在地址
 * @param addr_start    开始地址
 * @param addr_end      结束地址
 */
void CopyData(uint32_t addr_init, uint32_t addr_start, uint32_t addr_end)
{
    /* 链接时4字节对齐 */
    while (addr_start < addr_end)
    {
        *((uint32_t *)addr_start) = *((uint32_t *)addr_init);
        addr_start += 4;
        addr_init += 4;
    }
}

/**
 * @brief 填充0
 *
 * @param addr_start
 * @param addr_end
 */
void FillZero(uint32_t addr_start, uint32_t addr_end)
{
    /* 链接时4字节对齐 */
    while (addr_start < addr_end)
    {
        *((uint32_t *)addr_start) = 0UL;
        addr_start += 4;
    }
}

void Reset_Handler(void)
{
    ExitRun0Mode();
    SystemInit();

    /* 复制变量 */
    CopyData((uint32_t)&_sidata, (uint32_t)&_sdata, (uint32_t)&_edata);
    FillZero((uint32_t)&_sbss, (uint32_t)&_ebss);

    /* 复制ITCM数据 */
    CopyData((uint32_t)&_siitcm, (uint32_t)&_sitcm, (uint32_t)&_eitcm);

    __libc_init_array();
    main();

    while (1)
    {
    }
}

/* 默认中断处理函数的弱定义 */

__WEAK void NMI_Handler(void)
{
}

__WEAK void HardFault_Handler(void)
{
}

__WEAK void MemManage_Handler(void)
{
}

__WEAK void BusFault_Handler(void)
{
}

__WEAK void UsageFault_Handler(void)
{
}

__WEAK void SVC_Handler(void)
{
}

__WEAK void DebugMon_Handler(void)
{
}

__WEAK void PendSV_Handler(void)
{
}

__WEAK void SysTick_Handler(void)
{
}

__WEAK void WWDG_IRQHandler(void)
{
}

__WEAK void PVD_AVD_IRQHandler(void)
{
}

__WEAK void TAMP_STAMP_IRQHandler(void)
{
}

__WEAK void RTC_WKUP_IRQHandler(void)
{
}

__WEAK void FLASH_IRQHandler(void)
{
}

__WEAK void RCC_IRQHandler(void)
{
}

__WEAK void EXTI0_IRQHandler(void)
{
}

__WEAK void EXTI1_IRQHandler(void)
{
}

__WEAK void EXTI2_IRQHandler(void)
{
}

__WEAK void EXTI3_IRQHandler(void)
{
}

__WEAK void EXTI4_IRQHandler(void)
{
}

__WEAK void DMA1_Stream0_IRQHandler(void)
{
}

__WEAK void DMA1_Stream1_IRQHandler(void)
{
}

__WEAK void DMA1_Stream2_IRQHandler(void)
{
}

__WEAK void DMA1_Stream3_IRQHandler(void)
{
}

__WEAK void DMA1_Stream4_IRQHandler(void)
{
}

__WEAK void DMA1_Stream5_IRQHandler(void)
{
}

__WEAK void DMA1_Stream6_IRQHandler(void)
{
}

__WEAK void ADC_IRQHandler(void)
{
}

__WEAK void FDCAN1_IT0_IRQHandler(void)
{
}

__WEAK void FDCAN2_IT0_IRQHandler(void)
{
}

__WEAK void FDCAN1_IT1_IRQHandler(void)
{
}

__WEAK void FDCAN2_IT1_IRQHandler(void)
{
}

__WEAK void EXTI9_5_IRQHandler(void)
{
}

__WEAK void TIM1_BRK_IRQHandler(void)
{
}

__WEAK void TIM1_UP_IRQHandler(void)
{
}

__WEAK void TIM1_TRG_COM_IRQHandler(void)
{
}

__WEAK void TIM1_CC_IRQHandler(void)
{
}

__WEAK void TIM2_IRQHandler(void)
{
}

__WEAK void TIM3_IRQHandler(void)
{
}

__WEAK void TIM4_IRQHandler(void)
{
}

__WEAK void I2C1_EV_IRQHandler(void)
{
}

__WEAK void I2C1_ER_IRQHandler(void)
{
}

__WEAK void I2C2_EV_IRQHandler(void)
{
}

__WEAK void I2C2_ER_IRQHandler(void)
{
}

__WEAK void SPI1_IRQHandler(void)
{
}

__WEAK void SPI2_IRQHandler(void)
{
}

__WEAK void USART1_IRQHandler(void)
{
}

__WEAK void USART2_IRQHandler(void)
{
}

__WEAK void USART3_IRQHandler(void)
{
}

__WEAK void EXTI15_10_IRQHandler(void)
{
}

__WEAK void RTC_Alarm_IRQHandler(void)
{
}

__WEAK void TIM8_BRK_TIM12_IRQHandler(void)
{
}

__WEAK void TIM8_UP_TIM13_IRQHandler(void)
{
}

__WEAK void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
}

__WEAK void TIM8_CC_IRQHandler(void)
{
}

__WEAK void DMA1_Stream7_IRQHandler(void)
{
}

__WEAK void FMC_IRQHandler(void)
{
}

__WEAK void SDMMC1_IRQHandler(void)
{
}

__WEAK void TIM5_IRQHandler(void)
{
}

__WEAK void SPI3_IRQHandler(void)
{
}

__WEAK void UART4_IRQHandler(void)
{
}

__WEAK void UART5_IRQHandler(void)
{
}

__WEAK void TIM6_DAC_IRQHandler(void)
{
}

__WEAK void TIM7_IRQHandler(void)
{
}

__WEAK void DMA2_Stream0_IRQHandler(void)
{
}

__WEAK void DMA2_Stream1_IRQHandler(void)
{
}

__WEAK void DMA2_Stream2_IRQHandler(void)
{
}

__WEAK void DMA2_Stream3_IRQHandler(void)
{
}

__WEAK void DMA2_Stream4_IRQHandler(void)
{
}

__WEAK void ETH_IRQHandler(void)
{
}

__WEAK void ETH_WKUP_IRQHandler(void)
{
}

__WEAK void FDCAN_CAL_IRQHandler(void)
{
}

__WEAK void DMA2_Stream5_IRQHandler(void)
{
}

__WEAK void DMA2_Stream6_IRQHandler(void)
{
}

__WEAK void DMA2_Stream7_IRQHandler(void)
{
}

__WEAK void USART6_IRQHandler(void)
{
}

__WEAK void I2C3_EV_IRQHandler(void)
{
}

__WEAK void I2C3_ER_IRQHandler(void)
{
}

__WEAK void OTG_HS_EP1_OUT_IRQHandler(void)
{
}

__WEAK void OTG_HS_EP1_IN_IRQHandler(void)
{
}

__WEAK void OTG_HS_WKUP_IRQHandler(void)
{
}

__WEAK void OTG_HS_IRQHandler(void)
{
}

__WEAK void DCMI_IRQHandler(void)
{
}

__WEAK void CRYP_IRQHandler(void)
{
}

__WEAK void HASH_RNG_IRQHandler(void)
{
}

__WEAK void FPU_IRQHandler(void)
{
}

__WEAK void UART7_IRQHandler(void)
{
}

__WEAK void UART8_IRQHandler(void)
{
}

__WEAK void SPI4_IRQHandler(void)
{
}

__WEAK void SPI5_IRQHandler(void)
{
}

__WEAK void SPI6_IRQHandler(void)
{
}

__WEAK void SAI1_IRQHandler(void)
{
}

__WEAK void LTDC_IRQHandler(void)
{
}

__WEAK void LTDC_ER_IRQHandler(void)
{
}

__WEAK void DMA2D_IRQHandler(void)
{
}

__WEAK void SAI2_IRQHandler(void)
{
}

__WEAK void QUADSPI_IRQHandler(void)
{
}

__WEAK void LPTIM1_IRQHandler(void)
{
}

__WEAK void CEC_IRQHandler(void)
{
}

__WEAK void I2C4_EV_IRQHandler(void)
{
}

__WEAK void I2C4_ER_IRQHandler(void)
{
}

__WEAK void SPDIF_RX_IRQHandler(void)
{
}

__WEAK void OTG_FS_EP1_OUT_IRQHandler(void)
{
}

__WEAK void OTG_FS_EP1_IN_IRQHandler(void)
{
}

__WEAK void OTG_FS_WKUP_IRQHandler(void)
{
}

__WEAK void OTG_FS_IRQHandler(void)
{
}

__WEAK void DMAMUX1_OVR_IRQHandler(void)
{
}

__WEAK void HRTIM1_Master_IRQHandler(void)
{
}

__WEAK void HRTIM1_TIMA_IRQHandler(void)
{
}

__WEAK void HRTIM1_TIMB_IRQHandler(void)
{
}

__WEAK void HRTIM1_TIMC_IRQHandler(void)
{
}

__WEAK void HRTIM1_TIMD_IRQHandler(void)
{
}

__WEAK void HRTIM1_TIME_IRQHandler(void)
{
}

__WEAK void HRTIM1_FLT_IRQHandler(void)
{
}

__WEAK void DFSDM1_FLT0_IRQHandler(void)
{
}

__WEAK void DFSDM1_FLT1_IRQHandler(void)
{
}

__WEAK void DFSDM1_FLT2_IRQHandler(void)
{
}

__WEAK void DFSDM1_FLT3_IRQHandler(void)
{
}

__WEAK void SAI3_IRQHandler(void)
{
}

__WEAK void SWPMI1_IRQHandler(void)
{
}

__WEAK void TIM15_IRQHandler(void)
{
}

__WEAK void TIM16_IRQHandler(void)
{
}

__WEAK void TIM17_IRQHandler(void)
{
}

__WEAK void MDIOS_WKUP_IRQHandler(void)
{
}

__WEAK void MDIOS_IRQHandler(void)
{
}

__WEAK void JPEG_IRQHandler(void)
{
}

__WEAK void MDMA_IRQHandler(void)
{
}

__WEAK void SDMMC2_IRQHandler(void)
{
}

__WEAK void HSEM1_IRQHandler(void)
{
}

__WEAK void ADC3_IRQHandler(void)
{
}

__WEAK void DMAMUX2_OVR_IRQHandler(void)
{
}

__WEAK void BDMA_Channel0_IRQHandler(void)
{
}

__WEAK void BDMA_Channel1_IRQHandler(void)
{
}

__WEAK void BDMA_Channel2_IRQHandler(void)
{
}

__WEAK void BDMA_Channel3_IRQHandler(void)
{
}

__WEAK void BDMA_Channel4_IRQHandler(void)
{
}

__WEAK void BDMA_Channel5_IRQHandler(void)
{
}

__WEAK void BDMA_Channel6_IRQHandler(void)
{
}

__WEAK void BDMA_Channel7_IRQHandler(void)
{
}

__WEAK void COMP1_IRQHandler(void)
{
}

__WEAK void LPTIM2_IRQHandler(void)
{
}

__WEAK void LPTIM3_IRQHandler(void)
{
}

__WEAK void LPTIM4_IRQHandler(void)
{
}

__WEAK void LPTIM5_IRQHandler(void)
{
}

__WEAK void LPUART1_IRQHandler(void)
{
}

__WEAK void CRS_IRQHandler(void)
{
}

__WEAK void ECC_IRQHandler(void)
{
}

__WEAK void SAI4_IRQHandler(void)
{
}

__WEAK void WAKEUP_PIN_IRQHandler(void)
{
}

/* 中断向量表 */
void (*const __USED g_pfnVectors[256])(void) __attribute__((section(".isr_vector"))) = {
    (void (*)(void))((uint32_t)&_estack),
    Reset_Handler,

    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    /* External Interrupts */
    WWDG_IRQHandler,               /* Window WatchDog              */
    PVD_AVD_IRQHandler,            /* PVD/AVD through EXTI Line detection */
    TAMP_STAMP_IRQHandler,         /* Tamper and TimeStamps through the EXTI line */
    RTC_WKUP_IRQHandler,           /* RTC Wakeup through the EXTI line */
    FLASH_IRQHandler,              /* FLASH                        */
    RCC_IRQHandler,                /* RCC                          */
    EXTI0_IRQHandler,              /* EXTI Line0                   */
    EXTI1_IRQHandler,              /* EXTI Line1                   */
    EXTI2_IRQHandler,              /* EXTI Line2                   */
    EXTI3_IRQHandler,              /* EXTI Line3                   */
    EXTI4_IRQHandler,              /* EXTI Line4                   */
    DMA1_Stream0_IRQHandler,       /* DMA1 Stream 0                */
    DMA1_Stream1_IRQHandler,       /* DMA1 Stream 1                */
    DMA1_Stream2_IRQHandler,       /* DMA1 Stream 2                */
    DMA1_Stream3_IRQHandler,       /* DMA1 Stream 3                */
    DMA1_Stream4_IRQHandler,       /* DMA1 Stream 4                */
    DMA1_Stream5_IRQHandler,       /* DMA1 Stream 5                */
    DMA1_Stream6_IRQHandler,       /* DMA1 Stream 6                */
    ADC_IRQHandler,                /* ADC1, ADC2 and ADC3s         */
    FDCAN1_IT0_IRQHandler,         /* FDCAN1 interrupt line 0      */
    FDCAN2_IT0_IRQHandler,         /* FDCAN2 interrupt line 0      */
    FDCAN1_IT1_IRQHandler,         /* FDCAN1 interrupt line 1      */
    FDCAN2_IT1_IRQHandler,         /* FDCAN2 interrupt line 1      */
    EXTI9_5_IRQHandler,            /* External Line[9:5]s          */
    TIM1_BRK_IRQHandler,           /* TIM1 Break interrupt         */
    TIM1_UP_IRQHandler,            /* TIM1 Update interrupt        */
    TIM1_TRG_COM_IRQHandler,       /* TIM1 Trigger and Commutation interrupt */
    TIM1_CC_IRQHandler,            /* TIM1 Capture Compare         */
    TIM2_IRQHandler,               /* TIM2                         */
    TIM3_IRQHandler,               /* TIM3                         */
    TIM4_IRQHandler,               /* TIM4                         */
    I2C1_EV_IRQHandler,            /* I2C1 Event                   */
    I2C1_ER_IRQHandler,            /* I2C1 Error                   */
    I2C2_EV_IRQHandler,            /* I2C2 Event                   */
    I2C2_ER_IRQHandler,            /* I2C2 Error                   */
    SPI1_IRQHandler,               /* SPI1                         */
    SPI2_IRQHandler,               /* SPI2                         */
    USART1_IRQHandler,             /* USART1                       */
    USART2_IRQHandler,             /* USART2                       */
    USART3_IRQHandler,             /* USART3                       */
    EXTI15_10_IRQHandler,          /* External Line[15:10]s        */
    RTC_Alarm_IRQHandler,          /* RTC Alarm (A and B) through EXTI Line */
    0,                             /* Reserved                     */
    TIM8_BRK_TIM12_IRQHandler,     /* TIM8 Break and TIM12         */
    TIM8_UP_TIM13_IRQHandler,      /* TIM8 Update and TIM13        */
    TIM8_TRG_COM_TIM14_IRQHandler, /* TIM8 Trigger and Commutation and TIM14 */
    TIM8_CC_IRQHandler,            /* TIM8 Capture Compare         */
    DMA1_Stream7_IRQHandler,       /* DMA1 Stream7                 */
    FMC_IRQHandler,                /* FMC                          */
    SDMMC1_IRQHandler,             /* SDMMC1                       */
    TIM5_IRQHandler,               /* TIM5                         */
    SPI3_IRQHandler,               /* SPI3                         */
    UART4_IRQHandler,              /* UART4                        */
    UART5_IRQHandler,              /* UART5                        */
    TIM6_DAC_IRQHandler,           /* TIM6 and DAC1&2 underrun errors */
    TIM7_IRQHandler,               /* TIM7                         */
    DMA2_Stream0_IRQHandler,       /* DMA2 Stream 0                */
    DMA2_Stream1_IRQHandler,       /* DMA2 Stream 1                */
    DMA2_Stream2_IRQHandler,       /* DMA2 Stream 2                */
    DMA2_Stream3_IRQHandler,       /* DMA2 Stream 3                */
    DMA2_Stream4_IRQHandler,       /* DMA2 Stream 4                */
    ETH_IRQHandler,                /* Ethernet                     */
    ETH_WKUP_IRQHandler,           /* Ethernet Wakeup through EXTI line */
    FDCAN_CAL_IRQHandler,          /* FDCAN calibration unit interrupt*/
    0,                             /* Reserved                     */
    0,                             /* Reserved                     */
    0,                             /* Reserved                     */
    0,                             /* Reserved                     */
    DMA2_Stream5_IRQHandler,       /* DMA2 Stream 5                */
    DMA2_Stream6_IRQHandler,       /* DMA2 Stream 6                */
    DMA2_Stream7_IRQHandler,       /* DMA2 Stream 7                */
    USART6_IRQHandler,             /* USART6                       */
    I2C3_EV_IRQHandler,            /* I2C3 event                   */
    I2C3_ER_IRQHandler,            /* I2C3 error                   */
    OTG_HS_EP1_OUT_IRQHandler,     /* USB OTG HS End Point 1 Out   */
    OTG_HS_EP1_IN_IRQHandler,      /* USB OTG HS End Point 1 In    */
    OTG_HS_WKUP_IRQHandler,        /* USB OTG HS Wakeup through EXTI */
    OTG_HS_IRQHandler,             /* USB OTG HS                   */
    DCMI_IRQHandler,               /* DCMI                         */
    CRYP_IRQHandler,               /* Crypto                       */
    HASH_RNG_IRQHandler,           /* Hash and Rng                 */
    FPU_IRQHandler,                /* FPU                          */
    UART7_IRQHandler,              /* UART7                        */
    UART8_IRQHandler,              /* UART8                        */
    SPI4_IRQHandler,               /* SPI4                         */
    SPI5_IRQHandler,               /* SPI5                         */
    SPI6_IRQHandler,               /* SPI6                         */
    SAI1_IRQHandler,               /* SAI1                         */
    LTDC_IRQHandler,               /* LTDC                         */
    LTDC_ER_IRQHandler,            /* LTDC error                   */
    DMA2D_IRQHandler,              /* DMA2D                        */
    SAI2_IRQHandler,               /* SAI2                         */
    QUADSPI_IRQHandler,            /* QUADSPI                      */
    LPTIM1_IRQHandler,             /* LPTIM1                       */
    CEC_IRQHandler,                /* HDMI_CEC                     */
    I2C4_EV_IRQHandler,            /* I2C4 Event                   */
    I2C4_ER_IRQHandler,            /* I2C4 Error                   */
    SPDIF_RX_IRQHandler,           /* SPDIF_RX                     */
    OTG_FS_EP1_OUT_IRQHandler,     /* USB OTG FS End Point 1 Out   */
    OTG_FS_EP1_IN_IRQHandler,      /* USB OTG FS End Point 1 In    */
    OTG_FS_WKUP_IRQHandler,        /* USB OTG FS Wakeup through EXTI */
    OTG_FS_IRQHandler,             /* USB OTG FS                   */
    DMAMUX1_OVR_IRQHandler,        /* DMAMUX1 Overrun interrupt    */
    HRTIM1_Master_IRQHandler,      /* HRTIM Master Timer global Interrupt */
    HRTIM1_TIMA_IRQHandler,        /* HRTIM Timer A global Interrupt */
    HRTIM1_TIMB_IRQHandler,        /* HRTIM Timer B global Interrupt */
    HRTIM1_TIMC_IRQHandler,        /* HRTIM Timer C global Interrupt */
    HRTIM1_TIMD_IRQHandler,        /* HRTIM Timer D global Interrupt */
    HRTIM1_TIME_IRQHandler,        /* HRTIM Timer E global Interrupt */
    HRTIM1_FLT_IRQHandler,         /* HRTIM Fault global Interrupt   */
    DFSDM1_FLT0_IRQHandler,        /* DFSDM Filter0 Interrupt        */
    DFSDM1_FLT1_IRQHandler,        /* DFSDM Filter1 Interrupt        */
    DFSDM1_FLT2_IRQHandler,        /* DFSDM Filter2 Interrupt        */
    DFSDM1_FLT3_IRQHandler,        /* DFSDM Filter3 Interrupt        */
    SAI3_IRQHandler,               /* SAI3 global Interrupt          */
    SWPMI1_IRQHandler,             /* Serial Wire Interface 1 global interrupt */
    TIM15_IRQHandler,              /* TIM15 global Interrupt      */
    TIM16_IRQHandler,              /* TIM16 global Interrupt      */
    TIM17_IRQHandler,              /* TIM17 global Interrupt      */
    MDIOS_WKUP_IRQHandler,         /* MDIOS Wakeup  Interrupt     */
    MDIOS_IRQHandler,              /* MDIOS global Interrupt      */
    JPEG_IRQHandler,               /* JPEG global Interrupt       */
    MDMA_IRQHandler,               /* MDMA global Interrupt       */
    0,                             /* Reserved                    */
    SDMMC2_IRQHandler,             /* SDMMC2 global Interrupt     */
    HSEM1_IRQHandler,              /* HSEM1 global Interrupt      */
    0,                             /* Reserved                    */
    ADC3_IRQHandler,               /* ADC3 global Interrupt       */
    DMAMUX2_OVR_IRQHandler,        /* DMAMUX Overrun interrupt    */
    BDMA_Channel0_IRQHandler,      /* BDMA Channel 0 global Interrupt */
    BDMA_Channel1_IRQHandler,      /* BDMA Channel 1 global Interrupt */
    BDMA_Channel2_IRQHandler,      /* BDMA Channel 2 global Interrupt */
    BDMA_Channel3_IRQHandler,      /* BDMA Channel 3 global Interrupt */
    BDMA_Channel4_IRQHandler,      /* BDMA Channel 4 global Interrupt */
    BDMA_Channel5_IRQHandler,      /* BDMA Channel 5 global Interrupt */
    BDMA_Channel6_IRQHandler,      /* BDMA Channel 6 global Interrupt */
    BDMA_Channel7_IRQHandler,      /* BDMA Channel 7 global Interrupt */
    COMP1_IRQHandler,              /* COMP1 global Interrupt     */
    LPTIM2_IRQHandler,             /* LP TIM2 global interrupt   */
    LPTIM3_IRQHandler,             /* LP TIM3 global interrupt   */
    LPTIM4_IRQHandler,             /* LP TIM4 global interrupt   */
    LPTIM5_IRQHandler,             /* LP TIM5 global interrupt   */
    LPUART1_IRQHandler,            /* LP UART1 interrupt         */
    0,                             /* Reserved                   */
    CRS_IRQHandler,                /* Clock Recovery Global Interrupt */
    ECC_IRQHandler,                /* ECC diagnostic Global Interrupt */
    SAI4_IRQHandler,               /* SAI4 global interrupt      */
    0,                             /* Reserved                   */
    0,                             /* Reserved                   */
    WAKEUP_PIN_IRQHandler,         /* Interrupt for all 6 wake-up pins */
};
