#define SOKOL_IMPL
#define SOKOL_NO_ENTRY

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "util/sokol_framebuffer.h"

#include "SokolFramebuffer.h"

static sfb_framebuffer videocapture_framebuffer;

void videocapture_sokol_framebuffer_setup(void) {
    sfb_desc description = {0};
    description.logger.func = slog_func;
    sfb_setup(&description);
}

void videocapture_sokol_framebuffer_shutdown(void) {
    if (videocapture_framebuffer.id != 0U) {
        sfb_destroy_framebuffer(videocapture_framebuffer);
        videocapture_framebuffer.id = 0U;
    }
    sfb_shutdown();
}

bool videocapture_sokol_framebuffer_update(const void* pixels, size_t size, int width, int height) {
    if (videocapture_framebuffer.id == 0U) {
        sfb_framebuffer_desc description = {0};
        description.width = width;
        description.height = height;
        videocapture_framebuffer = sfb_make_framebuffer(&description);
    } else {
        sfb_resize_desc description = {0};
        description.width = width;
        description.height = height;
        sfb_resize(videocapture_framebuffer, &description);
    }

    if (sfb_query_framebuffer_state(videocapture_framebuffer) != SFB_RESOURCESTATE_VALID) {
        return false;
    }

    sfb_update_desc update = {0};
    update.pixels.ptr = pixels;
    update.pixels.size = size;
    sfb_update(videocapture_framebuffer, &update);
    return true;
}

void videocapture_sokol_framebuffer_render(void) {
    if (videocapture_framebuffer.id != 0U &&
        sfb_query_framebuffer_state(videocapture_framebuffer) == SFB_RESOURCESTATE_VALID) {
        sfb_render(videocapture_framebuffer);
    }
}
