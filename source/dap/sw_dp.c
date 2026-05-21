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

#ifndef __RAM_FUNC
#define __RAM_FUNC __attribute__((section(".RamFunc")))
#endif

extern dap_data_t dap_data;

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))

static uint8_t swd_tx_buff[JTAG_SPI_FIFO_SIZE] = {0};
static uint8_t swd_rx_buff[JTAG_SPI_FIFO_SIZE] = {0};

static __INLINE uint8_t get_parity_u32(uint32_t v)
{
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0x0F;
    return (0x6996 >> v) & 1;
}

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

    // Wait for last cmd
    if (dap_data.rx_data_remain)
    {
        bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], dap_data.rx_data_remain);
        dap_data.rx_data_remain = 0;
    }

    while (count > 0)
    {
        n_cycle = (count > (8 * JTAG_SPI_FIFO_SIZE)) ? (8 * JTAG_SPI_FIFO_SIZE) : count;
        n_bytes = (n_cycle + 7U) / 8U;

        bsp_jtag_write_tms_tx_fifo(data ? (uint8_t *)data : swd_tx_buff, n_bytes);
#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE
        bsp_jtag_generate_data_cycle_with_oe(n_cycle, 0U);
#else  /* CONFIG_OPTIMIZE_TMS_HARD_OE */
        bsp_jtag_generate_data_cycle(n_cycle);
#endif /* CONFIG_OPTIMIZE_TMS_HARD_OE */
        bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], n_bytes);

        count = (count - n_cycle);
        if (data)
        {
            data = data + n_bytes;
        }
    }
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

    // Wait for last cmd
    if (dap_data.rx_data_remain)
    {
        bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], dap_data.rx_data_remain);
        dap_data.rx_data_remain = 0;
    }

    uint8_t *di_buff = swd_rx_buff;
    uint8_t *do_buff = swd_tx_buff;
    uint32_t n_cycle, n_byte;

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
            return;
        }
        di_buff = swdi;
    }
    else
    {
        if (swdo == NULL)
        {
            return;
        }
        do_buff = (uint8_t *)swdo;
    }

    bsp_jtag_write_tms_tx_fifo(do_buff, n_byte);
#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE
    if (info & SWD_SEQUENCE_DIN)
    {
        bsp_jtag_generate_data_cycle_with_oe(0U, n_cycle);
    }
    else
    {
        bsp_jtag_generate_data_cycle_with_oe(n_cycle, 0U);
    }
#else  /* CONFIG_OPTIMIZE_TMS_HARD_OE */
    bsp_jtag_generate_data_cycle(n_cycle);
