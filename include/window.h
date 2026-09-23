#ifndef D1X_VIRTUAL_WINDOW_H
#define D1X_VIRTUAL_WINDOW_H

typedef struct window {
    int id_marker;
    void *canvas_hook;
} window;

#include "event.h"

window *window_create(void *canvas, int x, int y, int w, int h, int (*handler)(window *, d_event *, void *), void *userdata);
void window_close(window *w);

#endif
