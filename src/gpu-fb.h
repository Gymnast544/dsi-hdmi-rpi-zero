
#define __GPU_FB_H__
#include "rpi.h"

typedef struct gpu_fb {
    uint32_t x_offset; uint32_t y_offset;
    uint32_t height;
    uint32_t width;
    uint32_t virtual_height; //2x height for double buffering
    uint32_t virtual_width;
    uint32_t depth; //bits per pixel
    uint32_t pitch; //bytes per row

    void *framebuffer_addr;
    uint32_t framebuffer_size;
    uint32_t back_y; //y-row of back (write) buffer
} gpu_fb_t;

gpu_fb_t *gpu_fb_init(uint32_t width, uint32_t height, uint32_t depth);
void gpu_fb_flip(gpu_fb_t *gpu);
