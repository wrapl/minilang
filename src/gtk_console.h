#ifndef VIEWER_CONSOLE_H
#define VIEWER_CONSOLE_H

#include <gtk/gtk.h>
#include "minilang.h"

typedef struct console_t console_t;

console_t *console_new(ml_getter_t GlobalGet, void *Globals);
void console_show(console_t *Console, GtkWindow *Parent);
void console_log(console_t *Console, ml_value_t *Value);
int console_append(console_t *Console, const char *Buffer, int Length);
ml_value_t *console_print(console_t *Console, int Count, ml_value_t **Args);
void console_printf(console_t *Console, const char *Format, ...);

#endif
