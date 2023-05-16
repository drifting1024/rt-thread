#include "os_ipc.h"

os_err_t os_queue_send(const os_queue_t queue, const void* pdata)
{
    os_err_t ret = RT_EOK;
    rt_mq_t q = (rt_mq_t)queue;
    ret = rt_mq_send(q, (void*)pdata, q->msg_size);
    return ret;
}


