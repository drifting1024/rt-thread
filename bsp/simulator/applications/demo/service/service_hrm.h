#pragma once

#include <stdio.h>
#include "os_ipc.h"

typedef enum {
    SERVICE_HRM_EVT_START = 0,
    SERVICE_HRM_EVT_VALUE_CHANGE,
} service_hrm_evt_t;

typedef struct {
    uint8_t value;
    bool onoff;
} hrm_cur_status_t;

os_err_t service_hrm_init(void);
os_ipc_sender_instance_t* service_hrm_get_sender(void);

