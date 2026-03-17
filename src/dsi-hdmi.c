//dsi screen to hdmi main code
#include "rpi.h"
#include "gpu-fb.h"
#include "vm-cache.h"
#include "mbox.h"

#define LOG(...) printk(__VA_ARGS__)
#define MAX_FRAMES 200//for testing/dev to auto restart


#define GPLEV0 0x20200034u
#define GPLEV0_BUS 0x7E200034u
#define DMA_BASE 0x20007000u
#define DMA_CH0 (DMA_BASE + 0u * 0x100u)
#define DMA_CS_REG 0x00u
#define DMA_CBAD_REG 0x04u
#define DMA_CS_ACTIVE (1u << 0)
#define DMA_CS_DISDEBUG (1u << 29)
#define PHYS_TO_BUS(x) ((uint32_t)(x) | 0x40000000u)

typedef struct {
    uint32_t ti, src, dst, len, stride, next, pad[2];
} __attribute__((aligned(32))) fast_cb_t;

#define DMA_TI_NO_WIDE_BURSTS (1 << 26)
#define DMA_TI_TDMODE (1 <<  1)
#define DMA_TI_DEST_INC (1 <<  4)

#define FAST_N_CB 14u
#define FAST_YLENGTH 65535//num samples
#define FAST_N_TOTAL (FAST_N_CB * FAST_YLENGTH)

static fast_cb_t fast_cbs[FAST_N_CB] __attribute__((aligned(32)));
static uint32_t fast_sbuf[FAST_N_TOTAL] __attribute__((aligned(32)));

static uint8_t *fb_base;
static uint32_t fb_pitch, fb_x_off, fb_y_base;


static void dma_wr(uint32_t r, uint32_t v) { PUT32(DMA_CH0 + r, v); }
static uint32_t dma_rd(uint32_t r) { return GET32(DMA_CH0 + r); }

//DSi signal pins
#define PIN_DCLK 4
#define PIN_GSP 5
#define PIN_GCK 6

//active pixel window
#define ACTIVE_PIXEL_START 28
#define ACTIVE_PIXEL_END 284
#define DSI_W 256
#define DSI_H 192

//scan line decoder states
enum { SCAN_WAIT_GCK = 2, SCAN_ACTIVE = 3 };

#define GCK_BIT (1 << PIN_GCK)
#define DCLK_BIT (1 << PIN_DCLK)
#define EDGE_MASK (GCK_BIT | DCLK_BIT)
#define SCALE 2//scale factor for HDMI

static void dma_init(void) {
    dev_barrier();
    PUT32(DMA_BASE +0xFF0u,GET32(DMA_BASE + 0xFF0u) | 1);//DMA_ENABLE: enable ch0
    dev_barrier();
    dma_wr(DMA_CS_REG, 1u << 31); //resetdma 
    delay_us(10);//small wait for reset
    dma_wr(DMA_CS_REG, 0);//clear cs (deassert reset)
    dma_wr(0x20u, 0x7u); //clear dma flags
    delay_us(10);
    for (unsigned i = 0; i< FAST_N_CB; i++) {//build the 14 CB chain
        //32 bit transfers, 2d mode, auto increment destination address
        fast_cbs[i].ti=DMA_TI_NO_WIDE_BURSTS|DMA_TI_TDMODE|DMA_TI_DEST_INC;
        fast_cbs[i].src=GPLEV0_BUS;//source is gpio
        fast_cbs[i].dst=PHYS_TO_BUS((uint32_t)(fast_sbuf + i * FAST_YLENGTH));//destination is cbs
        fast_cbs[i].len=((uint32_t)FAST_YLENGTH << 16) | 4;//length is 65535 samples, 4 bytes per sample
        fast_cbs[i].stride= 0;//no stride (skip between rows)
        fast_cbs[i].next= (i + 1u < FAST_N_CB)? PHYS_TO_BUS((uint32_t)&fast_cbs[i + 1]): 0;
        fast_cbs[i].pad[0] = fast_cbs[i].pad[1] = 0;//pad to 32byte alignment
    }
    dev_barrier();
}

static void dma_fire(void) {//start dma capture
    dma_wr(DMA_CBAD_REG, PHYS_TO_BUS((uint32_t)&fast_cbs[0]));
    dma_wr(DMA_CS_REG, DMA_CS_ACTIVE | DMA_CS_DISDEBUG | (15 <<20) | (15<< 16)); //max priority
}

static void dma_wait(void) {//blocks until capture finished
    while(dma_rd(DMA_CS_REG) & DMA_CS_ACTIVE);//wait
    dev_barrier();
}

static uint32_t gsp_sync(void) {//wait for GSP signal to fall and rise (start of frame)
    uint32_t prev =GET32(GPLEV0);
    uint32_t t0=timer_get_usec();
    int got_fall =0;
    while (timer_get_usec() - t0 < 200000) {
        uint32_t cur = GET32(GPLEV0);
        if (!got_fall) {
            if ((prev >>PIN_GSP & 1u) && !(cur >> PIN_GSP & 1)) got_fall = 1;
        } else if (!(prev >> PIN_GSP & 1u) && (cur >> PIN_GSP & 1u)) {
            return timer_get_usec();
        }
        prev = cur;
    }
    return 0;
}

