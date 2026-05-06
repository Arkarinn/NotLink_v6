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
 * Title:        SW_DP.c CMSIS-DAP SW DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "dap.h"

// #include "ulog.h"

extern dap_data_t dap_data;

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))

static uint8_t swd_tx_buff[JTAG_SPI_FIFO_SIZE] = {0};
static uint8_t swd_rx_buff[JTAG_SPI_FIFO_SIZE] = {0};

// 奇偶校验值计算表
static const uint8_t parity_table[256] = {
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, //
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0  //
};

/**
 * @brief bitbang
 *
 * @param request
 * @param response
 */
void SWJ_Pins(const uint8_t *request, uint8_t *response)
{
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
    uint32_t value;
    uint32_t select;
    uint32_t wait;
    uint32_t timestamp;

    value = (uint32_t)request[0];
    select = (uint32_t)request[1];
    wait = (((uint32_t)request[2]) << 0) |
           (((uint32_t)request[3]) << 8) |
           (((uint32_t)request[4]) << 16) |
           (((uint32_t)request[5]) << 24);

    bsp_jtag_check_gpio_mode(1U);

    if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U)
    {
        JTAG_TCK_OUT((value >> DAP_SWJ_SWCLK_TCK) & 1U);
    }
    if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U)
    {
        JTAG_TMS_OUT((value >> DAP_SWJ_SWDIO_TMS) & 1U);
    }
    if ((select & (1U << DAP_SWJ_TDI)) != 0U)
    {
        JTAG_TDI_OUT((value >> DAP_SWJ_TDI) & 1U);
    }
    if ((select & (1U << DAP_SWJ_nTRST)) != 0U)
    {
        JTAG_TRST_OUT((value >> DAP_SWJ_nTRST) & 1U);
    }
    if ((select & (1U << DAP_SWJ_nRESET)) != 0U)
    {
        JTAG_RESET_OD_OUT((value >> DAP_SWJ_nRESET) & 1U);
    }

    if (wait != 0U)
    {
#if (TIMESTAMP_CLOCK != 0U)
        if (wait > 3000000U)
        {
            wait = 3000000U;
        }
#if (TIMESTAMP_CLOCK >= 1000000U)
        wait *= TIMESTAMP_CLOCK / 1000000U;
#else
        wait /= (1000000U / TIMESTAMP_CLOCK);
#endif
#else
        wait = 1U;
#endif
        timestamp = bsp_jtag_get_time_stamp();
        do
        {
            if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U)
            {
                if ((value >> DAP_SWJ_SWCLK_TCK) ^ JTAG_TCK_IN())
                {
                    continue;
                }
            }
            if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U)
            {
                if ((value >> DAP_SWJ_SWDIO_TMS) ^ JTAG_TMS_IN())
                {
                    continue;
                }
            }
            if ((select & (1U << DAP_SWJ_TDI)) != 0U)
            {
                if ((value >> DAP_SWJ_TDI) ^ JTAG_TDI_IN())
                {
                    continue;
                }
            }
            if ((select & (1U << DAP_SWJ_nTRST)) != 0U)
            {
                if ((value >> DAP_SWJ_nTRST) ^ JTAG_TRST_IN())
                {
                    continue;
                }
            }
            if ((select & (1U << DAP_SWJ_nRESET)) != 0U)
            {
                if ((value >> DAP_SWJ_nRESET) ^ JTAG_RESET_IN())
                {
                    continue;
                }
            }
            break;
        } while ((bsp_jtag_get_time_stamp() - timestamp) < wait);
    }

    value = (JTAG_TCK_IN() << DAP_SWJ_SWCLK_TCK) |
            (JTAG_TMS_IN() << DAP_SWJ_SWDIO_TMS) |
            (JTAG_TDI_IN() << DAP_SWJ_TDI) |
            (JTAG_TDO_IN() << DAP_SWJ_TDO) |
            (JTAG_TRST_IN() << DAP_SWJ_nTRST) |
            (JTAG_RESET_IN() << DAP_SWJ_nRESET);

    *response = (uint8_t)value;