#endif /* CONFIG_OPTIMIZE_TMS_HARD_OE */
    bsp_jtag_read_tms_rx_fifo(di_buff, n_byte);
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
__RAM_FUNC uint8_t SWD_Transfer(uint32_t request, uint32_t *data)
{
    bsp_jtag_check_gpio_mode(0U);

    uint32_t req_raw = request & 0x0000000FU;
    uint8_t req_parity = get_parity_u32(req_raw);                  /* 奇偶校验，奇1偶0 */
    swd_tx_buff[0] = 0x81U | (req_raw << 1U) | (req_parity << 5U); /* req 8bit */

#ifdef CONFIG_OPTIMIZE_TMS_HARD_OE

    uint32_t read_n_write = (request & DAP_TRANSFER_RnW) ? 1U : 0U;                 /* 读or写 */
    uint32_t ack_n_cycle = 3U + dap_data.swd_conf.turnaround * (2U - read_n_write); /* 3 ACK + trn */
    uint32_t req_ack_n_bytes = (8U + ack_n_cycle + 7U) / 8U;

    uint32_t data_val = 0;
    uint8_t *data_val_u8p = (uint8_t *)(&data_val);

    bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], req_ack_n_bytes); // Req+ACK

    // Wait for last cmd
    if (dap_data.rx_data_remain)
    {
        bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], dap_data.rx_data_remain);
        dap_data.rx_data_remain = 0;
    }

    bsp_jtag_generate_data_cycle_with_oe(8U, ack_n_cycle);

    // Data
    uint32_t n_cycle = read_n_write ? (33U + dap_data.swd_conf.turnaround) : 33U;
    uint32_t n_bytes = read_n_write ? ((33U + dap_data.swd_conf.turnaround + 7U) / 8U) : ((33U + 7U) / 8u);

    // Write data
    if (!read_n_write)
    {
        data_val = *data;
        swd_tx_buff[0] = data_val_u8p[0];
        swd_tx_buff[1] = data_val_u8p[1];
        swd_tx_buff[2] = data_val_u8p[2];
        swd_tx_buff[3] = data_val_u8p[3];
        swd_tx_buff[4] = get_parity_u32(data_val);
    }

    // Wait
    bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], req_ack_n_bytes);
    uint8_t ack = (swd_rx_buff[1U] >> dap_data.swd_conf.turnaround) & 0x07U; // TRN最大4bit，不会超过第一个字节

    if (read_n_write)
    {
        switch (ack)
        {
        case DAP_TRANSFER_OK:
            bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
            bsp_jtag_generate_data_cycle_with_oe(0U, n_cycle);

            if (request & DAP_TRANSFER_TIMESTAMP)
            {
                dap_data.timestamp = bsp_jtag_get_time_stamp();
            }

            // Wait
            bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], n_bytes);
            data_val_u8p[0] = swd_rx_buff[0];
            data_val_u8p[1] = swd_rx_buff[1];
            data_val_u8p[2] = swd_rx_buff[2];
            data_val_u8p[3] = swd_rx_buff[3];

            // data may be NULL
            if (data)
            {
                *data = data_val;
            }

            // Check parity
            if ((swd_rx_buff[4] ^ get_parity_u32(data_val)) & 0x01U)
            {
                ack = DAP_TRANSFER_ERROR;
            }
            break;

        case DAP_TRANSFER_WAIT:
        case DAP_TRANSFER_FAULT:
            if (dap_data.swd_conf.data_phase) // 始终发送数据周期
            {
                bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
                bsp_jtag_generate_data_cycle_with_oe(0U, n_cycle);
                dap_data.rx_data_remain = n_bytes;
            }
            else
            {
                bsp_jtag_write_tms_tx_fifo_byte(&swd_tx_buff[0]);
                bsp_jtag_generate_data_cycle_with_oe(dap_data.swd_conf.turnaround, 0U);
                dap_data.rx_data_remain = dap_data.swd_conf.turnaround;
            }
            break;

        default:
            bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
            bsp_jtag_generate_data_cycle_with_oe(0U, n_cycle);
            dap_data.rx_data_remain = n_bytes;
            break;
        }
    }
    else
    {
        switch (ack)
        {
        case DAP_TRANSFER_OK:
            bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
            bsp_jtag_generate_data_cycle_with_oe(n_cycle, 0U);
            dap_data.rx_data_remain = n_bytes;
            if (request & DAP_TRANSFER_TIMESTAMP)
            {
                dap_data.timestamp = bsp_jtag_get_time_stamp();
            }
            break;

        case DAP_TRANSFER_WAIT:
        case DAP_TRANSFER_FAULT:
            if (dap_data.swd_conf.data_phase) // 始终发送数据周期
            {
                bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
                bsp_jtag_generate_data_cycle_with_oe(n_cycle, 0U);
                dap_data.rx_data_remain = n_bytes;
            }
            break;

        default:
            bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
            bsp_jtag_generate_data_cycle_with_oe(n_cycle, 0U);
            dap_data.rx_data_remain = n_bytes;
            break;
        }
    }

