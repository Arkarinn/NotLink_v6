#include "bsp_jtag.h"

#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_tim.h"

#ifndef __RAM_FUNC
#define __RAM_FUNC __attribute__((section(".RamFunc")))
#endif

static uint32_t bsp_tim_div_count = 24;
static uint8_t bsp_jtag_enabled_port = JTAG_PORT_GPIO;
static uint8_t bsp_port_gpio_mode = 1;

static void jtag_set_gpio_af_mode(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Alternate);

/**
 * @brief 设置GPIO的复用功能
 *
 * @param GPIOx
 * @param Pin
 * @param Alternate
 */
static void jtag_set_gpio_af_mode(GPIO_TypeDef *GPIOx, uint32_t Pin, uint32_t Alternate)
{
    if (Pin > LL_GPIO_PIN_7)
    {
        LL_GPIO_SetAFPin_8_15(GPIOx, Pin, Alternate);
        return;
    }

    LL_GPIO_SetAFPin_0_7(GPIOx, Pin, Alternate);
}

/**
 * @brief 初始化
 *
 * @param port  接口序号
 *              JTAG_PORT_SWD
 *              JTAG_PORT_JTAG
 *              JTAG_PORT_GPIO
 */
void bsp_jtag_init(void)
{
    /* 打开GPIO时钟 */
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1); // TMS收发
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3); // TDI/TDO收发
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8); // 主时钟
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4); // 驱动使能
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2); // 时间戳

    /****************************** 特殊引脚，只需要一次配置 *****************************/

    // 5V输出脚
    JTAG_VOUT_ENABLE();
    LL_GPIO_SetPinMode(JTAG_VOUT_EN_GPIO_Port, JTAG_VOUT_EN_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(JTAG_VOUT_EN_GPIO_Port, JTAG_VOUT_EN_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    // RESET开漏输出脚
    LL_GPIO_SetPinMode(JTAG_RESET_OD_GPIO_Port, JTAG_RESET_OD_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(JTAG_RESET_OD_GPIO_Port, JTAG_RESET_OD_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    // 时钟默认电平脚
    LL_GPIO_SetPinMode(JTAG_TCK_PULL_GPIO_Port, JTAG_TCK_PULL_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(JTAG_TCK_PULL_GPIO_Port, JTAG_TCK_PULL_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    // 重置所有引脚为默认状态
    bsp_jtag_deinit();

    /* 32位定时器作为时间戳 */
    LL_TIM_InitTypeDef TIM_InitStruct;
    LL_TIM_StructInit(&TIM_InitStruct);
    TIM_InitStruct.Prescaler = 240U - 1U;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 0xFFFFFFFFUL;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM2, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM2);
    LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM2);
    LL_TIM_GenerateEvent_UPDATE(TIM2);
    LL_TIM_EnableCounter(TIM2);
}

/**
 * @brief 获取时间戳
 *
 * @return uint32_t
 */
__INLINE uint32_t bsp_jtag_get_time_stamp(void)
{
    return LL_TIM_GetCounter(TIM2);
}

/**
 * @brief 初始化接口
 *
 * @param port GPIO/SWD/JTAG
 */
void bsp_jtag_enable_port(jtag_port_t port)
{
    LL_SPI_InitTypeDef SPI_InitStruct;
    LL_TIM_InitTypeDef TIM_InitStruct;
    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct;
    LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct;

    bsp_jtag_deinit(); // 重置所有引脚为默认状态

    LL_RCC_ClocksTypeDef clks;
    LL_RCC_GetSystemClocksFreq(&clks);
    uint32_t tim_freq = clks.HCLK_Frequency;

    /* 分配功能引脚 */
    switch (port)
    {
    case JTAG_PORT_SWD:
        bsp_jtag_enabled_port = JTAG_PORT_SWD;

        /* 主通信 */
        LL_SPI_StructInit(&SPI_InitStruct);
        LL_SPI_DeInit(SPI1); // 去使能，防止寄存器无法写入
        SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
        SPI_InitStruct.Mode = LL_SPI_MODE_SLAVE; // 从机模式
        SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
        SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH; // 高
        SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;      // 边沿
        SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
        SPI_InitStruct.BitOrder = LL_SPI_LSB_FIRST; // SWD要求LSB
        SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
        SPI_InitStruct.CRCPoly = 0x0;
        LL_SPI_Init(SPI1, &SPI_InitStruct);
        LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
        LL_SPI_SetFIFOThreshold(SPI1, LL_SPI_FIFO_TH_01DATA);
        LL_SPI_DisableNSSPulseMgt(SPI1);
        LL_SPI_DisableDMAReq_TX(SPI1);
        LL_SPI_DisableDMAReq_RX(SPI1);
        LL_SPI_DisableGPIOControl(SPI1); // SPI从机不使能时，所有引脚都是高阻态
        LL_SPI_SetInterDataIdleness(SPI1, LL_SPI_ID_IDLENESS_00CYCLE);
        LL_SPI_SetNSSPolarity(SPI1, LL_SPI_NSS_POLARITY_HIGH);
        LL_SPI_SetUDRConfiguration(SPI1, LL_SPI_UDR_CONFIG_REGISTER_PATTERN); // 下溢行为
        LL_SPI_SetUDRPattern(SPI1, 0x00000000);                               // 固定值0
        LL_SPI_SetUDRDetection(SPI1, LL_SPI_UDR_DETECT_BEGIN_DATA_FRAME);     // 下溢检测

        LL_TIM_StructInit(&TIM_InitStruct);
        LL_TIM_DeInit(TIM8);
        TIM_InitStruct.Prescaler = 0;
        TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
        TIM_InitStruct.Autoreload = ((tim_freq / 10000000) - 1); // 默认10M
        TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
        TIM_InitStruct.RepetitionCounter = 0;
        LL_TIM_Init(TIM8, &TIM_InitStruct);
        LL_TIM_DisableMasterSlaveMode(TIM8);
        LL_TIM_EnableARRPreload(TIM8); // 自动重装

        LL_TIM_SetClockSource(TIM8, LL_TIM_CLOCKSOURCE_INTERNAL); // 内部时钟
        LL_TIM_SetTriggerOutput(TIM8, LL_TIM_TRGO_ENABLE);        // 输出使能脉冲信号
        LL_TIM_SetTriggerOutput2(TIM8, LL_TIM_TRGO2_ENABLE);      // 未使用
        LL_TIM_SetOnePulseMode(TIM8, LL_TIM_ONEPULSEMODE_SINGLE); // 单周期模式
        LL_TIM_SetRepetitionCounter(TIM8, 8 - 1);                 // 重复计数

        LL_TIM_OC_StructInit(&TIM_OC_InitStruct);
        TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
        TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能输出
        TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
        TIM_OC_InitStruct.CompareValue = ((tim_freq / 10000000) / 2);
        TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 极性高
        TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
        TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
        TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
        LL_TIM_OC_Init(TIM8, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM8, LL_TIM_CHANNEL_CH3);   // 快速模式
        LL_TIM_OC_EnablePreload(TIM8, LL_TIM_CHANNEL_CH3); // 自动重装

        LL_TIM_BDTR_StructInit(&TIM_BDTRInitStruct);
        TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
        TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
        TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
        TIM_BDTRInitStruct.DeadTime = 0;
        TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE; // 关
        TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
        TIM_BDTRInitStruct.BreakFilter = LL_TIM_BREAK_FILTER_FDIV1;
        TIM_BDTRInitStruct.Break2State = LL_TIM_BREAK2_DISABLE;
        TIM_BDTRInitStruct.Break2Polarity = LL_TIM_BREAK2_POLARITY_HIGH;
        TIM_BDTRInitStruct.Break2Filter = LL_TIM_BREAK2_FILTER_FDIV1;
        TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
        LL_TIM_BDTR_Init(TIM8, &TIM_BDTRInitStruct);

        LL_TIM_EnableAllOutputs(TIM8);     // 打开所有输出
        LL_TIM_GenerateEvent_UPDATE(TIM8); // 产生更新事件，更新寄存器缓冲值

        LL_TIM_StructInit(&TIM_InitStruct);
        TIM_InitStruct.Prescaler = 0;
        TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
        TIM_InitStruct.Autoreload = 8 * (tim_freq / 10000000) + (tim_freq / 10000000) / 4 - 1;
        TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
        LL_TIM_Init(TIM4, &TIM_InitStruct);
        LL_TIM_EnableARRPreload(TIM4);

        LL_TIM_EnableMasterSlaveMode(TIM4);                                // 打开主从模式
        LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);          // 内部时钟计数
        LL_TIM_SetTriggerInput(TIM4, LL_TIM_TS_ITR3);                      // TIM8
        LL_TIM_SetSlaveMode(TIM4, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER); // 可重触发使能模式
        LL_TIM_DisableIT_TRIG(TIM4);
        LL_TIM_DisableDMAReq_TRIG(TIM4);
        LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_UPDATE);

#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE

        LL_TIM_OC_StructInit(&TIM_OC_InitStruct);
        TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;     //
        TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能输出
        TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
        TIM_OC_InitStruct.CompareValue = 8 * (tim_freq / 10000000) + (tim_freq / 10000000) / 4;
        TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 极性正
        LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH1);
        LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH1);
        LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH2);
        LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH2);

