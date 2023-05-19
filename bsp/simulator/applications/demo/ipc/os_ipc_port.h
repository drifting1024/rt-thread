#pragma once
#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

typedef rt_err_t os_err_t;       /**< Type for error number */
typedef void* os_queue_t;
typedef void* os_mutex_t;
typedef void* os_thread_t;
typedef uint32_t os_priority_t;      // 1->highest prio,  31->lowest prio
typedef uint32_t os_stack_size_t;    // size with bytes
typedef void (*os_thread_entry_t)(void* ctx);

os_queue_t os_queue_create(const char* name, const uint32_t item_size, const uint32_t item_max_cnt);
os_err_t os_queue_send(const os_queue_t queue, const void* pdata);
os_err_t os_queue_recv(const os_queue_t queue, void* p_data);
os_err_t os_queue_recv_timeout_ms(const os_queue_t queue, void* p_data, uint32_t ms);
os_err_t os_queue_delete(const os_queue_t queue);

os_mutex_t os_mutex_create(const char* name);
os_err_t os_mutex_take(const os_mutex_t mutex);
os_err_t os_mutex_release(const os_mutex_t mutex);
os_err_t os_mutex_release(const os_mutex_t mutex);

os_thread_t os_thread_create(
    const char* name, 
    const os_thread_entry_t entry, 
    const void* para,
    const os_stack_size_t stack_size,
    const os_priority_t prio);
os_err_t os_thread_delete(const os_thread_t thread);

bool os_is_inside_isr(void);
uint32_t os_irq_disable(void);
void os_irq_enable(uint32_t irq);

os_err_t os_delay_ms(uint32_t ms);
uint32_t os_tick_from_ms(uint32_t ms);
