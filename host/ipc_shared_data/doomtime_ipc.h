#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

// interface header for chocolate doom

// async, must be called before first frame
void doomtime_ipc_set_palettes(const uint8_t* pals, size_t pal_count);
// pixels are ready to be rendered
// a screen is 320 * 200 byte
void doomtime_ipc_frame_ready(const uint8_t* pixels);
// blocks until data is read by consumer
// from frame synchronization
void doomtime_ipc_frame_rendered();
// async
void doomtime_ipc_palette_changed(uint8_t p);
// blocks until client disconnects
void doomtime_ipc_disconnected();

#ifdef __cplusplus
}
#endif
