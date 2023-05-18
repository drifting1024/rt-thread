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

typedef struct {
    os_ipc_sender_instance_t* sender;
    os_ipc_receiver_instance_t* recv;
} service_a_ctx_t;
static service_a_ctx_t service_a_ctx;

enum {
    SERVICE_A_EVT_TEST = 0,
};

os_err_t service_a_entry_init(os_ipc_receiver_instance_t* inst)
{
    os_ipc_register_event(service_a_ctx.sender, inst, (uint32_t)SERVICE_A_EVT_TEST, OS_IPC_MSG_PRIO_DEFAULT);

    return RT_EOK;
}

void service_a_recv_handler(os_ipc_receiver_instance_t* inst, os_ipc_msg_t* msg)
{
    if (msg->from->id == service_a_ctx.sender->id) {
        switch (msg->event) {
            case SERVICE_A_EVT_TEST:
                rt_kprintf("service_a recv event\n");
                break;
            default:
                break;
        }
    }
}

os_err_t service_a_init(void)
{
    service_a_ctx.sender = os_ipc_create_sender("A");
    service_a_ctx.recv = os_ipc_create_receiver("A", 20, 10, 1024, &service_a_ctx, service_a_entry_init, service_a_recv_handler);
}



typedef struct {
    os_ipc_sender_instance_t* sender;
    os_ipc_receiver_instance_t* recv;
} service_b_ctx_t;
static service_b_ctx_t service_b_ctx;

enum {
    SERVICE_B_EVT_TEST = 0,
};

os_err_t service_b_entry_init(os_ipc_receiver_instance_t* inst)
{
    os_ipc_register_event(service_b_ctx.sender, inst, (uint32_t)SERVICE_B_EVT_TEST, OS_IPC_MSG_PRIO_DEFAULT);

    return RT_EOK;
}

void service_b_recv_handler(os_ipc_receiver_instance_t* inst, os_ipc_msg_t* msg)
{
    if (msg->from->id == service_b_ctx.sender->id) {
        switch (msg->event) {
            case SERVICE_B_EVT_TEST:
                rt_kprintf("service_b recv event\n");
                break;
            default:
                break;
        }
    }
}

os_err_t service_b_init(void)
{
    service_b_ctx.sender = os_ipc_create_sender("B");
    service_b_ctx.recv = os_ipc_create_receiver("B", 20, 10, 1024, &service_b_ctx, service_b_entry_init, service_b_recv_handler);
}

int ipc_send(int argc, char **argv)
{
    os_ipc_send_evt(service_b_ctx.sender, SERVICE_A_EVT_TEST);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ipc_send, ipc_send_b, service_b_send);


int main(void)
{
    rt_kprintf("Hello RT-Thread!\n");
    os_ipc_init();
    service_a_init();
    service_b_init();
    return 0;
}
