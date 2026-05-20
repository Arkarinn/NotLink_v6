/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        JTAG_DP.c CMSIS-DAP JTAG DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "dap.h"

// #include "ulog.h"

#if (DAP_JTAG != 0)

static uint8_t jtag_tx_buff[JTAG_SPI_FIFO_SIZE] = {0};
static uint8_t jtag_rx_buff[JTAG_SPI_FIFO_SIZE] = {0};

static const uint8_t ack_remap_table[8] = {0, 2, 1, 3, 4, 6, 5, 7}; // JTAG的回复和SWD不一样，前两位交换

#endif /* (DAP_JTAG != 0) */

extern dap_data_t dap_data;

#if (DAP_JTAG != 0)

/**
 * @brief Generate JTAG Sequence
 *
 * @param info  sequence information
 * @param tdi   pointer to TDI generated data
 * @param tdo   pointer to TDO captured data
 */
void JTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    uint32_t n_cycle, n_byte;

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tdi_tdo();
    bsp_jtag_enable_transfer_tdi_tdo();

    JTAG_TMS_OUT((info & JTAG_SEQUENCE_TMS) ? 1U : 0U);                              // TMS固定
    LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_OUTPUT); // 手动控制

    n_cycle = info & JTAG_SEQUENCE_TCK;
    if (n_cycle == 0U)
    {
        n_cycle = 64U; // 最大64
    }
    n_byte = (n_cycle + 7) / 8;

    bsp_jtag_write_tdi_tx_fifo(((tdi) ? (uint8_t *)tdi : jtag_tx_buff), n_byte);
    bsp_jtag_generate_data_cycle(n_cycle);
    bsp_jtag_read_tdo_rx_fifo((((info & JTAG_SEQUENCE_TDO) && tdo) ? tdo : jtag_rx_buff), n_byte);

    LL_GPIO_SetPinMode(JTAG_TMS_DO_GPIO_Port, JTAG_TMS_DO_Pin, LL_GPIO_MODE_ALTERNATE); // 外设控制
    bsp_jtag_disable_transfer_tdi_tdo();
    bsp_jtag_disable_spi_tdi_tdo();
}

/**
 * @brief JTAG Read IDCODE register
 *
 * @return uint32_t value read
 */
uint32_t JTAG_ReadIDCode(void)
{
    uint8_t tms;
    uint8_t tdi;

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_spi_tdi_tdo();

    /* TMS *******************************************************************/

    bsp_jtag_enable_transfer_tms();

    /*  Idle            1->
        Select-DR-Scan  0->
        Capture-DR      0->
        Shift-DR
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(3U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    /* DATA ******************************************************************/

    /*  TDO <- (before) <- (device) <- (after) <- TDI */

    bsp_jtag_enable_transfer_tdi_tdo();

    tms = 0x00; // 保持状态机
    tdi = 0xFF; // 无意义，其他设备处于bypass状态，并且延迟一个周期

    uint32_t n_cycle = dap_data.jtag_dev.index;
    while (n_cycle) // 前面有设备，需要填充前面的，处于bypass状态的DR，每个设备延迟一个周期
    {
        uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
        uint32_t b = (n + 7) / 8;
        for (uint32_t i = 0; i < b; i++)
        {
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        }
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b); // 清理接收缓冲区
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

        n_cycle -= n;
    }

    uint32_t tms_u32 = 0x80000000; // 最后一个周期切换TMS
    uint32_t tdi_u32 = 0xFFFFFFFF;
    bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4); // Exit1-DR
    bsp_jtag_write_tdi_tx_fifo((uint8_t *)&tdi_u32, 4);
    bsp_jtag_generate_data_cycle(32U);
    bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U); // 丢弃
    bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);

    uint32_t id_code = ((uint32_t)jtag_rx_buff[0] << 0U) |
                       ((uint32_t)jtag_rx_buff[1] << 8U) |
                       ((uint32_t)jtag_rx_buff[2] << 16U) |
                       ((uint32_t)jtag_rx_buff[3] << 24U);

    bsp_jtag_disable_transfer_tdi_tdo();

    /* TMS *******************************************************************/

    /*  Exit1-DR    1->
        Update-DR   0->
        Idle
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(2U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    bsp_jtag_disable_transfer_tms();

    bsp_jtag_disable_spi_tms();
    bsp_jtag_disable_spi_tdi_tdo();

    return id_code;
}

/**
 * @brief JTAG Write ABORT register
 *
 * @param data value to write
 */
