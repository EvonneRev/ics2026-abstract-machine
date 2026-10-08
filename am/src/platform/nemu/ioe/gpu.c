#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t wh = inl(VGACTL_ADDR);
  int w = wh >> 16;
  int h = wh & 0xffff;
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = w*h*sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
   int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t *pixels = ctl->pixels;
  
  AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
  
  for (int i = 0; i < h; i++) {
    uintptr_t addr = FB_ADDR + ((y + i) * cfg.width + x) * sizeof(uint32_t);
    for (int j = 0; j < w; j++) {
      *(volatile uint32_t *)(addr + j * 4) = pixels[i * w + j];
    }
  }
  
  if (ctl->sync) {
    *(volatile uint32_t *)SYNC_ADDR = 1;
  }
  
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