#else
    *response = 0U;
#endif
}

/**
 * @brief Generate SWJ Sequence
 *
 * @param count sequence bit count
 * @param data  pointer to sequence bit data
 */
void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    uint32_t n_bytes;
    uint32_t n_cycle;

    if (count == 0)
    {
        count = 256;
    }

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_transfer_tms();

    while (count > 0)
    {
        n_cycle = (count > (8 * JTAG_SPI_FIFO_SIZE)) ? (8 * JTAG_SPI_FIFO_SIZE) : count;
        n_bytes = (n_cycle + 7U) / 8U;

        bsp_jtag_write_tms_tx_fifo(data ? (uint8_t *)data : swd_tx_buff, n_bytes);
        bsp_jtag_generate_data_cycle(n_cycle, n_bytes);
        bsp_jtag_read_tms_rx_fifo(swd_rx_buff, n_bytes);

        count = (count - n_cycle);
        if (data)
        {
            data = data + n_bytes;
        }
    }

    bsp_jtag_disable_transfer_tms();
    bsp_jtag_disable_spi_tms();
}
#endif

#if (DAP_SWD != 0)

/**
 * @brief Generate SWD Sequence
 *
 * @param info  sequence information
 * @param swdo  pointer to SWDIO generated data
 * @param swdi  pointer to SWDIO captured data
 */
void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    bsp_jtag_check_gpio_mode(0U);

    uint8_t *di_buff = swd_rx_buff;
    uint8_t *do_buff = swd_tx_buff;
    uint32_t n_cycle, n_byte;

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_transfer_tms();

    n_cycle = info & SWD_SEQUENCE_CLK;
    if (n_cycle == 0U)
    {
        n_cycle = 64U; // 最大64
    }

    n_byte = (n_cycle + 7U) / 8U;

    if (info & SWD_SEQUENCE_DIN) // 捕获输入
    {
        if (swdi == NULL)
        {
            goto seq_exit;
        }
        di_buff = swdi;
    }
    else
    {
        if (swdo == NULL)
        {
            goto seq_exit;
        }
        do_buff = (uint8_t *)swdo;
    }

    bsp_jtag_write_tms_tx_fifo(do_buff, n_byte);
    bsp_jtag_generate_data_cycle(n_cycle, n_byte);
    bsp_jtag_read_tms_rx_fifo(di_buff, n_byte);

seq_exit:
    bsp_jtag_disable_transfer_tms();
    bsp_jtag_disable_spi_tms();
}
#endif

#if (DAP_SWD != 0)

/**
 * @brief SWD Transfer I/O
 *
 * @param request   A[3:2] RnW APnDP
 * @param data      DATA[31:0]
 * @return uint8_t  ACK[2:0]
 */
