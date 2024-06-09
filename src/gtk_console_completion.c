#include "gtk_console_completion.h"
#include "ml_gir.h"

struct _ConsoleCompletionProvider {
	GObject parent_instance;
	ml_compiler_t *Compiler;
};

static void gtk_console_completion_provider_interface_init(GtkSourceCompletionProviderIface *Interface);

G_DEFINE_TYPE_WITH_CODE(ConsoleCompletionProvider, gtk_console_completion_provider, G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE(GTK_SOURCE_TYPE_COMPLETION_PROVIDER, gtk_console_completion_provider_interface_init))

static void gtk_console_completion_provider_class_init(ConsoleCompletionProviderClass *klass) {
}

static void gtk_console_completion_provider_init(ConsoleCompletionProvider *Provider) {
}

static gchar *gtk_console_completion_provider_get_name(ConsoleCompletionProvider *Provider) {
	return g_strdup("console-completion");
}

/*
static GdkPixbuf *console_completion_provider_get_icon(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);

}

static const gchar *console_completion_provider_get_icon_name(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);

}

static GIcon *console_completion_provider_get_gicon(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);

}
*/

typedef struct {
	gchar *Prefix;
	GList *Proposals;
	int PrefixLength;
} populate_info_t;

static int populate_fn(const char *Name, void *Value, populate_info_t *Info) {
	if (Info->Prefix) {
		if (strncmp(Name, Info->Prefix, Info->PrefixLength)) return 0;
	}
	GtkSourceCompletionItem *Item = gtk_source_completion_item_new();
	gtk_source_completion_item_set_label(Item, g_strdup(Name));
	gtk_source_completion_item_set_text(Item, g_strdup(Name));
	Info->Proposals = g_list_prepend(Info->Proposals, Item);
	return 0;
}

static void gtk_console_completion_provider_populate(ConsoleCompletionProvider *Provider, GtkSourceCompletionContext *Context) {
	GtkTextIter Start;
	gtk_source_completion_context_get_iter(Context, &Start);
	populate_info_t Info[1];
	Info->Prefix = NULL;
	Info->Proposals = NULL;
	if (gtk_text_iter_ends_word(&Start)) {
		GtkTextIter Iter = Start;
		gtk_text_iter_backward_word_start(&Start);
		Info->Prefix = gtk_text_iter_get_text(&Start, &Iter);
		Info->PrefixLength = strlen(Info->Prefix);
	}
	GtkTextIter End = Start;
	gtk_text_iter_backward_visible_word_start(&Start);
	gtk_text_iter_backward_chars(&End, 2);
	gchar *Name = gtk_text_iter_get_text(&Start, &End);
	ml_value_t *Value = ml_compiler_lookup(Provider->Compiler, Name, "", 0, 0);
	if (!Value) {
		GtkTextIter Iter = Start;
		do {
			if (!gtk_text_iter_backward_char(&Iter)) break;
			if (gtk_text_iter_get_char(&Iter) != ':') break;
			if (!gtk_text_iter_backward_char(&Iter)) break;
			if (gtk_text_iter_get_char(&Iter) != ':') break;
			gtk_text_iter_backward_word_start(&Start);
			gchar *Name0 = gtk_text_iter_get_text(&Start, &Iter);
			ml_value_t *Value0 = ml_compiler_lookup(Provider->Compiler, Name0, "", 0, 0);
			g_free(Name0);
			if (!Value0) break;
			if (ml_is(Value0, MLGlobalT)) Value0 = ml_global_get(Value0);
			if (ml_is(Value0, MLGirTypelibT)) Value = ml_gir_import(Value0, Name);
			if (ml_is(Value0, MLTypeT)) Value = stringmap_search(((ml_type_t *)Value0)->Exports, Name);
		} while (0);
	}
	g_free(Name);
	if (Value && ml_is(Value, MLGlobalT)) Value = ml_global_get(Value);
	if (Value) {
		if (ml_is(Value, MLTypeT)) {
			ml_type_t *Type = (ml_type_t *)Value;
			stringmap_foreach(Type->Exports, Info, (void *)populate_fn);
		} else if (ml_is(Value, MLModuleT)) {
			ml_module_t *Module = (ml_module_t *)Value;
			stringmap_foreach(Module->Exports, Info, (void *)populate_fn);
		} else if (ml_is(Value, MLGirTypelibT)) {
			const char *Namespace = ml_gir_get_namespace(Value);
			int Total = g_irepository_get_n_infos(NULL, Namespace);
			for (int I = 0; I < Total; ++I) {
				GIBaseInfo *Base = g_irepository_get_info(NULL, Namespace, I);
				const char *Name = g_base_info_get_name(Base);
				if (Info->Prefix) {
					if (strncmp(Name, Info->Prefix, Info->PrefixLength)) continue;
				}
				GtkSourceCompletionItem *Item = gtk_source_completion_item_new();
				gtk_source_completion_item_set_label(Item, g_strdup(Name));
				gtk_source_completion_item_set_text(Item, g_strdup(Name));
				Info->Proposals = g_list_prepend(Info->Proposals, Item);
			}
		}
	}
	if (Info->Prefix) g_free(Info->Prefix);
	Info->Proposals = g_list_reverse(Info->Proposals);
	gtk_source_completion_context_add_proposals(Context, GTK_SOURCE_COMPLETION_PROVIDER(Provider), Info->Proposals, TRUE);
}

