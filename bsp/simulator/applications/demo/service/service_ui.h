#pragma once

#include <stdio.h>
#include "os_ipc.h"

os_err_t service_ui_init(void);
os_ipc_sender_instance_t* service_ui_get_sender(void);
