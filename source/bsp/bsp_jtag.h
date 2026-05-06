#ifndef __JTAG_H__
#define __JTAG_H__

#include <stdint.h>

#include "stm32h7xx_ll_gpio.h"

/* JTAG_TRST        PE0 */
#define JTAG_TRST_GPIO_Port GPIOE
#define JTAG_TRST_Pin LL_GPIO_PIN_0

/* JTAG_TDI         PC11    SPI3_MISO */
#define JTAG_TDI_GPIO_Port GPIOC
#define JTAG_TDI_Pin LL_GPIO_PIN_11

/* JTAG_TDI_TXD     PD1     UART4_TX */
#define JTAG_TDI_TXD_GPIO_Port GPIOD
#define JTAG_TDI_TXD_Pin LL_GPIO_PIN_1

/* JTAG_TMS_DO      PB4     SPI1_MISO */
#define JTAG_TMS_DO_GPIO_Port GPIOB
#define JTAG_TMS_DO_Pin LL_GPIO_PIN_4

/* JTAG_TMS_DI      PD7     SPI1_MOSI */
#define JTAG_TMS_DI_GPIO_Port GPIOD
#define JTAG_TMS_DI_Pin LL_GPIO_PIN_7

/* JTAG_TCK_M       PB3     SPI1_SCK */
#define JTAG_TCK_M_GPIO_Port GPIOB
#define JTAG_TCK_M_Pin LL_GPIO_PIN_3

/* JTAG_TCK_S       PC10    SPI3_SCK */
#define JTAG_TCK_S_GPIO_Port GPIOC
#define JTAG_TCK_S_Pin LL_GPIO_PIN_10

/* JTAG_TCK_EN      PB6     TIM4_CH1 */
#define JTAG_TCK_EN_GPIO_Port GPIOB
#define JTAG_TCK_EN_Pin LL_GPIO_PIN_6

/* JTAG_TCK_GEN     PC8     TIM8_CH3 */
#define JTAG_TCK_GEN_GPIO_Port GPIOC
#define JTAG_TCK_GEN_Pin LL_GPIO_PIN_8

/* JTAG_TCK_PULL    PD2 */
#define JTAG_TCK_PULL_GPIO_Port GPIOD
#define JTAG_TCK_PULL_Pin LL_GPIO_PIN_2

/* JTAG_RTCK        PD5 */
#define JTAG_RTCK_GPIO_Port GPIOD
#define JTAG_RTCK_Pin LL_GPIO_PIN_5

/* JTAG_TDO         PC12    SPI3_MOSI */
#define JTAG_TDO_GPIO_Port GPIOC
#define JTAG_TDO_Pin LL_GPIO_PIN_12

/* JTAG_RESET       PA15 */
#define JTAG_RESET_GPIO_Port GPIOA
#define JTAG_RESET_Pin LL_GPIO_PIN_15

/* JTAG_RESET_OD    PD15 */
#define JTAG_RESET_OD_GPIO_Port GPIOD
#define JTAG_RESET_OD_Pin LL_GPIO_PIN_15

/* JTAG_DBGRQ_RXD   PD0     UART4_RX */
#define JTAG_DBGRQ_RXD_GPIO_Port GPIOD
#define JTAG_DBGRQ_RXD_Pin LL_GPIO_PIN_0

/* JTAG_VOUT_EN     PD12 */
#define JTAG_VOUT_EN_GPIO_Port GPIOD
#define JTAG_VOUT_EN_Pin LL_GPIO_PIN_12

/* JTAG_TRST_DIR    PE1 */
#define JTAG_TRST_DIR_GPIO_Port GPIOE
#define JTAG_TRST_DIR_Pin LL_GPIO_PIN_1

/* JTAG_TDI_DIR     PB9 */
#define JTAG_TDI_DIR_GPIO_Port GPIOB
#define JTAG_TDI_DIR_Pin LL_GPIO_PIN_9

/* JTAG_TMS_DIR     PB7     TIM4_CH2 */
#define JTAG_TMS_DIR_GPIO_Port GPIOB
#define JTAG_TMS_DIR_Pin LL_GPIO_PIN_7

/* JTAG_TCK_DIR     PD3 */
#define JTAG_TCK_DIR_GPIO_Port GPIOD
#define JTAG_TCK_DIR_Pin LL_GPIO_PIN_3

/* JTAG_RTCK_DIR    PD6 */
#define JTAG_RTCK_DIR_GPIO_Port GPIOD
#define JTAG_RTCK_DIR_Pin LL_GPIO_PIN_6

/* JTAG_TDO_DIR     PD4 */
#define JTAG_TDO_DIR_GPIO_Port GPIOD
#define JTAG_TDO_DIR_Pin LL_GPIO_PIN_4

/* JTAG_RESET_DIR   PC6 */
#define JTAG_RESET_DIR_GPIO_Port GPIOC
#define JTAG_RESET_DIR_Pin LL_GPIO_PIN_6

/* JTAG_DBGRQ_DIR   PC7 */
#define JTAG_DBGRQ_DIR_GPIO_Port GPIOC
#define JTAG_DBGRQ_DIR_Pin LL_GPIO_PIN_7

