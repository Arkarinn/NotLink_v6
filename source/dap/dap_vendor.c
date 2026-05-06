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
 * Title:        DAP_vendor.c CMSIS-DAP Vendor Commands
 *
 *---------------------------------------------------------------------------*/

#include "dap.h"
#include "dap_config.h"

//**************************************************************************************************
/**

The file DAP_vendor.c provides template source code for extension of a Debug Unit with
Vendor Commands. Copy this file to the project folder of the Debug Unit and add the
file to the MDK-ARM project under the file group Configuration.
*/

/**
 * @brief Process DAP Vendor Command and prepare Response Data
 *
 * @param request   pointer to request data
 * @param response  pointer to response data
 * @return uint32_t number of bytes in response (lower 16 bits)
 *                  number of bytes in request (upper 16 bits)
 */
uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response)
{
    uint32_t num = (1U << 16) | 1U;

    uint8_t id = *request;
    *response = id; // copy Command ID
    response++;
    request++;

    switch (id)
    {
#if (DAP_VENDOR_SPI != 0)

#endif /* (DAP_VENDOR_SPI != 0) */

#if (DAP_VENDOR_IIC != 0)

#endif /* (DAP_VENDOR_IIC != 0) */

    default:
        *(response - 1) = ID_DAP_Invalid; // 无效ID
        break;
    }

    return (num);
}
