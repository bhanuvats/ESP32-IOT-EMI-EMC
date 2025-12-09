
#include "ring_buffer.h"
#include <string.h>
#include "esp_heap_caps.h"

bool ring_buffer_init(ring_buffer_t *rb, size_t capacity, size_t element_size)
{
    rb->buffer = heap_caps_malloc(capacity * element_size, MALLOC_CAP_DEFAULT);
    if (!rb->buffer) return false;

    rb->capacity     = capacity;
    rb->element_size = element_size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    rb->lock = xSemaphoreCreateMutex();
    return rb->lock != NULL;
}

// Always accepts — overwrites oldest entry if full
void ring_buffer_push(ring_buffer_t *rb, const void *element)
{
    xSemaphoreTake(rb->lock, portMAX_DELAY);

    void *dest = (uint8_t*)rb->buffer + (rb->head * rb->element_size);
    memcpy(dest, element, rb->element_size);

    rb->head = (rb->head + 1) % rb->capacity;

    if (rb->count < rb->capacity) {
        rb->count++;
    } else {
        // buffer full → overwrite → drop oldest
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    xSemaphoreGive(rb->lock);
}

bool ring_buffer_pop(ring_buffer_t *rb, void *out_element)
{
    bool ok = false;
    xSemaphoreTake(rb->lock, portMAX_DELAY);

    if (rb->count > 0) {
        void *src = (uint8_t*)rb->buffer + (rb->tail * rb->element_size);
        memcpy(out_element, src, rb->element_size);
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count--;
        ok = true;
    }

    xSemaphoreGive(rb->lock);
    return ok;
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    bool empty;
    xSemaphoreTake(rb->lock, portMAX_DELAY);
    empty = (rb->count == 0);
    xSemaphoreGive(rb->lock);
    return empty;
}