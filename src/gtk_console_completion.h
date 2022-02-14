#ifndef GTK_CONSOLE_COMPLETION_H
#define GTK_CONSOLE_COMPLETION_H

#include <glib-object.h>
#include <gtksourceview/gtksource.h>
#include "ml_compiler.h"

G_BEGIN_DECLS

#define CONSOLE_TYPE_COMPLETION_PROVIDER gtk_console_completion_provider_get_type()
G_DECLARE_FINAL_TYPE(ConsoleCompletionProvider, gtk_console_completion_provider, CONSOLE, COMPLETION_PROVIDER, GObject)

GtkSourceCompletionProvider *gtk_console_completion_provider(ml_compiler_t *Compiler);

G_END_DECLS

#endif
