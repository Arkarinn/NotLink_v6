#include <stdint.h>

#include "stm32h7xx_hal_cortex.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_cortex.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_utils.h"

#include "bsp.h"
#include "ulog.h"

#include "lwxm.h"
#include "w25qxx.h"

#define APP_SECTION_SIZE (512 * 1024U)   // 大小512K
#define APP_SECTION_ADDR D1_AXISRAM_BASE // 位置AXISRAM

void sys_clock_config(void);

static void sys_mpu_config(void);
static void CopyApp(uint32_t base_addr, uint32_t size);
static void JumpToApp(uint8_t *addr);

struct app_map
{
    uint32_t estack;
    void (*app_entry)(void);
};

int main(void)
{
    LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    sys_clock_config();
    sys_mpu_config();

    bsp_debug_init();

    bsp_tick_init();
    bsp_led_init();
    bsp_jtag_init();
    bsp_vee_init();

    bsp_led_on(LED_RUNNING);
    bsp_led_on(LED_CONNECT);

    bsp_spi_init();
    w25_init();
    w25_read_byte(0); // 重置总线状态

    ulog_init(ULOG_LEVEL_INFO);
    ulog_info("Boot running");

    uint8_t c;

    // 清理缓冲区
    while (bsp_debug_get_byte(&c) == 0)
    {
    }

    ulog_warn("Press [Enter] to enter bootloader");

    uint32_t tick = bsp_tick_get_count();
    uint32_t key_count = 0;
    while (1)
    {
        if ((bsp_debug_get_byte(&c) == 0) && (c == 0x0D))
        {
            key_count++;
            if (key_count > 3)
            {
                // 进入boot
                break;
            }
        }

        bsp_tick_delay(100);
        bsp_led_toggle(LED_RUNNING);

        // 超时处理
        if (bsp_tick_get_count() - tick > 500)
        {
            ulog_warn("Fw update timeout");
            // 正常运行
            goto copy_app;
        }
    }

    // 擦除app区域
    for (uint32_t i = 0; i < APP_SECTION_SIZE; i += (4 * 1024))
    {
        ulog_warn("Erase sector: 0x%0X, size: %d", i, 4 * 1024);
        w25_erase_sector_4k(i);
        bsp_led_toggle(LED_RUNNING);
    }
    ulog_info("Erase done");
    bsp_led_off(LED_RUNNING);

    // 清理缓冲区
    while (bsp_debug_get_byte(&c) == 0)
    {
    }

    ulog_info("Waiting for Xmodem transfer");
    int r = lwxm_loop(0x00000000);

    bsp_tick_delay(1000); // 等待上位机传输结束，防止吞字符
    if (r != 0)
    {
        ulog_error("Update error");
        goto boot_error;
    }
    ulog_info("Update done");

copy_app:

    bsp_led_off(LED_RUNNING);
    bsp_led_on(LED_CONNECT);

    uint32_t estack = w25_read_word(0);
    ulog_info("App estack address: 0x%08X", estack);

    if ((estack & 0x03) != 0) // 字节对齐
    {
        ulog_error("Stack address error, boot failed");
        goto boot_error;
    }

    bsp_led_on(LED_RUNNING);
    ulog_info("Copy app section to: 0x%08X, size: 0x%08X (%d bytes)", //
              APP_SECTION_ADDR, APP_SECTION_SIZE, APP_SECTION_SIZE);
    CopyApp(D1_AXISRAM_BASE, APP_SECTION_SIZE); // 复制APP
    ulog_info("Copy app section completed");
    bsp_led_off(LED_RUNNING);

    ulog_info("Jump to app");
    bsp_debug_deinit();
    w25_deinit();
    bsp_tick_deinit();

    JumpToApp((uint8_t *)D1_AXISRAM_BASE); // 跳转

boot_error:

    while (1)
    {
        bsp_led_toggle(LED_RUNNING); // 闪灯
        bsp_tick_delay(300);
    }
}

/**
 * @brief 复制APP
 *
 */
void CopyApp(uint32_t base_addr, uint32_t size)
{
    w25_read_mass(0, (uint8_t *)base_addr, size);
}

/**
 * @brief 跳转到APP
 *
 * @param addr
 */
void JumpToApp(uint8_t *addr)
{
    struct app_map *app = (struct app_map *)addr;

    __disable_irq();
    for (uint32_t i = 0; i < 8; i++)
    {
        /* 关闭所有中断，清除所有中断挂起标志 */
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    __enable_irq();

    __set_MSP(app->estack); // 设置堆栈指针
    __set_CONTROL(0);       // 特权模式
    app->app_entry();
}

void sys_mpu_config(void)
{
    /* Disables the MPU */
    LL_MPU_Disable();
}

void sys_clock_config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4)
    {
    }
    LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
    while (LL_PWR_IsActiveFlag_VOS() == 0)
    {
    }
    LL_RCC_CSI_Enable();

    /* Wait till LSI is ready */
    while (LL_RCC_CSI_IsReady() != 1)
    {
    }

    LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_CSI); // 内部RC
    LL_RCC_PLL1P_Enable();
    LL_RCC_PLL1Q_Enable();
    LL_RCC_PLL1R_Enable();
    LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
    LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
    LL_RCC_PLL1_SetM(1);
    LL_RCC_PLL1_SetN(240); // 4*240=960MHz
    LL_RCC_PLL1_SetP(2);   // 960/2=480MHz
    LL_RCC_PLL1_SetQ(8);   // 960/8=120MHz to spi
    LL_RCC_PLL1_SetR(8);
    LL_RCC_PLL1_Enable();

    /* Wait till PLL is ready */
    while (LL_RCC_PLL1_IsReady() != 1)
    {
    }

    /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
    LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
    {
    }
    LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
    LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);

    // LL_Init1msTick(480000000);

    LL_SetSystemCoreClock(480000000);
    LL_RCC_SetSPIClockSource(LL_RCC_SPI123_CLKSOURCE_PLL1Q);    // SPI
    LL_RCC_SetUSARTClockSource(LL_RCC_USART16_CLKSOURCE_PCLK2); // USART1
}

/**
 * @brief 格式化字符串打印
 *
 * @param ch
 * @return int
 */
int __io_putchar(int ch)
{
    bsp_debug_send_byte_poll(ch);
    return 0;
}

/*******************************************************************/

int lwxm_get_byte(uint8_t *ch)
{
    return bsp_debug_get_byte(ch);
}

void lwxm_put_byte_poll(uint8_t *ch)
{
    bsp_debug_send_byte_poll(*ch);
}

uint32_t lwxm_get_tick(void)
{
    return bsp_tick_get_count();
}

int lwxm_mem_write(uint32_t addr, uint8_t *data_buff, uint32_t size)
{
#if 0
    for (uint32_t i = 0; i < size; i++)
    {
        w25_write_byte(addr + i, data_buff[i]);
    }
#else
    w25_write_mass(addr, data_buff, size);
#endif

    return 0;
}
