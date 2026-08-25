#pragma once

#include <pebble.h>

void logos_init(void);
void logos_deinit(void);
GBitmap *logo_for_code(const char *code);
GBitmap *logo_for_comp(int comp);
GBitmap *logo_trophy(void);
void draw_logo(GContext *ctx, GBitmap *bmp, GRect box, bool highlight);
