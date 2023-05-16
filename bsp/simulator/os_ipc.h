#pragma once
#include <rtdef.h>
#include <stdint.h>
#include <list.h>

typedef rt_err_t os_err_t;       /**< Type for error number */
typedef void* os_queue_t;
typedef void* os_mutex_t;
typedef void* os_thread_t;
typedef uint32_t os_priority_t;      // 1->highest prio,  31->lowest prio
typedef uint32_t os_stack_size_t;    // size with bytes


typedef struct os_ipc_msg os_ipc_msg_t;
typedef struct os_ipc_sender_instance os_ipc_sender_instance_t;
typedef struct os_ipc_receiver_instance os_ipc_receiver_instance_t;


os_err_t os_ipc_init(void);

typedef enum {
    OS_IPC_MSG_PRIO_HIGH,
    OS_IPC_MSG_PRIO_NORMAL,
    OS_IPC_MSG_PRIO_DEFAULT = OS_IPC_MSG_PRIO_NORMAL,
} os_ipc_msg_prio_t;

struct os_ipc_msg {
    os_ipc_sender_instance_t* from;
    uint32_t event;
    uint32_t len;
    uint8_t data[0];
};

typedef struct os_ipc_sender_register os_ipc_sender_register_t;
struct os_ipc_sender_register{
    struct list_head list;
    uint32_t event;
    os_ipc_msg_prio_t prio;
    os_ipc_receiver_instance_t* receiver;
};


struct os_ipc_sender_instance{
    uint32_t id;
    os_mutex_t lock;
    struct list_head list;
    void* data;
};

os_ipc_sender_instance_t* os_ipc_create_sender(char const* name);
os_err_t os_ipc_send_evt(os_ipc_sender_instance_t* sender, uint32_t evt);
os_err_t os_ipc_send_evt_data(os_ipc_sender_instance_t* sender, uint32_t evt, void* data, uint32_t len);

typedef void (*os_ipc_receiver_handler_t)(os_ipc_receiver_instance_t* inst, os_ipc_msg_t* msg);
typedef os_err_t (*os_ipc_init_cb_t)(os_ipc_receiver_instance_t* inst);

struct os_ipc_receiver_instance {
    os_thread_t thread;
    os_queue_t queue;
    os_ipc_init_cb_t init_cb;
    os_ipc_receiver_handler_t handler;
    void* data;
};

os_ipc_receiver_instance_t* os_ipc_create_receiver(
    const char* name,
    uint32_t buffer_cnt,
    os_priority_t prio,
    os_stack_size_t stack_size,
    void* ctx,
    os_ipc_init_cb_t init_cb,
    os_ipc_receiver_handler_t handler
);

os_ipc_receiver_instance_t* os_ipc_create_receiver_async(
    uint32_t buffer_cnt,
    void* ctx,
    os_ipc_receiver_handler_t handler
);

os_err_t os_ipc_try_recv(os_ipc_receiver_instance_t* recv, uint32_t first_wait_ms);
os_err_t os_ipc_register_event(
    os_ipc_sender_instance_t* sender,
    os_ipc_receiver_instance_t* receiver,
    uint32_t event,
    os_ipc_msg_prio_t prio
);

os_err_t os_ipc_unregister_event(
    os_ipc_sender_instance_t* sender,
    os_ipc_receiver_instance_t* receiver,
    uint32_t event);

os_err_t os_ipc_sender_destroy(os_ipc_sender_instance_t** pinst);