void JTAG_WriteAbort(uint32_t data)
{
    uint8_t tms;
    uint8_t tdi;

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_spi_tdi_tdo();

    /* TMS *******************************************************************/

    bsp_jtag_enable_transfer_tms();

    /*  Idle            1->
        Select-DR-Scan  0->
        Capture-DR      0->
        Shift-DR
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(3U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    /* DATA ******************************************************************/

    /*  TDO <- (before) <- (device) <- (after) <- TDI */

    bsp_jtag_enable_transfer_tdi_tdo();

    tms = 0x00; // 保持状态机
    tdi = 0xFF; // 无意义，其他设备处于bypass状态，并且延迟一个周期

    uint32_t n_cycle = dap_data.jtag_dev.index;
    while (n_cycle) // 填充菊花链前的bypass状态的DR，每个延迟一个周期
    {
        uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
        uint32_t b = (n + 7) / 8;
        for (uint32_t i = 0; i < b; i++)
        {
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        }
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

        n_cycle -= n;
    }

    tdi = 0x00; // RnW=0 A2=0 A3=0
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
    bsp_jtag_generate_data_cycle(3U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);
    bsp_jtag_read_tdo_rx_fifo_byte(jtag_rx_buff);

    n_cycle = dap_data.jtag_dev.count - dap_data.jtag_dev.index - 1U;
    if (n_cycle) // 如果后面还有设备
    {
        uint32_t tms_u32 = 0x00000000; // 这里不着急切换TMS
        bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4);
        bsp_jtag_write_tdi_tx_fifo((uint8_t *)&data, 4);
        bsp_jtag_generate_data_cycle(32U);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);

        while (n_cycle) // 填充菊花链后的bypass状态的DR
        {
            tdi = 0xFF;

            uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
            uint32_t b = (n + 7) / 8;
            for (uint32_t i = 0; i < (b - 1); i++)
            {
                bsp_jtag_write_tms_tx_fifo_byte(&tms);
                bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
            }

            if (n_cycle <= 8U * JTAG_SPI_FIFO_SIZE) // 最后一次操作
            {
                tms = 1U << ((n % 8) - 1);
            }
            bsp_jtag_write_tms_tx_fifo_byte(&tms); // Exit1-DR
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
            bsp_jtag_generate_data_cycle(n);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

            n_cycle -= n;
        }
    }
    else // 如果后面没有设备
    {
        uint32_t tms_u32 = 0x80000000;                      // 必须切换TMS
        bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4); // Exit1-DR
        bsp_jtag_write_tdi_tx_fifo((uint8_t *)&data, 4);
        bsp_jtag_generate_data_cycle(32U);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);
    }

    bsp_jtag_disable_transfer_tdi_tdo();

    /* TMS *******************************************************************/

    /*  Exit1-DR    1->
        Update-DR   0->
        Idle
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(2U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    bsp_jtag_disable_transfer_tms();

    bsp_jtag_disable_spi_tms();
    bsp_jtag_disable_spi_tdi_tdo();
}

/**
 * @brief JTAG Set IR
 *
 * @param ir IR value
 */
