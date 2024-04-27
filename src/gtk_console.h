#ifndef VIEWER_CONSOLE_H
#define VIEWER_CONSOLE_H

#include <gtk/gtk.h>
#include "minilang.h"

typedef struct gtk_console_t gtk_console_t;

gtk_console_t *gtk_console(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals);
void gtk_console_show(gtk_console_t *Console, GtkWindow *Parent);
void gtk_console_log(gtk_console_t *Console, ml_value_t *Value);
int gtk_console_append(gtk_console_t *Console, const char *Buffer, int Length);
ml_value_t *gtk_console_print(gtk_console_t *Console, int Count, ml_value_t **Args);
void gtk_console_printf(gtk_console_t *Console, const char *Format, ...);
void gtk_console_load_file(gtk_console_t *Console, const char *FileName, ml_value_t *Args);
void gtk_console_evaluate(gtk_console_t *Console, const char *Text);

void gtk_console_init();

#endif
