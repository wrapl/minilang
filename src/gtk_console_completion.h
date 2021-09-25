#ifndef GTK_CONSOLE_COMPLETION_H
#define GTK_CONSOLE_COMPLETION_H

#include <glib-object.h>
#include <gtksourceview/gtksource.h>
#include "ml_compiler.h"

G_BEGIN_DECLS

#define CONSOLE_TYPE_COMPLETION_PROVIDER console_completion_provider_get_type()
G_DECLARE_FINAL_TYPE(ConsoleCompletionProvider, console_completion_provider, CONSOLE, COMPLETION_PROVIDER, GObject)

GtkSourceCompletionProvider *console_completion_provider_new(ml_compiler_t *Compiler);

G_END_DECLS

#endif
