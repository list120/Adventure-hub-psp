/* text.h - tiny 5x7 bitmap font drawn with filled rectangles (no textures) */
#ifndef TEXT_H
#define TEXT_H

#include <psptypes.h>

/* Draws an uppercase-styled string.  scale = pixel size (int>=1). */
void text_draw(const char *str, float x, float y, float scale, u32 color);
/* Same, but centred horizontally around x. */
void text_draw_center(const char *str, float cx, float y, float scale, u32 color);
/* Same, but with a soft drop shadow (looks nicer on bright backgrounds). */
void text_draw_shadow(const char *str, float x, float y, float scale, u32 color, u32 shadow);
float text_width(const char *str, float scale);

#endif
