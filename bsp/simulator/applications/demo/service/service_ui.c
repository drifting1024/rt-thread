#include <rtthread.h>
#include <stdio.h>
#include <board.h>
#include "os_ipc.h"
#include "service_ui.h"
#include "service_hrm.h"

#define SERVICE_NAME "ui"

typedef struct {
    os_ipc_sender_instance_t* sender;
    os_ipc_receiver_instance_t* recv;
} service_ui_ctx_t;

static service_ui_ctx_t service_ui_ctx;

os_ipc_sender_instance_t* service_ui_get_sender(void)
{
    return service_ui_ctx.sender;
}

os_err_t service_ui_entry_init(os_ipc_receiver_instance_t* inst)
{
    os_ipc_sender_instance_t* hrm_sender = service_hrm_get_sender();
    os_ipc_register_event(hrm_sender, inst, (uint32_t)SERVICE_HRM_EVT_START, OS_IPC_MSG_PRIO_DEFAULT);
    os_ipc_register_event(hrm_sender, inst, (uint32_t)SERVICE_HRM_EVT_VALUE_CHANGE, OS_IPC_MSG_PRIO_DEFAULT);

    return RT_EOK;
}

void service_ui_recv_handler(os_ipc_receiver_instance_t* inst, os_ipc_msg_t* msg)
{
    os_ipc_sender_instance_t* hrm_sender = service_hrm_get_sender();
    if (msg->from->id == hrm_sender->id) {
        switch (msg->event) {
            case SERVICE_HRM_EVT_START:
                rt_kprintf("service_ui recv hrm start\n");
                break;
            case SERVICE_HRM_EVT_VALUE_CHANGE:
                rt_kprintf("service_ui recv hrm value change\n");
                hrm_cur_status_t hrm_status;
                rt_memcpy(&hrm_status, msg->data, msg->len);
                rt_kprintf("hrm recv status [onoff:%d  value:%d]\n", hrm_status.onoff, hrm_status.value);
                break;
            default:
                break;
        }
    }
}

os_err_t service_ui_init(void)
{
    service_ui_ctx.sender = os_ipc_create_sender(SERVICE_NAME);
    service_ui_ctx.recv = os_ipc_create_receiver(
        SERVICE_NAME, 
        20, 
        10, 
        1024, 
        &service_ui_ctx, 
        service_ui_entry_init, 
        service_ui_recv_handler);
}
