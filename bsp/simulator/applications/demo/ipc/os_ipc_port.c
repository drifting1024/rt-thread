#include "os_ipc_port.h"

// queue
os_queue_t os_queue_create(const char* name, const uint32_t item_size, const uint32_t item_max_cnt)
{
    rt_mq_t q = rt_mq_create(name, item_size, item_max_cnt, RT_IPC_FLAG_FIFO);
    return (os_queue_t)q;
}

os_err_t os_queue_recv(const os_queue_t queue, void* p_data)
{
    os_err_t ret = RT_EOK;
    rt_mq_t q = (rt_mq_t)queue;
    ret = rt_mq_recv(q, p_data, q->msg_size, RT_WAITING_FOREVER);

    return ret;
}

os_err_t os_queue_recv_timeout_ms(const os_queue_t queue, void* p_data, uint32_t ms)
{
    uint32_t tick = os_tick_from_ms(ms);
    os_err_t ret = RT_EOK;
    rt_mq_t q = (rt_mq_t)queue;
    ret = rt_mq_recv(q, p_data, q->msg_size, tick);
    return ret;
}

os_err_t os_queue_delete(const os_queue_t queue)
{
    os_err_t ret = RT_EOK;
    ret = rt_mq_delete(queue);
    return ret;
}

os_err_t os_queue_send(const os_queue_t queue, const void* pdata)
{
    os_err_t ret = RT_EOK;
    rt_mq_t q = (rt_mq_t)queue;
    ret = rt_mq_send(q, (void*)pdata, q->msg_size);
    return ret;
}

// mutex
os_mutex_t os_mutex_create(const char* name)
{
    os_mutex_t mutex = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    return mutex;
}

os_err_t os_mutex_take(const os_mutex_t mutex)
{
    os_err_t ret = 0;
    rt_mutex_take(mutex, RT_WAITING_FOREVER);
    return ret;
}

os_err_t os_mutex_release(const os_mutex_t mutex)
{
    os_err_t ret = 0;
    rt_mutex_release(mutex);
    return ret;
}

os_err_t os_mutex_delete(const os_mutex_t mutex)
{
    os_err_t ret = 0;
    rt_mutex_delete(mutex);
    return ret;
}

// thread
os_thread_t os_thread_create(
    const char* name, 
    const os_thread_entry_t entry, 
    const void* para,
    const os_stack_size_t stack_size,
    const os_priority_t prio)
{
    rt_thread_t thread = NULL;
    rt_kprintf("create thread %s\n", name);
    thread = rt_thread_create(name, entry, para, stack_size, prio, 10);
    if (thread != NULL) {
        rt_thread_startup(thread);
    }

    return thread;
}

os_err_t os_thread_delete(const os_thread_t thread)
{
    rt_thread_t t = (rt_thread_t)thread;
    rt_err_t rt_ret = rt_thread_delete(t);
    return (os_err_t)rt_ret;
}

// isr
bool os_is_inside_isr(void)
{
    // __get_IPSR()
    return false;
}

uint32_t os_irq_disable(void)
{
    return rt_hw_interrupt_disable();
}

void os_irq_enable(uint32_t irq)
{
    rt_hw_interrupt_enable(irq);
}

// delay
os_err_t os_delay_ms(uint32_t ms)
{
    return rt_thread_mdelay(ms);
}

uint32_t os_tick_from_ms(uint32_t ms)
{
    if (ms == 0) {
        return 0;
    } else {
        return rt_tick_from_millisecond(ms);
    }
}
