/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */
#include <rtthread.h>
#include <stdio.h>
#include <board.h>
#include "os_ipc.h"


int main(void)
{
    rt_kprintf("Hello RT-Thread!\n");

    os_ipc_init();
    service_hrm_init();
    service_ui_init();

    return 0;
}
