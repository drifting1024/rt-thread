#include "os_ipc.h"

#define NODE_CNT 1024

typedef struct {
    os_ipc_sender_register_t buffer[NODE_CNT];
    uint32_t cnt;
    os_mutex_t alloc_lock;
    os_queue_t queue_req_from_isr;
    os_thread_t thread_isr_dispatcher;
} os_ipc_register_entry_buffer_ctx_t;

typedef struct {
    os_ipc_sender_instance_t* sender;
    uint32_t event;
} os_ipc_send_evt_dispatcher_t;

typedef struct os_ipc_msg_internal os_ipc_msg_internal_t;
struct os_ipc_msg_internal {
    os_ipc_receiver_instance_t* to;
    os_ipc_msg_prio_t prio;
    uint32_t ref_cnt;
    os_ipc_msg_t value;
};

static os_ipc_register_entry_buffer_ctx_t* __p_list_buffer_ctx_v = NULL;


static void os_ipc_register_event_trigger_lock(os_ipc_sender_instance_t* sender)
{
    os_mutex_take(sender->lock);
}

static void os_ipc_register_event_trigger_unlock(os_ipc_sender_instance_t* sender)
{
    os_mutex_release(sender->lock);
}

static void os_ipc_handler_ref_increase(os_ipc_msg_internal_t* msg)
{
    uint32_t mask = os_irq_disable();
    msg->ref_cnt++;
    os_irq_enable(mask);
}

static void os_ipc_handler_ref_decrease(os_ipc_msg_internal_t* msg)
{
    bool free = false;
    uint32_t mask = os_irq_disable();
    msg->ref_cnt--;
    if (0 == msg->ref_cnt) {
        free = true;
    }
    os_irq_enable(mask);
    if (free) {
        rt_free(msg);
    }
}

static int os_ipc_sender_id_create(void)
{
    static int __last_id = 1;
    int id;
    uint32_t mask = os_irq_disable();
    __last_id++;
    id = __last_id;
    os_irq_enable(mask);
    return id;
}

static os_ipc_sender_register_t* os_ipc_register_node_new(uint32_t event, os_ipc_receiver_instance_t* receiver)
{
    os_ipc_sender_register_t* node = NULL;
    os_ipc_register_entry_buffer_ctx_t* p_ctx = __p_list_buffer_ctx_v;
    os_mutex_take(p_ctx->alloc_lock);
    for (int i = 0; i < p_ctx->cnt; i++) {
        if (p_ctx->buffer[i].receiver == NULL) {
            node = &p_ctx->buffer[i];
            node->event = event;
            node->receiver = receiver;
            break;
        }
    }
    os_mutex_release(p_ctx->alloc_lock);
    return node;
}

static void os_ipc_register_node_free(os_ipc_sender_register_t* node)
{
    os_ipc_register_entry_buffer_ctx_t* p_ctx = __p_list_buffer_ctx_v;
    os_mutex_take(p_ctx->alloc_lock);
    node->receiver == NULL;
    os_mutex_release(p_ctx->alloc_lock);

}

static void os_ipc_register_list_init(os_ipc_sender_instance_t* sender)
{
    INIT_LIST_HEAD(&sender->list);
}

static void os_ipc_list_append(os_ipc_sender_instance_t* sender, os_ipc_sender_register_t* item)
{
    uint32_t mask = os_irq_disable();
    list_add_tail(&item->list, &sender->list);
    os_irq_enable(mask);
}

static void os_ipc_list_insert(os_ipc_sender_instance_t* sender, os_ipc_sender_register_t* item)
{
    uint32_t mask = os_irq_disable();
    list_add(&item->list, &sender->list);
    os_irq_enable(mask);
}

static void os_ipc_list_remove(os_ipc_sender_instance_t* sender, uint32_t event, os_ipc_receiver_instance_t* receiver)
{
    os_ipc_sender_register_t* node_to_free = NULL;
    uint32_t mask = os_irq_disable();
    os_list_foreach(&sender->list) {
        os_ipc_sender_register_t* node = os_list_entry(os_ipc_sender_register_t, list);
        if ((node->receiver == receiver 
        && (node->event == node->event))) {
            node_to_free = node;
            break;
        }
    }
    os_irq_enable(mask);
    if (node_to_free) {
        list_del(&node_to_free->list);
        os_ipc_register_node_free(node_to_free);
    }
}

os_ipc_sender_instance_t* os_ipc_create_sender(const char* name)
{
    os_ipc_sender_instance_t* sender = rt_malloc(sizeof(os_ipc_sender_instance_t));
    char name_buffer[20];
    if (sender == NULL) {
        return NULL;
    }
    rt_snprintf(name_buffer, sizeof(name_buffer), "ipc_%s", name);
    sender->lock = os_mutex_create(name_buffer);
    sender->id = os_ipc_sender_id_create();
    os_ipc_register_list_init(sender);

    return sender;
}