#else /* CONFIG_OPTIMIZE_TMS_HARD_OE */

        LL_TIM_OC_StructInit(&TIM_OC_InitStruct);
        TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;     //
        TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能输出
        TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
        TIM_OC_InitStruct.CompareValue = 8 * (tim_freq / 10000000) + (tim_freq / 10000000) / 4;
        TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 极性正
        LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH1);
        LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH1);

#endif /* CONFIG_OPTIMIZE_TMS_HARD_OE */

        LL_TIM_SetOnePulseMode(TIM4, LL_TIM_ONEPULSEMODE_SINGLE); // 单脉冲模式
        LL_TIM_EnableAllOutputs(TIM4);                            // 使能所有输出
        LL_TIM_GenerateEvent_UPDATE(TIM4);                        // 更新参数
        LL_TIM_EnableCounter(TIM4);                               // 启动

        // TMS->SPI数据收发
        JTAG_TMS_OEN_HIGH(); // SWD数据输出
        JTAG_TMS_OUT(1U);
        jtag_set_gpio_af_mode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_AF_5);          //
        LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_ALTERNATE);   // SPI1_MISO
        jtag_set_gpio_af_mode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_AF_5);          //
        LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_ALTERNATE);   // SPI1_MOSI
        LL_GPIO_SetPinSpeed(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_SPEED_FREQ_HIGH); //

        // TCK->SPI时钟输出
        JTAG_TCK_EN_HIGH();
        JTAG_TCK_OUT(1U);
        JTAG_TCK_PULL_HIGH();                                                                   // 时钟不输出时默认为高
        JTAG_TCK_OEN_HIGH();                                                                    // SWD时钟输出
        LL_GPIO_SetPinSpeed(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_SPEED_FREQ_HIGH); //
        jtag_set_gpio_af_mode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_AF_3);          // SWD TIM8_CH3
        LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_ALTERNATE);   //
        jtag_set_gpio_af_mode(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_AF_2);            // SWD TIM4_CH1
        LL_GPIO_SetPinMode(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_MODE_ALTERNATE);     //
        jtag_set_gpio_af_mode(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_AF_5);              // SWD SPI1_SCK
        LL_GPIO_SetPinMode(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_MODE_ALTERNATE);       //