void JTAG_IR(uint32_t ir)
{
    uint8_t tms;
    uint8_t tdi;

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_spi_tdi_tdo();

    /* TMS *******************************************************************/

    bsp_jtag_enable_transfer_tms();

    /*  Idle            1->
        Select-DR-Scan  1->
        Select-IR-Scan  0->
        Capture-IR      0->
        Shift-IR
    */

    tms = 0x03;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(4);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    /* DATA ******************************************************************/

    /*  TDO <- (before) <- (device) <- (after) <- TDI */

    bsp_jtag_enable_transfer_tdi_tdo();

    tms = 0x00; // 保持状态机
    tdi = 0xFF; // 填充菊花链前的IR为0xFF，即bypass

    uint32_t n_cycle = dap_data.jtag_dev.ir_before[dap_data.jtag_dev.index];
    while (n_cycle) // 填充菊花链前的IR，此阶段可能没有
    {
        uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
        uint32_t b = (n + 7) / 8;
        for (uint32_t i = 0; i < b; i++)
        {
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        }
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

        n_cycle -= n;
    }

    if (dap_data.jtag_dev.ir_after[dap_data.jtag_dev.index] > 0) // 后面是否有设备，会影响TMS的转变时机
    {
        /* 后面还有设备，TMS不需要在这一阶段转变 */
        /* 虽然理论上IR长度最长可达255，但是此函数的参数只有32bit，也就是IR长度必然不大于32 */

        n_cycle = dap_data.jtag_dev.ir_length[dap_data.jtag_dev.index]; // <=32
        if (n_cycle)
        {
            tms = 0x00;
            uint32_t n = n_cycle;
            uint32_t b = (n + 7) / 8; // 必然>=1
            for (uint32_t i = 0; i < b; i++)
            {
                tdi = (uint8_t)(ir >> (i * 8U));
                bsp_jtag_write_tms_tx_fifo_byte(&tms);
                bsp_jtag_write_tdi_tx_fifo_byte(&tdi); // 填充此设备的IR
            }
            bsp_jtag_generate_data_cycle(n);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);
        }

        n_cycle = dap_data.jtag_dev.ir_after[dap_data.jtag_dev.index];
        while (n_cycle) // 填充菊花链后的IR，把IR推到合适的位置
        {
            tdi = 0xFF; // 填充菊花链后的IR为0xFF，即bypass
            uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
            uint32_t b = (n + 7) / 8; // 必然>=1

            for (uint32_t i = 0; i < (b - 1); i++)
            {
                bsp_jtag_write_tms_tx_fifo_byte(&tms);
                bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
            }

            if (n_cycle <= 8U * JTAG_SPI_FIFO_SIZE)
            {
                tms = 1U << (n % 8U - 1); // 最后一个周期，1进入Exit1-IR
            }
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
            bsp_jtag_generate_data_cycle(n);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

            n_cycle -= n;
        }
    }
    else
    {
        /* 后面没有设备了，TMS要在最后一个时钟周期锁存 */

        n_cycle = dap_data.jtag_dev.ir_length[dap_data.jtag_dev.index]; // <=32
        uint32_t n = n_cycle;
        uint32_t b = (n + 7) / 8;

        for (uint32_t i = 0; i < (b - 1); i++)
        {
            tdi = (uint8_t)(ir >> (8U * b));
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        }

        tms = 1U << (n % 8 - 1);
        tdi = ir >> ((b - 1) * 8);
        bsp_jtag_write_tms_tx_fifo_byte(&tms); // 最后一个周期，1进入Exit1-IR
        bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);
    }

    bsp_jtag_disable_transfer_tdi_tdo();

    /* TMS *******************************************************************/

    /*  Exit1-IR    1->
        Update-IR   0->
        Idle
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(2U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    bsp_jtag_disable_transfer_tms();

    bsp_jtag_disable_spi_tms();
    bsp_jtag_disable_spi_tdi_tdo();
}

/**
 * @brief JTAG Transfer I/O
 *
 * @param request   A[3:2] RnW APnDP
 * @param data      DATA[31:0]
 * @return uint8_t  ACK[2:0]
 */