/* 操作电平转换器的DIR引脚 */
#define JTAG_TRST_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_TRST_DIR_GPIO_Port, JTAG_TRST_DIR_Pin)
#define JTAG_TRST_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_TRST_DIR_GPIO_Port, JTAG_TRST_DIR_Pin)
#define JTAG_TDI_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_TDI_DIR_GPIO_Port, JTAG_TDI_DIR_Pin)
#define JTAG_TDI_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_TDI_DIR_GPIO_Port, JTAG_TDI_DIR_Pin)
#define JTAG_TMS_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin)
#define JTAG_TMS_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_TMS_DIR_GPIO_Port, JTAG_TMS_DIR_Pin)
#define JTAG_TCK_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_TCK_DIR_GPIO_Port, JTAG_TCK_DIR_Pin)
#define JTAG_TCK_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_TCK_DIR_GPIO_Port, JTAG_TCK_DIR_Pin)
#define JTAG_RTCK_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_RTCK_DIR_GPIO_Port, JTAG_RTCK_DIR_Pin)
#define JTAG_RTCK_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_RTCK_DIR_GPIO_Port, JTAG_RTCK_DIR_Pin)
#define JTAG_TDO_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_TDO_DIR_GPIO_Port, JTAG_TDO_DIR_Pin)
#define JTAG_TDO_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_TDO_DIR_GPIO_Port, JTAG_TDO_DIR_Pin)
#define JTAG_RESET_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_RESET_DIR_GPIO_Port, JTAG_RESET_DIR_Pin)
#define JTAG_RESET_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_RESET_DIR_GPIO_Port, JTAG_RESET_DIR_Pin)
#define JTAG_DBGRQ_OEN_HIGH() LL_GPIO_SetOutputPin(JTAG_DBGRQ_DIR_GPIO_Port, JTAG_DBGRQ_DIR_Pin)
#define JTAG_DBGRQ_OEN_LOW() LL_GPIO_ResetOutputPin(JTAG_DBGRQ_DIR_GPIO_Port, JTAG_DBGRQ_DIR_Pin)

#define JTAG_VOUT_ENABLE() LL_GPIO_SetOutputPin(JTAG_VOUT_EN_GPIO_Port, JTAG_VOUT_EN_Pin)
#define JTAG_VOUT_DISABLE() LL_GPIO_ResetOutputPin(JTAG_VOUT_EN_GPIO_Port, JTAG_VOUT_EN_Pin)

#define JTAG_TCK_PULL_HIGH() LL_GPIO_SetOutputPin(JTAG_TCK_PULL_GPIO_Port, JTAG_TCK_PULL_Pin)
#define JTAG_TCK_PULL_LOW() LL_GPIO_ResetOutputPin(JTAG_TCK_PULL_GPIO_Port, JTAG_TCK_PULL_Pin)

#define JTAG_TCK_EN_HIGH() LL_GPIO_SetOutputPin(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin)
#define JTAG_TCK_EN_LOW() LL_GPIO_ResetOutputPin(JTAG_TCK_EN_GPIO_Port, JTAG_TCK_EN_Pin)

#define JTAG_RESET_OD_HIGH() LL_GPIO_ResetOutputPin(JTAG_RESET_OD_GPIO_Port, JTAG_RESET_OD_Pin) // 反相输出
#define JTAG_RESET_OD_LOW() LL_GPIO_SetOutputPin(JTAG_RESET_OD_GPIO_Port, JTAG_RESET_OD_Pin)    // 反相输出

// BSRR 寄存器高半字复位，低半字置位

#define JTAG_TRST_OUT(x) LL_GPIO_WriteReg(JTAG_TRST_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_TRST_Pin << 16) : (JTAG_TRST_Pin))
#define JTAG_TDI_OUT(x) LL_GPIO_WriteReg(JTAG_TDI_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_TDI_Pin << 16) : (JTAG_TDI_Pin))
#define JTAG_TMS_OUT(x) LL_GPIO_WriteReg(JTAG_TMS_DO_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_TMS_DO_Pin << 16) : (JTAG_TMS_DO_Pin))
#define JTAG_TCK_OUT(x) LL_GPIO_WriteReg(JTAG_TCK_GEN_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_TCK_GEN_Pin << 16) : (JTAG_TCK_GEN_Pin))
#define JTAG_RTCK_OUT(x) LL_GPIO_WriteReg(JTAG_RTCK_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_RTCK_Pin << 16) : (JTAG_RTCK_Pin))
#define JTAG_TDO_OUT(x) LL_GPIO_WriteReg(JTAG_TDO_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_TDO_Pin << 16) : (JTAG_TDO_Pin))
#define JTAG_RESET_OUT(x) LL_GPIO_WriteReg(JTAG_RESET_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_RESET_Pin << 16) : (JTAG_RESET_Pin))
#define JTAG_RESET_OD_OUT(x) LL_GPIO_WriteReg(JTAG_RESET_OD_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_RESET_OD_Pin) : (JTAG_RESET_OD_Pin << 16)) // 反相输出
#define JTAG_DBGRQ_OUT(x) LL_GPIO_WriteReg(JTAG_DBGRQ_RXD_GPIO_Port, BSRR, ((x) == 0) ? (JTAG_DBGRQ_RXD_Pin << 16) : (JTAG_DBGRQ_RXD_Pin))