#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE
        // TMS_DIR->数据方向，硬件控制
        jtag_set_gpio_af_mode(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin, LL_GPIO_AF_2);        // SWD TIM4_CH2
        LL_GPIO_SetPinMode(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin, LL_GPIO_MODE_ALTERNATE); //
#endif

        // Enable SPI and transfer
        bsp_jtag_enable_spi_tms();
        bsp_jtag_enable_transfer_tms();

        break;
    case JTAG_PORT_JTAG:
        bsp_jtag_enabled_port = JTAG_PORT_JTAG;

        LL_SPI_StructInit(&SPI_InitStruct);
        LL_SPI_DeInit(SPI1); // 去使能，防止寄存器无法写入
        SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
        SPI_InitStruct.Mode = LL_SPI_MODE_SLAVE; // 从机模式
        SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
        SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH; // 高
        SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;      // 边沿
        SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
        SPI_InitStruct.BitOrder = LL_SPI_LSB_FIRST; // SWD要求LSB
        SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
        SPI_InitStruct.CRCPoly = 0x0;
        LL_SPI_Init(SPI1, &SPI_InitStruct);
        LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
        LL_SPI_SetFIFOThreshold(SPI1, LL_SPI_FIFO_TH_01DATA);
        LL_SPI_DisableNSSPulseMgt(SPI1);
        LL_SPI_DisableDMAReq_TX(SPI1);
        LL_SPI_DisableDMAReq_RX(SPI1);
        LL_SPI_DisableGPIOControl(SPI1); // SPI从机不使能时，所有引脚都是高阻态
        LL_SPI_SetInterDataIdleness(SPI1, LL_SPI_ID_IDLENESS_00CYCLE);
        LL_SPI_SetNSSPolarity(SPI1, LL_SPI_NSS_POLARITY_HIGH);

        LL_SPI_StructInit(&SPI_InitStruct);
        LL_SPI_DeInit(SPI3); // 去使能，防止寄存器无法写入
        SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
        SPI_InitStruct.Mode = LL_SPI_MODE_SLAVE; // 从机模式
        SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
        SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH; // 高
        SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;      // 边沿
        SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
        SPI_InitStruct.BitOrder = LL_SPI_LSB_FIRST; // SWD要求LSB
        SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
        SPI_InitStruct.CRCPoly = 0x0;
        LL_SPI_Init(SPI3, &SPI_InitStruct);
        LL_SPI_SetStandard(SPI3, LL_SPI_PROTOCOL_MOTOROLA);
        LL_SPI_SetFIFOThreshold(SPI3, LL_SPI_FIFO_TH_01DATA);
        LL_SPI_DisableNSSPulseMgt(SPI3);
        LL_SPI_DisableDMAReq_TX(SPI3);
        LL_SPI_DisableDMAReq_RX(SPI3);
        LL_SPI_DisableGPIOControl(SPI3); // SPI从机不使能时，所有引脚都是高阻态
        LL_SPI_SetInterDataIdleness(SPI3, LL_SPI_ID_IDLENESS_00CYCLE);
        LL_SPI_SetNSSPolarity(SPI3, LL_SPI_NSS_POLARITY_HIGH);

        LL_TIM_StructInit(&TIM_InitStruct);
        LL_TIM_DeInit(TIM8);
        TIM_InitStruct.Prescaler = 0;
        TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
        TIM_InitStruct.Autoreload = ((tim_freq / 10000000) - 1); // 默认10M
        TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
        TIM_InitStruct.RepetitionCounter = 0;
        LL_TIM_Init(TIM8, &TIM_InitStruct);
        LL_TIM_DisableMasterSlaveMode(TIM8);
        LL_TIM_EnableARRPreload(TIM8);                            // 自动重装
        LL_TIM_SetClockSource(TIM8, LL_TIM_CLOCKSOURCE_INTERNAL); // 内部时钟
        LL_TIM_SetTriggerOutput(TIM8, LL_TIM_TRGO_ENABLE);        // 输出使能脉冲信号
        LL_TIM_SetTriggerOutput2(TIM8, LL_TIM_TRGO2_ENABLE);      // 未使用
        LL_TIM_SetOnePulseMode(TIM8, LL_TIM_ONEPULSEMODE_SINGLE); // 单周期模式
        LL_TIM_SetRepetitionCounter(TIM8, 8 - 1);                 // 重复计数

        LL_TIM_OC_StructInit(&TIM_OC_InitStruct);
        TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
        TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能输出
        TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
        TIM_OC_InitStruct.CompareValue = ((tim_freq / 10000000) / 2);
        TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 极性高
        TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
        TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
        TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
        LL_TIM_OC_Init(TIM8, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM8, LL_TIM_CHANNEL_CH3);   // 快速模式
        LL_TIM_OC_EnablePreload(TIM8, LL_TIM_CHANNEL_CH3); // 自动重装

        LL_TIM_BDTR_StructInit(&TIM_BDTRInitStruct);
        TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
        TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
        TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
        TIM_BDTRInitStruct.DeadTime = 0;
        TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE; // 关
        TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
        TIM_BDTRInitStruct.BreakFilter = LL_TIM_BREAK_FILTER_FDIV1;
        TIM_BDTRInitStruct.Break2State = LL_TIM_BREAK2_DISABLE;
        TIM_BDTRInitStruct.Break2Polarity = LL_TIM_BREAK2_POLARITY_HIGH;
        TIM_BDTRInitStruct.Break2Filter = LL_TIM_BREAK2_FILTER_FDIV1;
        TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
        LL_TIM_BDTR_Init(TIM8, &TIM_BDTRInitStruct);

        LL_TIM_EnableAllOutputs(TIM8);     // 打开所有输出
        LL_TIM_GenerateEvent_UPDATE(TIM8); // 产生更新事件，更新寄存器缓冲值

        LL_TIM_StructInit(&TIM_InitStruct);
        TIM_InitStruct.Prescaler = 0;
        TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
        TIM_InitStruct.Autoreload = 8 * (tim_freq / 10000000) + (tim_freq / 10000000) / 4 - 1;
        TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
        LL_TIM_Init(TIM4, &TIM_InitStruct);
        LL_TIM_EnableARRPreload(TIM4);
        LL_TIM_EnableMasterSlaveMode(TIM4);                                // 打开主从模式
        LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);          // 内部时钟计数
        LL_TIM_SetTriggerInput(TIM4, LL_TIM_TS_ITR3);                      // TIM8
        LL_TIM_SetSlaveMode(TIM4, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER); // 可重触发使能模式
        LL_TIM_DisableIT_TRIG(TIM4);
        LL_TIM_DisableDMAReq_TRIG(TIM4);
        LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_UPDATE);

        LL_TIM_OC_StructInit(&TIM_OC_InitStruct);
        TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;     // PWM1模式
        TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能输出
        TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
        TIM_OC_InitStruct.CompareValue = 8 * (tim_freq / 10000000) + (tim_freq / 10000000) / 4;
        TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 极性正
        LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
        LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH1);
        LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH1);

        LL_TIM_SetOnePulseMode(TIM4, LL_TIM_ONEPULSEMODE_SINGLE); // 单脉冲模式
        LL_TIM_EnableAllOutputs(TIM4);                            // 使能所有输出
        LL_TIM_GenerateEvent_UPDATE(TIM4);                        // 更新参数
        LL_TIM_EnableCounter(TIM4);                               // 启动

        // TRST->GPIO输出
        JTAG_TRST_OEN_HIGH();
        JTAG_TRST_OUT(1U);
        LL_GPIO_SetPinMode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_MODE_OUTPUT);

        // TDI->SPI数据输出
        JTAG_TDI_OEN_HIGH();                                                              // SPI输出
        LL_GPIO_SetPinMode(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_MODE_INPUT); // 串口关闭
        jtag_set_gpio_af_mode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_AF_6);            // SPI3_MISO
        LL_GPIO_SetPinMode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetPinSpeed(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_SPEED_FREQ_HIGH);

        // TMS->SPI数据收发
        JTAG_TMS_OEN_HIGH(); // SWD数据输出
        JTAG_TMS_OUT(1U);
        jtag_set_gpio_af_mode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_AF_5);          //
        LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_ALTERNATE);   // SPI1_MISO
        jtag_set_gpio_af_mode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_AF_5);          //
        LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_ALTERNATE);   // SPI1_MOSI
        LL_GPIO_SetPinSpeed(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_SPEED_FREQ_HIGH); //
        LL_GPIO_SetPinPull(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_PULL_DOWN);        // 高阻时输出低

        // TCK->SPI时钟输出
        JTAG_TCK_EN_HIGH();
        JTAG_TCK_OUT(1U);
        JTAG_TCK_PULL_HIGH();                                                                 // 时钟不输出时默认为高
        JTAG_TCK_OEN_HIGH();                                                                  // SWD时钟输出
        jtag_set_gpio_af_mode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_AF_3);        // SWD TIM8_CH3
        LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_ALTERNATE); //
        jtag_set_gpio_af_mode(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_AF_2);          // SWD TIM4_CH1
        LL_GPIO_SetPinMode(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_MODE_ALTERNATE);   //
        jtag_set_gpio_af_mode(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_AF_5);            // SWD SPI1_SCK
        LL_GPIO_SetPinMode(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_MODE_ALTERNATE);     //
        jtag_set_gpio_af_mode(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_AF_6);            // SWD SPI3_SCK
        LL_GPIO_SetPinMode(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_MODE_ALTERNATE);     //

        // TDO->SPI数据输入
        JTAG_TDO_OEN_LOW();                                                    // SPI输入
        jtag_set_gpio_af_mode(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_AF_6); // SPI3_MOSI
        LL_GPIO_SetPinMode(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_MODE_ALTERNATE);

        // Enable SPI and transfer
        bsp_jtag_enable_spi_tms();
        bsp_jtag_enable_spi_tdi_tdo();
        bsp_jtag_enable_transfer_tms();
        bsp_jtag_enable_transfer_tdi_tdo();

        break;
    case JTAG_PORT_GPIO:
        bsp_jtag_enabled_port = JTAG_PORT_GPIO;

    default:
        break;
    }
}

