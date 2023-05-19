#include <rtthread.h>
#include <stdio.h>
#include <board.h>
#include "os_ipc.h"
#include "service_hrm.h"

#define SERVICE_NAME "hrm"


typedef struct {
    os_ipc_sender_instance_t* sender;
    os_ipc_receiver_instance_t* recv;
    hrm_cur_status_t cur_status;
} service_hrm_ctx_t;

static service_hrm_ctx_t service_hrm_ctx;

os_ipc_sender_instance_t* service_hrm_get_sender(void)
{
    return service_hrm_ctx.sender;
}

os_err_t service_hrm_entry_init(os_ipc_receiver_instance_t* inst)
{
    // os_ipc_register_event(service_hrm_ctx.sender, inst, (uint32_t)SERVICE_HRM_EVT_VALUE_CHANGE, OS_IPC_MSG_PRIO_DEFAULT);

    return RT_EOK;
}

void service_hrm_recv_handler(os_ipc_receiver_instance_t* inst, os_ipc_msg_t* msg)
{
    if (msg->from->id == service_hrm_ctx.sender->id) {
        switch (msg->event) {
            // case xxx:
            //     break;
            default:
                break;
        }
    }
}

os_err_t service_hrm_init(void)
{
    service_hrm_ctx.sender = os_ipc_create_sender(SERVICE_NAME);
    service_hrm_ctx.recv = os_ipc_create_receiver(
        SERVICE_NAME, 
        20, 
        10, 
        1024, 
        &service_hrm_ctx, 
        service_hrm_entry_init, 
        service_hrm_recv_handler);
}


#include <time.h>
int service_hrm_start(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("need 2 para at least\n");
        return 0;
    }
    int cmd = atoi(argv[1]);

    switch (cmd) {
        case 0:
            os_ipc_send_evt(service_hrm_ctx.sender, SERVICE_HRM_EVT_START);
            break;
        case 1:
            srand((uint32_t)time(NULL));
            service_hrm_ctx.cur_status.onoff = true;
            service_hrm_ctx.cur_status.value = rand() % 0xff;
            rt_kprintf("hrm send status [onoff:%d  value:%d]\n", service_hrm_ctx.cur_status.onoff, service_hrm_ctx.cur_status.value);
            os_ipc_send_evt_data(
                service_hrm_ctx.sender, 
                SERVICE_HRM_EVT_VALUE_CHANGE,
                &service_hrm_ctx.cur_status,
                sizeof(hrm_cur_status_t));
            break;
        default:
            break;
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(service_hrm_start, hrm_start, hrm simulation);