static gboolean gtk_console_completion_provider_match(ConsoleCompletionProvider *Provider, GtkSourceCompletionContext *Context) {
	GtkTextIter Iter;
	gtk_source_completion_context_get_iter(Context, &Iter);
	if (gtk_text_iter_ends_word(&Iter)) {
		if (!gtk_text_iter_backward_word_start(&Iter)) return FALSE;
	}
	if (!gtk_text_iter_backward_char(&Iter)) return FALSE;
	if (gtk_text_iter_get_char(&Iter) != ':') return FALSE;
	if (!gtk_text_iter_backward_char(&Iter)) return FALSE;
	if (gtk_text_iter_get_char(&Iter) != ':') return FALSE;
	return TRUE;
}

/*
static GtkSourceCompletionActivation gtk_console_completion_provider_get_activation(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);

}

static GtkWidget *gtk_console_completion_provider_get_info_widget(ConsoleCompletionProvider *Provider, GtkSourceCompletionProposal *Proposal) {
	printf("%s()\n", __FUNCTION__);

}

static void gtk_console_completion_provider_update_info(ConsoleCompletionProvider *Provider, GtkSourceCompletionProposal *Proposal, GtkSourceCompletionInfo *Info) {
	printf("%s()\n", __FUNCTION__);
}

static gboolean	gtk_console_completion_provider_get_start_iter(ConsoleCompletionProvider *Provider, GtkSourceCompletionContext *Context, GtkSourceCompletionProposal *Proposal, GtkTextIter *Iter) {
	printf("%s()\n", __FUNCTION__);
}

static gboolean	gtk_console_completion_provider_activate_proposal(ConsoleCompletionProvider *Provider, GtkSourceCompletionProposal *Proposal, GtkTextIter *Iter) {
	printf("%s()\n", __FUNCTION__);
}

static gint gtk_console_completion_provider_get_interactive_delay(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);
}

static gint gtk_console_completion_provider_get_priority(ConsoleCompletionProvider *Provider) {
	printf("%s()\n", __FUNCTION__);
}
*/

static void gtk_console_completion_provider_interface_init(GtkSourceCompletionProviderIface *Interface) {
	Interface->get_name = (void *)gtk_console_completion_provider_get_name;
	//Interface->get_icon = (void *)console_completion_provider_get_icon;
	//Interface->get_icon_name = (void *)console_completion_provider_get_icon_name;
	//Interface->get_gicon = (void *)console_completion_provider_get_gicon;
	Interface->populate = (void *)gtk_console_completion_provider_populate;
	Interface->match = (void *)gtk_console_completion_provider_match;
	//Interface->get_activation = (void *)console_completion_provider_get_activation;
	//Interface->get_info_widget = (void *)console_completion_provider_get_info_widget;
	//Interface->update_info = (void *)console_completion_provider_update_info;
	//Interface->get_start_iter = (void *)console_completion_provider_get_start_iter;
	//Interface->activate_proposal = (void *)console_completion_provider_activate_proposal;
	//Interface->get_interactive_delay = (void *)console_completion_provider_get_interactive_delay;
	//Interface->get_priority = (void *)console_completion_provider_get_priority;
}

GtkSourceCompletionProvider *gtk_console_completion_provider(ml_compiler_t *Compiler) {
	ConsoleCompletionProvider *Provider = g_object_new(CONSOLE_TYPE_COMPLETION_PROVIDER, NULL);
	Provider->Compiler = Compiler;
	return GTK_SOURCE_COMPLETION_PROVIDER(Provider);
}
