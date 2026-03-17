#include "gpu-fb.h"
#include "rpi.h"
#include "mbox.h"

gpu_fb_t *gpu_fb_init(uint32_t width, uint32_t height, uint32_t depth) {
    static gpu_fb_t gpu_fb;
    printk("GPU: Initializing framebuffer");
    static uint32_t  _mailbox_buf[64]  __attribute__((aligned(16)));
    volatile uint32_t  *mailbox =_mailbox_buf;
    memset((void*)mailbox,0,256);
    unsigned idx = 0;
    mailbox[idx++] = 256;//buf size
    mailbox[idx++] = 0x00000000;//request
    //set physical size
    mailbox[idx++] = 0x00048003; mailbox[idx++] = 8;
    mailbox[idx++] = 0; mailbox[idx++] = width;
    mailbox[idx++] = height;
    //set virtual size
    mailbox[idx++] = 0x00048004; mailbox[idx++] = 8;
    mailbox[idx++] = 0; mailbox[idx++] = width;
    mailbox[idx++] = height * 2;
    //set depth
    mailbox[idx++] = 0x00048005; mailbox[idx++] = 4;
    mailbox[idx++] = 0; mailbox[idx++] = depth;
    //set pixel order RGB
    mailbox[idx++] = 0x00048006; mailbox[idx++] = 4;
    mailbox[idx++] = 0; mailbox[idx++] = 1; //1=RGB
    //alloc framebuffer
    mailbox[idx++] = 0x00040001;
    mailbox[idx++] = 8;
    mailbox[idx++] = 0;
    mailbox[idx++] = 16;
    mailbox[idx++] = 0;
    //get pitch
    mailbox[idx++] = 0x00040008;
    mailbox[idx++] = 4;
    mailbox[idx++] = 0;
    mailbox[idx++] = 0;
    mailbox[idx++] = 0x00000000; //end tag
    mbox_send(MBOX_CH, (void*)mailbox);
    if (mailbox[1] != 0x80000000) {
        printk("gpu mailbox request failed"); return NULL;
    }
    uint32_t fb_addr = mailbox[23] & 0x3FFFFFFF; //strip GPU bus addr bits
    uint32_t fb_size = mailbox[24];
    uint32_t pitch   = mailbox[28];
    if (fb_addr == 0 || fb_size == 0) {
        printk("gailed to allocate framebuffer");
        return NULL;
    }
    //set struct
    gpu_fb.width = width; gpu_fb.height = height; gpu_fb.virtual_width =width;
    gpu_fb.virtual_height = height * 2; gpu_fb.pitch = pitch; gpu_fb.depth= depth;
    gpu_fb.x_offset = 0; gpu_fb.y_offset = 0;
    gpu_fb.framebuffer_addr = (void*)fb_addr;
    gpu_fb.framebuffer_size = fb_size;
    gpu_fb.back_y = height;//back buffer

    memset((void*)fb_addr, 0, fb_size);
    return &gpu_fb;
}

//flip display to back_y, then swap
void gpu_fb_flip(gpu_fb_t  *gpu) {
    //set virtual offset
    static uint32_t flip_buf[16] __attribute__((aligned(16)));
    volatile uint32_t *m = flip_buf;
    m[0] = 64; m[1] = 0; m[2] = 0x00048009; m[3] = 8;
    m[4] = 0;
    m[5] = 0;//x offset
    m[6] = gpu->back_y;//y_offset
    m[7] = 0;//end
    gcc_mb();
    mbox_write(MBOX_CH, (void*)flip_buf);
    mbox_read(MBOX_CH);
    gcc_mb();
    gpu->back_y =(gpu->back_y ==0) ? gpu->height:0;
}
