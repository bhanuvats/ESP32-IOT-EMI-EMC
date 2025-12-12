#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdint.h>
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "soc/uart_struct.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_err.h"

#include "hal/gpio_types.h"
#include <inttypes.h>

#include "i2c_tools.h"
#include "eth.h"
#include "meter_emi_emc.h"
#include "ring_buffer.h"
#include "sd_card.h"
#include "wifi.h"
#include "AT_Command.h"

#include <ctype.h>
#include "freertos/queue.h"

typedef struct {
   int thread_id;
   char data[512];         // data sent by each thread
   float tcp_data;
   float rtu_data;
   uint8_t i2c_address;
} PartialPacket;

typedef struct {
    char full_line[2500];    // combined line
} CombinedPacket;

extern PartialPacket pkt;             // actual definition
extern QueueHandle_t partialQueue;
extern QueueHandle_t fullQueue;
extern uart_port_t uart_ports[];
extern SemaphoreHandle_t myMutex;
extern bool scan_start;
extern const char *TAG;