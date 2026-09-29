/* SPDX-License-Identifier: MIT */
/* Receive ring shared by the serial drivers: filled by the UART interrupt
 * handler, drained by readers. One producer (the ISR) and one consumer; the
 * consumer masks interrupts around rx_ring_pop() when it must also sleep. */
#ifndef HOMECORE_DRIVERS_RX_RING_H
#define HOMECORE_DRIVERS_RX_RING_H

#include <stdbool.h>
#include <stdint.h>

/* Power of two, so indexes wrap with a mask. */
#define RX_RING_SIZE 64U

typedef struct {
    volatile uint8_t data[RX_RING_SIZE];
    volatile uint32_t head;    /* Next write position (ISR only) */
    volatile uint32_t tail;    /* Next read position (reader only) */
    volatile uint32_t dropped; /* Bytes lost because the ring was full */
} rx_ring_t;

/* ISR side: store a byte, or count it as dropped when the ring is full. */
static inline void rx_ring_push(rx_ring_t *ring, uint8_t byte) {
    uint32_t head = ring->head;
    if (head - ring->tail == RX_RING_SIZE) {
        ring->dropped++;
        return;
    }
    ring->data[head & (RX_RING_SIZE - 1U)] = byte;
    ring->head = head + 1U;
}

static inline bool rx_ring_empty(const rx_ring_t *ring) {
    return ring->head == ring->tail;
}

/* Reader side: returns the oldest byte (0 to 255), or -1 when empty. */
static inline int rx_ring_pop(rx_ring_t *ring) {
    uint32_t tail = ring->tail;
    if (ring->head == tail) {
        return -1;
    }
    uint8_t byte = ring->data[tail & (RX_RING_SIZE - 1U)];
    ring->tail = tail + 1U;
    return byte;
}

#endif /* HOMECORE_DRIVERS_RX_RING_H */
