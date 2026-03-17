//reused from lab

#include "rpi.h"
#include "mbox.h"

uint32_t rpi_clock_curhz_get(uint32_t clock) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 8*4;
    msg[1] = 0;
    msg[2] = 0x00030002;
    msg[3] = 8;
    msg[4] = 0;
    msg[5] = clock;
    msg[6] = 0;
    msg[7] = 0;

    mbox_send(MBOX_CH, msg);
    if (msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}


uint32_t rpi_clock_maxhz_get(uint32_t clock) {
    volatile uint32_t msg[8] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 8*4;
    msg[1] = 0;
    msg[2] = 0x00030004;
    msg[3] = 8;
    msg[4] = 0;
    msg[5] = clock;
    msg[6] = 0;
    msg[7] = 0;

    mbox_send(MBOX_CH, msg);
    if (msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}

uint32_t rpi_clock_hz_set(uint32_t clock, uint32_t hz) {
    volatile uint32_t msg[12] __attribute__((aligned(16)));
    assert((unsigned)msg % 16 == 0);

    msg[0] = 12*4;
    msg[1] = 0;
    msg[2] = 0x00038002;
    msg[3] = 12;
    msg[4] = 0;
    msg[5] = clock;
    msg[6] = hz;
    msg[7] = 0;
    msg[8] = 0;
    msg[9] = 0;
    msg[10] = 0;
    msg[11] = 0;

    mbox_send(MBOX_CH, msg);
    if (msg[1] != 0x80000000)
        panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1 << 31) | 8));
    return msg[6];
}
