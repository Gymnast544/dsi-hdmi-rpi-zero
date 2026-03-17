// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"

// in libpi/include: has useful enums.
#include "rpi-interrupts.h"

//page 90
#define GPREN0 0x2020004C 
#define GPFEN0 0x20200058
#define GPEDS0 0x20200040
#define IRQPENDING2 0x2000B208
#define IRQENABLE2 0x2000B214



// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).

//page 117
//left shift by number of interrupt register
//register 49 in our case
int gpio_has_interrupt(void) {
    dev_barrier();
    //todo("implement: is there a GPIO_INT0 interrupt?\n");
    unsigned read = GET32(IRQPENDING2);
    dev_barrier();
    return (read & (0b1<<(49%32))) !=0;
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
//
// also have to enable GPIO interrupts at all in <IRQ_Enable_2>
void gpio_int_rising_edge(unsigned pin) {
    if(pin>=32){
        panic("pin is larger than 32");
        return;
    }
    dev_barrier();
    OR32(GPREN0, 0b1<<pin);
    dev_barrier();
    PUT32(IRQENABLE2, 0b1<<(49%32));
    dev_barrier();
    //todo("implement: detect rising edge\n");
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
//
// also have to enable GPIO interrupts at all in <IRQ_Enable_2>
void gpio_int_falling_edge(unsigned pin) {
    if(pin>=32){
        panic("pin is larger than 32");
        return;
    }
    dev_barrier();
    OR32(GPFEN0, 0b1<<pin);
    dev_barrier();
    PUT32(IRQENABLE2, 0b1<<(49%32));
    dev_barrier();
    //todo("implement: detect falling edge\n");
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if(pin>=32){
        panic("pin is larger than 32");
        return 0;
    }
    dev_barrier();
    unsigned read = GET32(GPEDS0);
    dev_barrier();
    return (read & (0b1<<pin))!=0;
    //todo("implement: is an event detected?\n");
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if(pin>=32){
        panic("pin is larger than 32");
        return;
    }
    dev_barrier();
    PUT32(GPEDS0, 0b1<<pin);
    dev_barrier();
    //todo("implement: clear event on <pin>\n");
}
