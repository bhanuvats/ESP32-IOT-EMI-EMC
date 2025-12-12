#pragma once

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"

#include "driver/i2c_master.h"

#include "main.h"

void i2c_main(void);