uint8_t SWD_Transfer(uint32_t request, uint32_t *data)
{
    // data可能为NULL

    bsp_jtag_check_gpio_mode(0U);

    bsp_jtag_enable_spi_tms();
    bsp_jtag_enable_transfer_tms();

    uint8_t req_raw = request & 0x0FU;
    uint8_t req_parity = parity_table[req_raw];                 /* 奇偶校验，奇1偶0 */
    uint8_t req = 0x81U | (req_raw << 1U) | (req_parity << 5U); /* req 8bit */

    /* Request ***************************************************************/

    bsp_jtag_write_tms_tx_fifo_byte(&req); // 发request
    bsp_jtag_generate_data_cycle(8U, 1U);

    uint32_t data_val = (data) ? (*data) : (0U);
    uint8_t *data_val_u8p = (uint8_t *)&data_val;
    uint8_t read_n_write = (request & DAP_TRANSFER_RnW) ? 1U : 0U;                  /* 读or写 */
    uint32_t ack_n_cycle = 3U + dap_data.swd_conf.turnaround * (2U - read_n_write); /* 3 ACK + trn */
    uint32_t ack_n_bytes = (ack_n_cycle + 7U) / 8U;
    bsp_jtag_write_tms_tx_fifo(&req, ack_n_bytes); // 发ACK，先塞到FIFO里，加速操作

    bsp_jtag_read_tms_rx_fifo_byte(swd_rx_buff); // 等request Byte传输完毕
    JTAG_TMS_OEN_LOW();                          // SWDIO方向改成输入

    /* ACK *******************************************************************/

    bsp_jtag_generate_data_cycle(ack_n_cycle, ack_n_bytes);

    uint8_t data_parity;
    uint32_t data_n_bytes;
    uint32_t data_n_cycle;

    if (read_n_write)
    {
        // data(32) + p(1) + trn(x)
        data_n_cycle = 32U + 1U + dap_data.swd_conf.turnaround;
    }
    else
    {
        // data(32) + p(1)
        data_n_cycle = 32U + 1U;

        /* 节约时间，提前计算发送数据的校验值 */
        data_parity = 0x01 & (parity_table[data_val_u8p[0]] + //
                              parity_table[data_val_u8p[1]] + //
                              parity_table[data_val_u8p[2]] + //
                              parity_table[data_val_u8p[3]]); //
    }
    data_n_bytes = (data_n_cycle + 7U) / 8U;

    bsp_jtag_read_tms_rx_fifo(swd_rx_buff, ack_n_bytes);                    // 等ACK传输完毕，收ACK
    uint8_t ack = (swd_rx_buff[0] >> dap_data.swd_conf.turnaround) & 0x07U; // TRN最大4bit，只需要读第一字节
    // bsp_jtag_clean_tms_rx_fifo(swd_rx_buff, JTAG_SPI_FIFO_SIZE);

    /* DATA ******************************************************************/

    switch (ack)
    {
    case DAP_TRANSFER_OK:
        if (read_n_write) // read
        {
            bsp_jtag_write_tms_tx_fifo(swd_tx_buff, data_n_bytes);
            bsp_jtag_generate_data_cycle(data_n_cycle, data_n_bytes);
#if 0
            // 一边收一边计算校验值
            bsp_jtag_read_tms_rx_fifo_byte(&data_val_u8p[0]);
            data_parity = parity_table[data_val_u8p[0]]; // 重置
            bsp_jtag_read_tms_rx_fifo_byte(&data_val_u8p[1]);
            data_parity += parity_table[data_val_u8p[1]];
            bsp_jtag_read_tms_rx_fifo_byte(&data_val_u8p[2]);
            data_parity += parity_table[data_val_u8p[2]];
            bsp_jtag_read_tms_rx_fifo_byte(&data_val_u8p[3]);
            data_parity += parity_table[data_val_u8p[3]];
#else
            // 一次性收完
            bsp_jtag_read_tms_rx_fifo(data_val_u8p, 4U);
            data_parity = parity_table[data_val_u8p[0]]; // 重置
            data_parity += parity_table[data_val_u8p[1]];
            data_parity += parity_table[data_val_u8p[2]];
            data_parity += parity_table[data_val_u8p[3]];
#endif

            if (data)
            {
                // NULL则丢弃数据
                *data = data_val;
            }

            bsp_jtag_read_tms_rx_fifo(swd_rx_buff, data_n_bytes - 4U); // 收校验值
            JTAG_TMS_OEN_HIGH();                                       // 重新控制总线

            if ((swd_rx_buff[0] ^ data_parity) & 0x01U)
            {
                ack = DAP_TRANSFER_ERROR;
            }

            uint32_t count = dap_data.transfer.idle_cycles;
            while (count)
            {
                // 128bit 一组
                uint32_t n_cycle = (count > (8 * JTAG_SPI_FIFO_SIZE)) ? (8 * JTAG_SPI_FIFO_SIZE) : count;
                uint32_t n_bytes = (n_cycle + 7U) / 8U;

                // 空周期，必须发送0
                bsp_jtag_write_tms_tx_fifo(swd_tx_buff, n_bytes);
                bsp_jtag_generate_data_cycle(n_cycle, n_bytes);
                bsp_jtag_read_tms_rx_fifo(swd_rx_buff, n_bytes);

                count = (count - n_cycle);
            }
        }
        else // write
        {
            JTAG_TMS_OEN_HIGH();                    // 重新控制总线
            if (dap_data.transfer.idle_cycles >> 3) // idle_cycles大于7，需要分开发送
            {
                bsp_jtag_write_tms_tx_fifo(data_val_u8p, 4U);
                bsp_jtag_write_tms_tx_fifo_byte(&data_parity);
                bsp_jtag_generate_data_cycle(32U + 1U, 5U);
                bsp_jtag_read_tms_rx_fifo(swd_rx_buff, 5U);

                uint32_t count = dap_data.transfer.idle_cycles;
                while (count)
                {
                    // 128bit 一组
                    uint32_t n_cycle = (count > (8 * JTAG_SPI_FIFO_SIZE)) ? (8 * JTAG_SPI_FIFO_SIZE) : count;
                    uint32_t n_bytes = (n_cycle + 7U) / 8U;

                    // 空周期，必须发送0
                    bsp_jtag_write_tms_tx_fifo(swd_tx_buff, n_bytes);
                    bsp_jtag_generate_data_cycle(n_cycle, n_bytes);
                    bsp_jtag_read_tms_rx_fifo(swd_rx_buff, n_bytes);

                    count = (count - n_cycle);
                }
            }
            else
            {
                // 如果idle_cycles小于7，可以和校验位一起发送
                bsp_jtag_write_tms_tx_fifo(data_val_u8p, 4U);
                bsp_jtag_write_tms_tx_fifo_byte(&data_parity);
                bsp_jtag_generate_data_cycle(32U + 1U + dap_data.transfer.idle_cycles, 5U);
                bsp_jtag_read_tms_rx_fifo(swd_rx_buff, 5U);
            }
        }

        // 更新时间戳
        if (request & DAP_TRANSFER_TIMESTAMP)
        {
            dap_data.timestamp = bsp_jtag_get_time_stamp();
        }

        break;
    case DAP_TRANSFER_WAIT:
    case DAP_TRANSFER_FAULT:
        if (dap_data.swd_conf.data_phase) // 始终发送数据周期
        {
            if (read_n_write) // read
            {
                bsp_jtag_write_tms_tx_fifo(swd_tx_buff, data_n_bytes);
                bsp_jtag_generate_data_cycle(data_n_cycle, data_n_bytes);
                bsp_jtag_read_tms_rx_fifo(swd_rx_buff, data_n_bytes);

                JTAG_TMS_OEN_HIGH(); // 重新控制总线
            }
            else // write
            {
                JTAG_TMS_OEN_HIGH(); // 重新控制总线

                bsp_jtag_write_tms_tx_fifo(swd_tx_buff, 5U);
                bsp_jtag_generate_data_cycle(32U + 1U, 5U); //
                bsp_jtag_read_tms_rx_fifo(swd_rx_buff, 5U);
            }
        }
        else
        {
            if (read_n_write) // 发送转换周期
            {
                bsp_jtag_write_tms_tx_fifo_byte(swd_tx_buff);
                bsp_jtag_generate_data_cycle(dap_data.swd_conf.turnaround, 1U); // 转换方向[0, 4]
                bsp_jtag_read_tms_rx_fifo_byte(swd_rx_buff);
            }
            JTAG_TMS_OEN_HIGH(); // 重新控制总线
        }
        break;
    default:
        bsp_jtag_write_tms_tx_fifo(swd_tx_buff, 5U);
        bsp_jtag_generate_data_cycle(32U + 1U + dap_data.swd_conf.turnaround, 5U); // [33, 37]
        bsp_jtag_read_tms_rx_fifo(swd_rx_buff, 5U);

        JTAG_TMS_OEN_HIGH(); // 重新控制总线
        break;
    }

    // bsp_jtag_wait_data_cycle();

    bsp_jtag_disable_transfer_tms();
    bsp_jtag_disable_spi_tms();

    return ack;
}

#endif /* (DAP_SWD != 0) */