/**
 * @brief 取消初始化，所有引脚进入默认状态
 *
 */
void bsp_jtag_deinit(void)
{
    JTAG_RESET_OD_HIGH(); // 复位信号输出高
    JTAG_TCK_PULL_HIGH(); // 时钟默认拉高
    JTAG_TCK_EN_LOW();    // 时钟输出关

    /* 信号脚全部设为输入状态 */
    LL_GPIO_SetPinMode(JTAG_TRST_GPIO_Port, JTAG_TRST_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_MODE_ALTERNATE); // 默认串口
    LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_RTCK_GPIO_Port, JTAG_RTCK_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_RESET_GPIO_Port, JTAG_RESET_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(JTAG_DBGRQ_RXD_GPIO_Port, JTAG_DBGRQ_RXD_Pin, LL_GPIO_MODE_ALTERNATE); // 默认串口

    /* 所有OEN引脚输出 */
    LL_GPIO_SetPinMode(JTAG_TRST_DIR_GPIO_Port, JTAG_TRST_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_TDI_DIR_GPIO_Port, JTAG_TDI_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_TCK_DIR_GPIO_Port, JTAG_TCK_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_RTCK_DIR_GPIO_Port, JTAG_RTCK_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_TDO_DIR_GPIO_Port, JTAG_TDO_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_RESET_DIR_GPIO_Port, JTAG_RESET_DIR_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(JTAG_DBGRQ_DIR_GPIO_Port, JTAG_DBGRQ_DIR_Pin, LL_GPIO_MODE_OUTPUT);

    /* 所有OEN引脚推挽输出 */
    LL_GPIO_SetPinOutputType(JTAG_TRST_DIR_GPIO_Port, JTAG_TRST_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TDI_DIR_GPIO_Port, JTAG_TDI_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TCK_DIR_GPIO_Port, JTAG_TCK_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_RTCK_DIR_GPIO_Port, JTAG_RTCK_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TDO_DIR_GPIO_Port, JTAG_TDO_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_RESET_DIR_GPIO_Port, JTAG_RESET_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_DBGRQ_DIR_GPIO_Port, JTAG_DBGRQ_DIR_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    /* 所有DIR引脚快速输出 */
    LL_GPIO_SetPinSpeed(JTAG_TRST_DIR_GPIO_Port, JTAG_TRST_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TDI_DIR_GPIO_Port, JTAG_TDI_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TCK_DIR_GPIO_Port, JTAG_TCK_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_RTCK_DIR_GPIO_Port, JTAG_RTCK_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TDO_DIR_GPIO_Port, JTAG_TDO_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_RESET_DIR_GPIO_Port, JTAG_RESET_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_DBGRQ_DIR_GPIO_Port, JTAG_DBGRQ_DIR_Pin, LL_GPIO_SPEED_FREQ_HIGH);

    /* 拉低所有OE信号，所有外部引脚高阻态，除了串口 */
    JTAG_TRST_OEN_LOW();
    JTAG_TDI_OEN_HIGH(); // 串口输出
    JTAG_TMS_OEN_LOW();
    JTAG_TCK_OEN_LOW();
    JTAG_RTCK_OEN_LOW();
    JTAG_TDO_OEN_LOW();
    JTAG_RESET_OEN_LOW();
    JTAG_DBGRQ_OEN_LOW(); // 串口输入

    /* 输出速度 */
    LL_GPIO_SetPinSpeed(JTAG_TRST_GPIO_Port, JTAG_TRST_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_RTCK_GPIO_Port, JTAG_RTCK_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_RESET_GPIO_Port, JTAG_RESET_Pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinSpeed(JTAG_DBGRQ_RXD_GPIO_Port, JTAG_DBGRQ_RXD_Pin, LL_GPIO_SPEED_FREQ_HIGH);

    /* 信号脚无上下拉 */
    LL_GPIO_SetPinPull(JTAG_TRST_GPIO_Port, JTAG_TRST_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_PULL_UP); // 上拉
    LL_GPIO_SetPinPull(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_RTCK_GPIO_Port, JTAG_RTCK_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_RESET_GPIO_Port, JTAG_RESET_Pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinPull(JTAG_DBGRQ_RXD_GPIO_Port, JTAG_DBGRQ_RXD_Pin, LL_GPIO_PULL_NO);

    /* 推挽输出 */
    LL_GPIO_SetPinOutputType(JTAG_TRST_GPIO_Port, JTAG_TRST_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TCK_M_GPIO_Port, JTAG_TCK_M_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TCK_S_GPIO_Port, JTAG_TCK_S_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_RTCK_GPIO_Port, JTAG_RTCK_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_RESET_GPIO_Port, JTAG_RESET_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinOutputType(JTAG_DBGRQ_RXD_GPIO_Port, JTAG_DBGRQ_RXD_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    /* 复用回默认状态 */
    jtag_set_gpio_af_mode(JTAG_TDI_TXD_GPIO_Port, JTAG_TDI_TXD_Pin, LL_GPIO_AF_8);     // COM UART4_TXD
    jtag_set_gpio_af_mode(JTAG_DBGRQ_RXD_GPIO_Port, JTAG_DBGRQ_RXD_Pin, LL_GPIO_AF_8); // COM UART4_RXD

    LL_SPI_SetInternalSSLevel(SPI1, LL_SPI_SS_LEVEL_LOW);
    LL_SPI_Disable(SPI1);

    LL_SPI_SetInternalSSLevel(SPI3, LL_SPI_SS_LEVEL_LOW);
    LL_SPI_Disable(SPI3);

    LL_TIM_DisableCounter(TIM8);
    LL_TIM_DisableCounter(TIM4);
}

/**
 * @brief 关闭输出
 *
 */
__INLINE void bsp_jtag_enable_vout(void)
{
    JTAG_VOUT_ENABLE();
}

/**
 * @brief 打开输出
 *
 */
__INLINE void bsp_jtag_disable_vout(void)
{
    JTAG_VOUT_DISABLE();
}

/**
 * @brief 设定TCK频率
 *
 * @param freq          期望频率
 * @return uint32_t     实际频率
 */
uint32_t bsp_jtag_set_tck_clock(uint32_t freq)
{
    // 消除尾数
    // freq = (freq / 1000) * 1000;
    freq = freq - (freq % 1000);

    // 管控有效值
    if (freq > JTAG_SWJ_FREQ_MAX)
    {
        freq = JTAG_SWJ_FREQ_MAX;
    }
    if (freq < JTAG_SWJ_FREQ_MIN)
    {
        freq = JTAG_SWJ_FREQ_MIN;
    }

    LL_RCC_ClocksTypeDef clks;
    LL_RCC_GetSystemClocksFreq(&clks);
    uint32_t tim_freq = clks.HCLK_Frequency;

    uint32_t div = (tim_freq + (freq - 1)) / freq; // 每周期的计数值

    // 通过预分频器将计数器值限制到合理范围
    uint32_t pre = 1;
    while ((div * (8U * JTAG_SPI_FIFO_SIZE + 1U) / pre) > 65535U)
    {
        pre++;
    }

    uint32_t count = div / pre;
    bsp_tim_div_count = count;
    uint32_t real_freq = (tim_freq / pre) / count; // 实际频率

    LL_TIM_SetPrescaler(TIM8, pre - 1U);
    LL_TIM_SetPrescaler(TIM4, pre - 1U);

    LL_TIM_SetAutoReload(TIM8, count - 1U);
    LL_TIM_OC_SetCompareCH3(TIM8, count * JTAG_CLK_DUTY_CYCLE);
    LL_TIM_SetAutoReload(TIM4, count * 8U + count / 8U - 1U);
    LL_TIM_OC_SetCompareCH1(TIM4, count * 8U + count / 8U);

    LL_TIM_GenerateEvent_UPDATE(TIM8); // 更新影子寄存器
    LL_TIM_GenerateEvent_UPDATE(TIM4); // 更新影子寄存器

    return real_freq;
}

#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE
/**
 * @brief 生成时钟，自动控制OE
 *
 * @param count Must be less than 8U * JTAG_SPI_FIFO_SIZE
 */
__INLINE void bsp_jtag_generate_data_cycle_with_oe(uint32_t count_s1, uint32_t count_s2)
{
    uint32_t n_bytes = (count_s1 + count_s2 + 7) / 8;

    LL_TIM_SetRepetitionCounter(TIM8, (8U * n_bytes) - 1U);
#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM8, EGR, TIM_EGR_UG);
#else
    LL_TIM_GenerateEvent_UPDATE(TIM8);
#endif

    LL_TIM_SetAutoReload(TIM4, bsp_tim_div_count * (8U * n_bytes) + (bsp_tim_div_count / 8U) - 1U);
    LL_TIM_OC_SetCompareCH1(TIM4, bsp_tim_div_count * (count_s1 + count_s2) + (bsp_tim_div_count / 8U));
    LL_TIM_OC_SetCompareCH2(TIM4, bsp_tim_div_count * count_s1 + (bsp_tim_div_count / 8U));

#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM4, EGR, TIM_EGR_UG);
#else
    LL_TIM_GenerateEvent_UPDATE(TIM4);
#endif

#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM8, CR1, TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_CEN);
#else
    LL_TIM_EnableCounter(TIM8);
#endif
}
#endif /* CONFIG_OPTIMIZE_TMS_HARD_OE */

/**
 * @brief 生成时钟
 *
 * @param count Must be less than 8U * JTAG_SPI_FIFO_SIZE
 */
__INLINE void bsp_jtag_generate_data_cycle(uint32_t count)
{
    uint32_t n_bytes = (count + 7) / 8;

    LL_TIM_SetRepetitionCounter(TIM8, (8U * n_bytes) - 1U);
#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM8, EGR, TIM_EGR_UG);
#else
    LL_TIM_GenerateEvent_UPDATE(TIM8);
#endif

    LL_TIM_SetAutoReload(TIM4, bsp_tim_div_count * (8U * n_bytes) + (bsp_tim_div_count / 8U) - 1U);
    LL_TIM_OC_SetCompareCH1(TIM4, bsp_tim_div_count * count + (bsp_tim_div_count / 8U));
#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM4, EGR, TIM_EGR_UG);
#else
    LL_TIM_GenerateEvent_UPDATE(TIM4);
#endif

#if (JTAG_REG_OPTIMIZE != 0)
    LL_TIM_WriteReg(TIM8, CR1, TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_CEN);
#else
    LL_TIM_EnableCounter(TIM8);
#endif
}

/**
 * @brief 等待定时器运行结束
 *
 */
__INLINE void bsp_jtag_wait_data_cycle(void)
{
    while (LL_TIM_IsEnabledCounter(TIM8))
    {
    }
}

/**
 * @brief 填充FIFO
 *
 * @param data
 * @param n_bytes
 */
__INLINE void bsp_jtag_write_tms_tx_fifo(uint8_t *data_buff, uint32_t n_bytes)
{
    while (n_bytes)
    {
        LL_SPI_TransmitData8(SPI1, *data_buff);
        data_buff++;
        n_bytes--;
    }
}

/**
 * @brief 填充FIFO
 *
 * @param data_buff
 */
__INLINE void bsp_jtag_write_tms_tx_fifo_byte(uint8_t *data_buff)
{
    LL_SPI_TransmitData8(SPI1, *data_buff);
}

/**
 * @brief 读FIFO
 *
 * @param data_buff
 * @param n_bytes
 */
__INLINE void bsp_jtag_read_tms_rx_fifo(uint8_t *data_buff, uint32_t n_bytes)
{
    while (n_bytes)
    {
        if (LL_SPI_IsActiveFlag_RXP(SPI1))
        {
            *data_buff = LL_SPI_ReceiveData8(SPI1);
            data_buff++;
            n_bytes--;
        }
    }
}

/**
 * @brief
 *
 * @param data_buff
 */
__INLINE void bsp_jtag_read_tms_rx_fifo_byte(uint8_t *data_buff)
{
    while (LL_SPI_IsActiveFlag_RXP(SPI1) != 1U)
    {
    }
    *data_buff = LL_SPI_ReceiveData8(SPI1);
}

/**
 * @brief 清理接收FIFO
 *
 * @param data_buff 缓冲区
 * @param max_size  可用大小
 */
__INLINE void bsp_jtag_clean_tms_rx_fifo(uint8_t *data_buff, uint32_t max_size)
{
    uint32_t i = 0;
    while (LL_SPI_IsActiveFlag_RXP(SPI1))
    {
        data_buff[i] = LL_SPI_ReceiveData8(SPI1);
        i = (i + 1) % max_size;
    }
}

/**
 * @brief 填充FIFO
 *
 * @param data
 * @param n_bytes
 */
__INLINE void bsp_jtag_write_tdi_tx_fifo(uint8_t *data_buff, uint32_t n_bytes)
{
    while (n_bytes)
    {
        LL_SPI_TransmitData8(SPI3, *data_buff);
        data_buff++;
        n_bytes--;
    }
}

/**
 * @brief 读FIFO
 *
 * @param data_buff
 * @param n_bytes
 */
__INLINE void bsp_jtag_read_tdo_rx_fifo(uint8_t *data_buff, uint32_t n_bytes)
{
    uint8_t *end = data_buff + n_bytes;
    while (data_buff < end)
    {
        if (LL_SPI_IsActiveFlag_RXP(SPI3))
        {
            *data_buff = LL_SPI_ReceiveData8(SPI3);
            data_buff++;
        }
    }
}

/**
 * @brief 填充FIFO
 *
 * @param data_buff
 */
__INLINE void bsp_jtag_write_tdi_tx_fifo_byte(uint8_t *data_buff)
{
    LL_SPI_TransmitData8(SPI3, *data_buff);
}

/**
 * @brief
 *
 * @param data_buff
 */
__INLINE void bsp_jtag_read_tdo_rx_fifo_byte(uint8_t *data_buff)
{
    while (LL_SPI_IsActiveFlag_RXP(SPI3) != 1U)
    {
    }
    *data_buff = LL_SPI_ReceiveData8(SPI3);
}

/**
 * @brief 打开SPI外设
 *
 */
__INLINE void bsp_jtag_enable_spi_tms(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI1, CR1, SPI_CR1_SPE);
#else
    LL_SPI_Enable(SPI1);
#endif
}

/**
 * @brief 关闭SPI外设
 *
 */
__INLINE void bsp_jtag_disable_spi_tms(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI1, CR1, 0);
#else
    LL_SPI_Disable(SPI1);
#endif
}

/**
 * @brief 打开SPI外设
 *
 */
__INLINE void bsp_jtag_enable_spi_tdi_tdo(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI3, CR1, SPI_CR1_SPE);
#else
    LL_SPI_Enable(SPI3);
#endif
}

/**
 * @brief 关闭SPI外设
 *
 */
__INLINE void bsp_jtag_disable_spi_tdi_tdo(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI3, CR1, 0);
#else
    LL_SPI_Disable(SPI3);
#endif
}

/**
 * @brief 从机片选
 *
 */
__INLINE void bsp_jtag_enable_transfer_tms(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI1, CR1, SPI_CR1_SPE | SPI_CR1_SSI);
#else
    LL_SPI_SetInternalSSLevel(SPI1, LL_SPI_SS_LEVEL_HIGH);
#endif
}

/**
 * @brief 从机片选
 *
 */
__INLINE void bsp_jtag_disable_transfer_tms(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI1, CR1, SPI_CR1_SPE);
#else
    LL_SPI_SetInternalSSLevel(SPI1, LL_SPI_SS_LEVEL_LOW);
#endif
}

