#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void videocapture_sokol_framebuffer_setup(void);
void videocapture_sokol_framebuffer_shutdown(void);
bool videocapture_sokol_framebuffer_update(const void* pixels, size_t size, int width, int height);
void videocapture_sokol_framebuffer_render(void);

#ifdef __cplusplus
}
#endif
