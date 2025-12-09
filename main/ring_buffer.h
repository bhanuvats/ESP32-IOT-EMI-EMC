#ifndef MAIN_RING_BUFFER_H_
#define MAIN_RING_BUFFER_H_


#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h"

typedef struct {
    void *buffer;           // raw memory for packets
    size_t element_size;    // sizeof(sensor_packet_t)
    size_t capacity;        // number of elements
    size_t head;            // write index
    size_t tail;            // read index
    size_t count;           // items in buffer
    SemaphoreHandle_t lock; // mutex for thread safety
} ring_buffer_t;

bool ring_buffer_init(ring_buffer_t *rb, size_t capacity, size_t element_size);
void ring_buffer_push(ring_buffer_t *rb, const void *element);
bool ring_buffer_pop(ring_buffer_t *rb, void *out_element);
bool ring_buffer_is_empty(ring_buffer_t *rb);

#define RING_BUFFER_SIZE 128   // number of packets

extern ring_buffer_t g_modbus_tcp_ring;
extern ring_buffer_t g_tx_ring;

#endif /* MAIN_RING_BUFFER_H_ */