#else /* CONFIG_OPTIMIZE_TMS_HARD_OE */

    bsp_jtag_write_tms_tx_fifo_byte(&swd_tx_buff[0]); // 发request
    bsp_jtag_generate_data_cycle(8U);

    uint32_t data_val = (data) ? (*data) : (0U);
    uint8_t *data_val_u8p = (uint8_t *)&data_val;
    uint8_t read_n_write = (request & DAP_TRANSFER_RnW) ? 1U : 0U;                  /* 读or写 */
    uint32_t ack_n_cycle = 3U + dap_data.swd_conf.turnaround * (2U - read_n_write); /* 3 ACK + trn */
    uint32_t ack_n_bytes = (ack_n_cycle + 7U) / 8U;                                 //
    bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], ack_n_bytes);                       // 发ACK

    bsp_jtag_read_tms_rx_fifo_byte(&swd_rx_buff[0]); // 等request Byte传输完毕
    JTAG_TMS_OEN_LOW();                              // SWDIO方向改成输入

    bsp_jtag_generate_data_cycle(ack_n_cycle);

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
        data_parity = get_parity_u32(data_val);
    }
    data_n_bytes = (data_n_cycle + 7U) / 8U;

    bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], ack_n_bytes);                // 等ACK传输完毕，收ACK
    uint8_t ack = (swd_rx_buff[0] >> dap_data.swd_conf.turnaround) & 0x07U; // TRN最大4bit，只需要读第一字节
    // bsp_jtag_clean_tms_rx_fifo(&swd_rx_buff[0], JTAG_SPI_FIFO_SIZE);

    switch (ack)
    {
    case DAP_TRANSFER_OK:
        if (read_n_write) // read
        {
            bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], data_n_bytes);
            bsp_jtag_generate_data_cycle(data_n_cycle);

            // 一次性收完
            bsp_jtag_read_tms_rx_fifo(data_val_u8p, 4U);
            data_parity = get_parity_u32(data_val);

            if (data)
            {
                // NULL则丢弃数据
                *data = data_val;
            }

            bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], data_n_bytes - 4U); // 收校验值
            JTAG_TMS_OEN_HIGH();                                           // 重新控制总线

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
                bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
                bsp_jtag_generate_data_cycle(n_cycle);
                bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], n_bytes);

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
                bsp_jtag_generate_data_cycle(32U + 1U);
                bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], 5U);

                uint32_t count = dap_data.transfer.idle_cycles;
                while (count)
                {
                    // 128bit 一组
                    uint32_t n_cycle = (count > (8 * JTAG_SPI_FIFO_SIZE)) ? (8 * JTAG_SPI_FIFO_SIZE) : count;
                    uint32_t n_bytes = (n_cycle + 7U) / 8U;

                    // 空周期，必须发送0
                    bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], n_bytes);
                    bsp_jtag_generate_data_cycle(n_cycle);
                    bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], n_bytes);

                    count = (count - n_cycle);
                }
            }
            else
            {
                // 如果idle_cycles小于7，可以和校验位一起发送
                bsp_jtag_write_tms_tx_fifo(data_val_u8p, 4U);
                bsp_jtag_write_tms_tx_fifo_byte(&data_parity);
                bsp_jtag_generate_data_cycle(32U + 1U + dap_data.transfer.idle_cycles);
                bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], 5U);
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
                bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], data_n_bytes);
                bsp_jtag_generate_data_cycle(data_n_cycle);
                bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], data_n_bytes);

                JTAG_TMS_OEN_HIGH(); // 重新控制总线
            }
            else // write
            {
                JTAG_TMS_OEN_HIGH(); // 重新控制总线

                bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], 5U);
                bsp_jtag_generate_data_cycle(32U + 1U); //
                bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], 5U);
            }
        }
        else
        {
            if (read_n_write) // 发送转换周期
            {
                bsp_jtag_write_tms_tx_fifo_byte(&swd_tx_buff[0]);
                bsp_jtag_generate_data_cycle(dap_data.swd_conf.turnaround); // 转换方向[0, 4]
                bsp_jtag_read_tms_rx_fifo_byte(&swd_rx_buff[0]);
            }
            JTAG_TMS_OEN_HIGH(); // 重新控制总线
        }
        break;

    default:
        bsp_jtag_write_tms_tx_fifo(&swd_tx_buff[0], 5U);
        bsp_jtag_generate_data_cycle(32U + 1U + dap_data.swd_conf.turnaround); // [33, 37]
        bsp_jtag_read_tms_rx_fifo(&swd_rx_buff[0], 5U);

        JTAG_TMS_OEN_HIGH(); // 重新控制总线
        break;
    }

#endif /* CONFIG_OPTIMIZE_TMS_HARD_OE */

    return ack;
}

#endif /* (DAP_SWD != 0) */