os_err_t os_ipc_sender_destroy(os_ipc_sender_instance_t** p_inst)
{
    os_ipc_sender_instance_t* inst = *p_inst;
    if (inst->lock) {
        os_mutex_delete(inst->lock);
    }

    rt_free(inst);
    *p_inst = NULL;
    return RT_EOK;
}

os_err_t os_ipc_send_evt_data(os_ipc_sender_instance_t* sender, uint32_t event, void* data, uint32_t len)
{
    os_err_t ret = RT_EOK;
    os_ipc_msg_internal_t* msg = NULL;
    os_ipc_register_event_trigger_lock(sender);
    os_list_foreach(&sender->list) {
        os_ipc_sender_register_t* node = os_list_entry(os_ipc_sender_register_t, list);
        if (node->event == event) {
            if (msg == NULL) {
                msg = rt_malloc(sizeof(os_ipc_msg_internal_t) + len);
                if (msg == NULL) {
                    ret = RT_ENOMEM;
                    break;
                };
                msg->value.from = sender;
                msg->ref_cnt = 1;
                msg->value.event = event;
                msg->value.len = len;

                if (len > 0) {
                    rt_memcpy(&msg->value.data[0], data, len);
                }
            }
            os_ipc_handler_ref_increase(msg);
            msg->prio = node->prio;
            msg->to = node->receiver;
            ret = os_ipc_send_msg(sender, msg);
            if (ret != RT_EOK) {
                os_ipc_handler_ref_decrease(msg);
                break;
            }


        }
    }
    os_ipc_register_event_trigger_unlock(sender);
    if (msg == NULL) {

    } else {
        os_ipc_handler_ref_decrease(msg);
    }

    return ret;
}

os_err_t os_ipc_send_msg(os_ipc_sender_instance_t* sender, os_ipc_msg_internal_t* msg)
{
    os_err_t ret = RT_EOK;
    int retry_times = 4;
    do {
        ret = os_queue_send(msg->to->queue, &msg);
        if (ret == RT_EOK) {
            break;
        } else if (ret == -RT_EFULL) {
            if (retry_times-- > 0) {
                os_delay_ms(100);
            } else {
                break;
            }
        } else {
            break;
        }
    } while (1);

    return ret;
}


os_err_t os_ipc_send_evt(os_ipc_sender_instance_t* sender, uint32_t event)
{
    if (os_is_inside_isr()) {
        os_ipc_send_evt_dispatcher_t req = {
            .sender = sender,
            .event = event,
        };

        return os_queue_send(__p_list_buffer_ctx_v->queue_req_from_isr, &req);
    } else {
        return os_ipc_send_evt_data(sender, event, NULL, 0);
    }
}

static void os_ipc_isr_dispatcher(void* ctx)
{
    os_queue_t q_from_isr = (os_queue_t)ctx;
    os_ipc_send_evt_dispatcher_t req;
    while (1) {
        if (RT_EOK == os_queue_recv(q_from_isr, &req)) {
            os_ipc_send_evt(req.sender, req.event);
        }
    }
}

static void os_ipc_base_entry(void* ctx)
{
    os_err_t ret = RT_EOK;
    os_ipc_receiver_instance_t* inst = (os_ipc_receiver_instance_t*)ctx;
    os_ipc_msg_internal_t* msg_internal;
    os_ipc_msg_t* msg;

    if (inst->init_cb != NULL) {
        inst->init_cb(inst);
    }

    while (1) {
        msg_internal = NULL;
        ret = os_queue_recv(inst->queue, &msg_internal);
        if (ret == RT_EOK) {
            msg = &msg_internal->value;
            inst->handler(inst, msg);
            os_ipc_handler_ref_decrease(msg_internal);
        } else {

        }
    }
}

os_ipc_receiver_instance_t* os_ipc_create_receiver(
    char const* name,
    uint32_t buffer_cnt,
    os_priority_t prio,
    os_stack_size_t stack_size,
    void* ctx,
    os_ipc_init_cb_t init_cb,
    os_ipc_receiver_handler_t handler)
{
    char name_buff[20];
    os_ipc_receiver_instance_t* inst = rt_malloc(sizeof(os_ipc_receiver_instance_t));
    if (inst == NULL) {
        goto failed;
    }

    inst->init_cb = init_cb;
    inst->handler = handler;
    inst->data = ctx;
    inst->thread = NULL;
    rt_snprintf(name_buff, sizeof(name_buff), "%s_q", name);
    inst->queue = os_queue_create(name_buff, sizeof(os_ipc_msg_internal_t*), buffer_cnt);
    if (inst->queue == NULL) {
        goto failed;
    }
    
    rt_snprintf(name_buff, sizeof(name_buff), "%s_t", name);
    inst->thread = os_thread_create(name_buff, os_ipc_base_entry, inst, stack_size, prio);
    if (inst->thread == NULL) {
        goto failed;
    }

    if (0) {
failed:
    if (inst != NULL) {
        if (inst->queue != NULL) {
            os_queue_delete(inst->queue);
        }
        if (inst->thread != NULL) {
            os_thread_delete(inst->thread);
        }
        rt_free(inst);
        inst = NULL;
    }
    }
    return inst;
}