static unsigned process_buf(void) {//convert captured data to RGB and write to framebuffer
    dcache_inv_region(fast_sbuf, sizeof fast_sbuf);
    unsigned state = SCAN_WAIT_GCK, ln = 0, px = 0;
    //state machine to scan for active pixels and detect line changes
    unsigned cb_counter =1;
    int line_dirty = 0;
    uint32_t s_prev = fast_sbuf[0];
    uint32_t *fb_line = (uint32_t *)(fb_base+fb_y_base* fb_pitch+fb_x_off*4);
    uint32_t *fb_prev_line = fb_line;
    for (int i = 1; i<FAST_N_TOTAL&&ln<DSI_H;i++,cb_counter++) {
        if (cb_counter>=FAST_YLENGTH) {//end of CB, check for line change
            cb_counter = 0;
            if (state==SCAN_ACTIVE) line_dirty = 1;//line changed
            s_prev = fast_sbuf[i];
            continue;
        }
        uint32_t cur= fast_sbuf[i];//current sample
        uint32_t changed = cur ^ s_prev;
        s_prev = cur;
        if (!(changed & EDGE_MASK)) continue;
        unsigned gck_rise  = (changed & GCK_BIT) && (cur & GCK_BIT);
        unsigned dclk_edge = changed & DCLK_BIT;

        if (state==SCAN_WAIT_GCK) {//waiting for GCK rise
            if (gck_rise) {
                px = 0;
                state = SCAN_ACTIVE;
                line_dirty = 0;
            }
        } else {//scanning active pixels
            if (dclk_edge && !line_dirty) {
                if (px >= ACTIVE_PIXEL_START && px < ACTIVE_PIXEL_END) {
                    uint32_t r =((cur >>7) &0x3Fu)<<2; //GPIO[12:7] red 6-bit
                    uint32_t g =((cur >> 16) &0x3Fu)<<2; //GPIO[21:16] green 6-bit
                    uint32_t b =((cur >> 22) &0x3Fu)<<2; //GPIO[27:22] blue 6-bit
                    fb_line[px - ACTIVE_PIXEL_START] = b | (g << 8) | (r << 16); //32bpp BGRX
                }
                if (px < 350u) px++;
            }
            if (gck_rise) {
                if (line_dirty && ln > 0)//copy previous line if dirty
                    memcpy(fb_line, fb_prev_line, DSI_W * 4);
                fb_prev_line = fb_line;
                ln++; px = 0; line_dirty = 0;
                fb_line = (uint32_t *)((uint8_t *)fb_line + fb_pitch);
            }
        }
    }
    return ln;
}

static void setup_fb_ptrs(gpu_fb_t *gpu) {//set up framebuffer pointers
    fb_base   = (uint8_t *)gpu->framebuffer_addr;
    fb_pitch  = gpu->pitch;
    fb_x_off  = (gpu->width  > DSI_W) ? (gpu->width  - DSI_W) / 2 : 0;
    fb_y_base = gpu->back_y + ((gpu->height > DSI_H) ? (gpu->height - DSI_H) / 2 : 0);
}

void notmain(void) {
    uart_init();//for debuggin
    //overclock the pi to its max speed
    uint32_t cpu_max = rpi_clock_maxhz_get(MBOX_CLK_CPU);
    uint32_t core_max = rpi_clock_maxhz_get(MBOX_CLK_CORE);
    uint32_t sdram_max = rpi_clock_maxhz_get(MBOX_CLK_SDRAM);
    if (cpu_max) rpi_clock_hz_set(MBOX_CLK_CPU, cpu_max);
    if (core_max) rpi_clock_hz_set(MBOX_CLK_CORE, core_max);
    if (sdram_max) rpi_clock_hz_set(MBOX_CLK_SDRAM, sdram_max);

    vm_cache_setup();//set up L1 cache

    for (int pin = 4; pin <= 12; pin++) gpio_set_input(pin);//set up GPIOs
    for (int pin = 16; pin <= 27; pin++) gpio_set_input(pin);

    dma_init();//set up DMA

    gpu_fb_t *gpu = gpu_fb_init(320, 240, 32);
    //gpu_fb_t *gpu =gpu_fb_init(1280, 720, 32);//720p for demo only
    if (!gpu) panic("HDMI failed\n");
    memset(gpu->framebuffer_addr, 0, gpu->framebuffer_size);

    //calibrate period between frames
    uint32_t t1 =gsp_sync();
    if (!t1) { printk("No GSP\n"); clean_reboot(); }
    uint32_t t2 =gsp_sync();
    if (!t2) { printk("No GSP\n"); clean_reboot(); }
    uint32_t frame_period = t2 - t1;

    uint32_t next_gsp = t2;
    uint32_t frames = 0;
#ifdef SD_BOOT
    while (1) {//main loop
#else
    while (frames < MAX_FRAMES) {//only do 200 frames
#endif
        //phase-locked GSP: sleep then brief recal poll
        uint32_t now = timer_get_usec();
        while ((int32_t)(next_gsp - now) < 0)
            next_gsp += frame_period;
        int32_t wait = (int32_t)(next_gsp - 2000) - (int32_t)now;
        if (wait > 0 && wait < 200000)
            delay_us(wait);
        uint32_t t_edge = gsp_sync();
        if (t_edge)
            next_gsp = t_edge + frame_period;
        else
            next_gsp += frame_period;

        uint32_t ta = timer_get_usec();




        //capture
        dma_fire();
        dma_wait();
        setup_fb_ptrs(gpu);
        unsigned lines = process_buf();
        uint32_t tb = timer_get_usec();
        if (lines > 0) {
            asm volatile("mcr p15, 0, %0, c7, c10, 0" :: "r"(0));//clean D-cache
            asm volatile("mcr p15, 0, %0, c7, c10, 4" :: "r"(0));//DSB
            gpu_fb_flip(gpu);//flip the framebuffer
            dev_barrier();
        }
        uint32_t tc = timer_get_usec();
    }
}