#define JTAG_TRST_IN() ((LL_GPIO_ReadInputPort(JTAG_TRST_GPIO_Port) & JTAG_TRST_Pin) != 0)
#define JTAG_TDI_IN() ((LL_GPIO_ReadInputPort(JTAG_TDI_GPIO_Port) & JTAG_TDI_Pin) != 0)
#define JTAG_TMS_IN() ((LL_GPIO_ReadInputPort(JTAG_TMS_DI_GPIO_Port) & JTAG_TMS_DI_Pin) != 0)
#define JTAG_TCK_IN() ((LL_GPIO_ReadInputPort(JTAG_TCK_GEN_GPIO_Port) & JTAG_TCK_GEN_Pin) != 0)
#define JTAG_RTCK_IN() ((LL_GPIO_ReadInputPort(JTAG_RTCK_GPIO_Port) & JTAG_RTCK_Pin) != 0)
#define JTAG_TDO_IN() ((LL_GPIO_ReadInputPort(JTAG_TDO_GPIO_Port) & JTAG_TDO_Pin) != 0)
#define JTAG_RESET_IN() ((LL_GPIO_ReadInputPort(JTAG_RESET_GPIO_Port) & JTAG_RESET_Pin) != 0)
#define JTAG_DBGRQ_IN() ((LL_GPIO_ReadInputPort(JTAG_DBGRQ_RXD_GPIO_Port) & JTAG_DBGRQ_RXD_Pin) != 0)

#define JTAG_REG_OPTIMIZE (1U)                // 寄存器优化
#define JTAG_SPI_FIFO_SIZE (16U)              // SPI FIFO 深度
#define JTAG_SWJ_FREQ_MAX (60 * 1000 * 1000U) // 最大频率
#define JTAG_SWJ_FREQ_MIN (5 * 1000U)         // 最小频率
#define JTAG_CLK_DUTY_CYCLE (5.0F / 10U)      // 占空比

typedef enum _jtag_port_enum
{
    JTAG_PORT_GPIO = 0,
    JTAG_PORT_SWD,
    JTAG_PORT_JTAG,
} jtag_port_t;

typedef enum _jtag_gpio_enum
{
    JTAG_GPIO_TRST = 0,
    JTAG_GPIO_TDI,
    JTAG_GPIO_TMS,
    JTAG_GPIO_TCK,
    JTAG_GPIO_RTCK,
    JTAG_GPIO_TDO,
    JTAG_GPIO_RESET,
    JTAG_GPIO_DBGRQ
} jtag_gpio_t;

typedef struct _swj_config_struct
{
    jtag_port_t port;       // 通信模式，SWD或JTAG
    uint32_t tim_div_count; // 定时器计数值
} swj_config_t;

void bsp_jtag_init(void);
uint32_t bsp_jtag_get_time_stamp(void);
void bsp_jtag_enable_port(jtag_port_t port);
void bsp_jtag_deinit(void);
void bsp_jtag_enable_vout(void);
void bsp_jtag_disable_vout(void);
uint32_t bsp_jtag_set_tck_clock(uint32_t freq);
void bsp_jtag_generate_data_cycle(uint32_t count, uint32_t n_bytes);
void bsp_jtag_wait_data_cycle(void);
void bsp_jtag_generate_dummy_cycle(uint32_t count);
void bsp_jtag_write_tms_tx_fifo(uint8_t *data_buff, uint32_t n_bytes);
void bsp_jtag_write_tms_tx_fifo_byte(uint8_t *data_buff);
void bsp_jtag_read_tms_rx_fifo(uint8_t *data_buff, uint32_t n_bytes);
void bsp_jtag_read_tms_rx_fifo_byte(uint8_t *data_buff);
void bsp_jtag_clean_tms_rx_fifo(uint8_t *data_buff, uint32_t max_size);
void bsp_jtag_write_tdi_tx_fifo(uint8_t *data_buff, uint32_t n_bytes);
void bsp_jtag_read_tdo_rx_fifo(uint8_t *data_buff, uint32_t n_bytes);
void bsp_jtag_write_tdi_tx_fifo_byte(uint8_t *data_buff);
void bsp_jtag_read_tdo_rx_fifo_byte(uint8_t *data_buff);
void bsp_jtag_enable_spi_tms(void);
void bsp_jtag_disable_spi_tms(void);
void bsp_jtag_enable_spi_tdi_tdo(void);
void bsp_jtag_disable_spi_tdi_tdo(void);
void bsp_jtag_enable_transfer_tms(void);
void bsp_jtag_disable_transfer_tms(void);
void bsp_jtag_enable_transfer_tdi_tdo(void);
void bsp_jtag_disable_transfer_tdi_tdo(void);
void bsp_jtag_check_gpio_mode(uint32_t mode);

#endif /* __JTAG_H__ */