/**
 * @brief 从机片选
 *
 */
__INLINE void bsp_jtag_enable_transfer_tdi_tdo(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI3, CR1, SPI_CR1_SPE | SPI_CR1_SSI);
#else
    LL_SPI_SetInternalSSLevel(SPI3, LL_SPI_SS_LEVEL_HIGH);
#endif
}

/**
 * @brief 从机片选
 *
 */
__INLINE void bsp_jtag_disable_transfer_tdi_tdo(void)
{
#if (JTAG_REG_OPTIMIZE != 0)
    LL_SPI_WriteReg(SPI3, CR1, SPI_CR1_SPE);
#else
    LL_SPI_SetInternalSSLevel(SPI3, LL_SPI_SS_LEVEL_LOW);
#endif
}

/**
 * @brief 检查GPIO模式
 *
 * @param mode  1: gpio模式
 *              0: 复用模式
 */
__RAM_FUNC void bsp_jtag_check_gpio_mode(uint32_t mode)
{
    // 不需要修改
    if (bsp_port_gpio_mode == mode)
    {
        return;
    }

    bsp_port_gpio_mode = mode;
    if (mode == 1)
    {
        // 切换为IO功能
        switch (bsp_jtag_enabled_port)
        {
        case JTAG_PORT_SWD:
            JTAG_TCK_OEN_HIGH();
            JTAG_TCK_OUT(1);
            LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_OUTPUT); // 切回IO功能

            JTAG_TMS_OEN_HIGH();
            JTAG_TMS_OUT(1);
            LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_OUTPUT); // 切回IO功能
            LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_INPUT);  // 切回IO功能

            break;
        case JTAG_PORT_JTAG:
            JTAG_TCK_OEN_HIGH();
            JTAG_TCK_OUT(1);
            LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_OUTPUT); // 切回IO功能

            JTAG_TMS_OEN_HIGH();
            JTAG_TMS_OUT(1);
            LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_OUTPUT); // 切回IO功能
            LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_INPUT);  // 切回IO功能
            LL_GPIO_SetPinMode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_MODE_OUTPUT);       // 切回IO功能
            LL_GPIO_SetPinMode(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_MODE_INPUT);        // 切回IO功能

            break;
        default:
            break;
        }
    }
    else
    {
        // 切回复用功能
        switch (bsp_jtag_enabled_port)
        {
        case JTAG_PORT_SWD:
            LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_ALTERNATE); // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_ALTERNATE);   // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_ALTERNATE);   // 切回AF功能
            break;

        case JTAG_PORT_JTAG:
            LL_GPIO_SetPinMode(JTAG_TCK_GEN_GPIO_Port, JTAG_TCK_GEN_Pin, LL_GPIO_MODE_ALTERNATE); // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_ALTERNATE);   // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TMS_DI_GPIO_Port, JTAG_TMS_DI_Pin, LL_GPIO_MODE_ALTERNATE);   // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TDI_GPIO_Port, JTAG_TDI_Pin, LL_GPIO_MODE_ALTERNATE);         // 切回AF功能
            LL_GPIO_SetPinMode(JTAG_TDO_GPIO_Port, JTAG_TDO_Pin, LL_GPIO_MODE_ALTERNATE);         // 切回AF功能
            break;

        default:
            break;
        }
    }
}