os_err_t os_ipc_receiver_destroy(os_ipc_receiver_instance_t** p_inst)
{
    os_ipc_receiver_instance_t* inst = *p_inst;
    if (inst->queue) {
        os_queue_delete(inst->queue);
    }

    if (inst->thread != NULL) {
        os_thread_delete(inst->thread);
    }

    rt_free(inst);

    *p_inst = NULL;
    return RT_EOK;
}


os_ipc_receiver_instance_t* os_ipc_create_receiver_async(
    char const* name,
    uint32_t buffer_cnt,
    void* ctx,
    os_ipc_receiver_handler_t handler
)
{
    char name_buff[20];
    os_ipc_receiver_instance_t* inst = rt_malloc(sizeof(os_ipc_receiver_instance_t));
    if (inst == NULL) {
        goto failed;
    }

    inst->init_cb = NULL;
    inst->handler = handler;
    inst->data = ctx;
    inst->thread = NULL;
    rt_snprintf(name_buff, sizeof(name_buff), "%s_q", name);
    inst->queue = os_queue_create(name_buff, sizeof(os_ipc_msg_internal_t*), buffer_cnt);
    if (inst->queue == NULL) {
        goto failed;
    }

failed:
    if (inst != NULL) {
        if (inst->queue != NULL) {
            os_queue_delete(inst->queue);
        }
        if (inst->thread != NULL) {
            os_thread_delete(inst->thread);
        }
        rt_free(inst);
        inst = NULL;
    }

}

os_err_t os_ipc_try_recv(os_ipc_receiver_instance_t* recv, uint32_t first_wait_ms)
{
    os_err_t ret = RT_EOK;
    bool empty = true;
    os_ipc_receiver_instance_t* inst = recv;
    os_ipc_msg_internal_t* msg_internal;
    os_ipc_msg_t* msg;
    uint32_t ms = first_wait_ms;

    while (1) {
        msg_internal = NULL;
        ret = os_queue_recv_timeout_ms(inst->queue, &msg_internal, ms);
        if (ret == RT_EOK) {
            empty = false;
            msg = &msg_internal->value;
            if (inst->handler == NULL) {

            } else {
                inst->handler(inst, msg);
            }
            os_ipc_handler_ref_decrease(msg_internal);
            ms = 0;
        } else if (ret == -RT_ETIMEOUT) {
            if (!empty) {

            }
            break;
        } else {

        }
    }

    return ret;
}


os_err_t os_ipc_register_event(
    os_ipc_sender_instance_t* sender,
    os_ipc_receiver_instance_t* receiver,
    uint32_t event,
    os_ipc_msg_prio_t prio)
{
    if (sender == NULL) {
        return RT_ERROR;
    }
    if (receiver == NULL) {
        return RT_ERROR;
    }

    os_ipc_sender_register_t* entry = os_ipc_register_node_new(event, receiver);
    if (entry == NULL) {
        return RT_ERROR;
    }

    entry->prio = prio;
    os_ipc_register_event_trigger_lock(sender);
    if (prio == OS_IPC_MSG_PRIO_HIGH) {
        os_ipc_list_insert(sender, entry);
    } else {
        os_ipc_list_append(sender, entry);
    }
    os_ipc_register_event_trigger_unlock(sender);

    return RT_EOK;
}

os_err_t os_ipc_unregister_event(
    os_ipc_sender_instance_t* sender, 
    os_ipc_receiver_instance_t* receiver,
    uint32_t event)
{
    if (sender == NULL) {
        return RT_ERROR;
    }
    if (receiver == NULL) {
        return RT_ERROR;
    }

    os_ipc_register_event_trigger_lock(sender);
    os_ipc_list_remove(sender, event, receiver);
    os_ipc_register_event_trigger_unlock(sender);
}

os_err_t os_ipc_init(void)
{
    if (__p_list_buffer_ctx_v == NULL) {
        __p_list_buffer_ctx_v = rt_malloc(sizeof(os_ipc_register_entry_buffer_ctx_t));
        __p_list_buffer_ctx_v->alloc_lock = os_mutex_create("ipc_buffer");
        rt_memset(__p_list_buffer_ctx_v->buffer, 0, sizeof(os_ipc_sender_register_t)*NODE_CNT);
        __p_list_buffer_ctx_v->cnt = NODE_CNT;
        __p_list_buffer_ctx_v->queue_req_from_isr = os_queue_create("ipc_isr_req", sizeof(os_ipc_send_evt_dispatcher_t), 8);
        __p_list_buffer_ctx_v->thread_isr_dispatcher = os_thread_create("ipc_isr", os_ipc_isr_dispatcher, __p_list_buffer_ctx_v->queue_req_from_isr, 512, 1);
    }

    return RT_EOK;
}