uint8_t JTAG_Transfer(uint32_t request, uint32_t *data)
{
    uint8_t ack;
    uint8_t tms;
    uint8_t tdi;

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_spi_tdi_tdo();

    /* TMS *******************************************************************/

    bsp_jtag_enable_transfer_tms();

    /*  Idle            1->
        Select-DR-Scan  0->
        Capture-DR      0->
        Shift-DR
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(3U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

    /* DATA ******************************************************************/

    /*  TDO <- (before) <- (device) <- (after) <- TDI */

    bsp_jtag_enable_transfer_tdi_tdo();

    tms = 0x00; // 保持状态机
    tdi = 0xFF; // 无意义，其他设备处于bypass状态，并且延迟一个周期

    uint32_t n_cycle = dap_data.jtag_dev.index;
    while (n_cycle) // 填充菊花链前的bypass状态的DR
    {
        uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
        uint32_t b = (n + 7) / 8;
        for (uint32_t i = 0; i < b; i++)
        {
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
            bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
        }
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
        bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

        n_cycle -= n;
    }

    tms = 0x00;         // 保持状态机
    tdi = request >> 1; // APnDP无意义(由IR决定)，所以去掉一位
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_write_tdi_tx_fifo_byte(&tdi); // 发送req
    bsp_jtag_generate_data_cycle(3U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);
    bsp_jtag_read_tdo_rx_fifo_byte(jtag_rx_buff + 1);
    ack = ack_remap_table[jtag_rx_buff[0 + 1] & 0x07]; // 通过映射表快速转换

    if (ack != DAP_TRANSFER_OK)
    {
        bsp_jtag_disable_transfer_tdi_tdo();

        /*  Shift-DR    1->
            Exit1-DR    1->
            Update-DR   0->
            Idle
        */

        tms = 0x03;                            // 011 LSB
        bsp_jtag_write_tms_tx_fifo_byte(&tms); // Idle
        bsp_jtag_generate_data_cycle(3U);
        bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

        goto idle_cycle;
    }

    if (request & DAP_TRANSFER_RnW)
    {
        // 读操作

        n_cycle = dap_data.jtag_dev.count - dap_data.jtag_dev.index - 1U;
        if (n_cycle)
        {
            // 后面还有设备，TMS不在数据阶段转变

            uint32_t tms_u32 = 0x00000000;
            uint32_t tdi_u32 = 0xFFFFFFFF;
            bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4);
            bsp_jtag_write_tdi_tx_fifo((uint8_t *)&tdi_u32, 4);
            bsp_jtag_generate_data_cycle(32U);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);

            if (data != 0) // data可能为NULL，此时丢弃数据
            {
                *data = ((uint32_t)jtag_rx_buff[0] << 0U) |
                        ((uint32_t)jtag_rx_buff[1] << 8U) |
                        ((uint32_t)jtag_rx_buff[2] << 16U) |
                        ((uint32_t)jtag_rx_buff[3] << 24U);
            }

            while (n_cycle) // 填充菊花链后的bypass状态的DR
            {
                tdi = 0xFF;

                uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
                uint32_t b = (n + 7) / 8;
                for (uint32_t i = 0; i < (b - 1); i++)
                {
                    bsp_jtag_write_tms_tx_fifo_byte(&tms);
                    bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
                }

                if (n_cycle <= 8U * JTAG_SPI_FIFO_SIZE)
                {
                    tms = 1U << (n % 8 - 1); // Exit1-DR
                }
                bsp_jtag_write_tms_tx_fifo_byte(&tms);
                bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
                bsp_jtag_generate_data_cycle(n);
                bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
                bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

                n_cycle -= n;
            }
        }
        else
        {
            // 后面没有设备，TMS在数据阶段尾部转变

            uint32_t tms_u32 = 0x80000000;
            uint32_t tdi_u32 = 0xFFFFFFFF;
            bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4); // Exit1-DR
            bsp_jtag_write_tdi_tx_fifo((uint8_t *)&tdi_u32, 4);
            bsp_jtag_generate_data_cycle(32U);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);

            if (data)
            {
                *data = ((uint32_t)jtag_rx_buff[0] << 0U) |
                        ((uint32_t)jtag_rx_buff[1] << 8U) |
                        ((uint32_t)jtag_rx_buff[2] << 16U) |
                        ((uint32_t)jtag_rx_buff[3] << 24U);
            }
        }
    }
    else
    {
        // 写操作，写完进入Exit1-DR

        n_cycle = dap_data.jtag_dev.count - dap_data.jtag_dev.index - 1U;
        if (n_cycle)
        {
            uint32_t tms_u32 = 0x00000000;
            uint32_t tdi_u32 = (data != 0) ? (*data) : 0x00000000;
            bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4);
            bsp_jtag_write_tdi_tx_fifo((uint8_t *)&tdi_u32, 4); // 写入数据
            bsp_jtag_generate_data_cycle(32U);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);

            while (n_cycle) // 填充菊花链后的bypass状态的DR
            {
                tms = 0x00;
                tdi = 0xFF;

                uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
                uint32_t b = (n + 7) / 8;
                for (uint32_t i = 0; i < (b - 1); i++)
                {
                    bsp_jtag_write_tms_tx_fifo_byte(&tms);
                    bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
                }

                if (n_cycle <= 8U * JTAG_SPI_FIFO_SIZE)
                {
                    tms = 1U << (n % 8 - 1); // Exit1-DR
                }
                bsp_jtag_write_tms_tx_fifo_byte(&tms);
                bsp_jtag_write_tdi_tx_fifo_byte(&tdi);
                bsp_jtag_generate_data_cycle(n);
                bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);
                bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, b);

                n_cycle -= n;
            }
        }
        else
        {
            uint32_t tms_u32 = 0x80000000; // 最后一个周期TMS转变
            uint32_t tdi_u32 = 0xFFFFFFFF;
            bsp_jtag_write_tms_tx_fifo((uint8_t *)&tms_u32, 4); // Exit1-DR
            bsp_jtag_write_tdi_tx_fifo((uint8_t *)&tdi_u32, 4);
            bsp_jtag_generate_data_cycle(32U);
            bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, 4U);
            bsp_jtag_read_tdo_rx_fifo(jtag_rx_buff, 4U);
        }
    }

    /* TMS *******************************************************************/

    bsp_jtag_disable_transfer_tdi_tdo();

    /*  Exit1-DR    1->
        Update-DR   0->
        Idle
    */

    tms = 0x01;
    bsp_jtag_write_tms_tx_fifo_byte(&tms);
    bsp_jtag_generate_data_cycle(2U);
    bsp_jtag_read_tms_rx_fifo_byte(jtag_rx_buff);

idle_cycle:

    /* Capture Timestamp */
    if (request & DAP_TRANSFER_TIMESTAMP)
    {
        dap_data.timestamp = bsp_jtag_get_time_stamp();
    }

    // idle cycle

    n_cycle = dap_data.transfer.idle_cycles;
    while (n_cycle) // 填充菊花链后的bypass状态的DR
    {
        tms = 0x00; // 不变
        tdi = 0xFF;

        uint32_t n = (n_cycle > 8U * JTAG_SPI_FIFO_SIZE) ? (8U * JTAG_SPI_FIFO_SIZE) : n_cycle;
        uint32_t b = (n + 7) / 8;
        for (uint32_t i = 0; i < b; i++)
        {
            bsp_jtag_write_tms_tx_fifo_byte(&tms);
        }
        bsp_jtag_generate_data_cycle(n);
        bsp_jtag_read_tms_rx_fifo(jtag_rx_buff, b);

        n_cycle -= n;
    }

    bsp_jtag_disable_transfer_tms();

    bsp_jtag_disable_spi_tms();
    bsp_jtag_disable_spi_tdi_tdo();

    return ack;
}

#endif /* (DAP_JTAG != 0) */
