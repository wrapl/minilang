#include "gtk_console.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtksourceview/gtksource.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gc/gc.h>
#include "minilang.h"
#include "ml_macros.h"
#include <stringmap.h>
#include "ml_compiler.h"
#include <sys/stat.h>

#include "ml_gir.h"

#define MAX_HISTORY 128

static ml_value_t *StringMethod;

struct console_t {
	GtkWidget *Window, *LogScrolled, *LogView, *InputView;
	GtkTextTag *OutputTag, *ResultTag, *ErrorTag, *BinaryTag;
	GtkTextMark *EndMark;
	ml_getter_t ParentGetter;
	void *ParentGlobals;
	mlc_scanner_t *Scanner;
	char *Input;
	char *History[MAX_HISTORY];
	int HistoryIndex, HistoryEnd;
	stringmap_t Globals[1];
	mlc_context_t Context[1];
};

#ifdef MINGW
static char *stpcpy(char *Dest, const char *Source) {
	while (*Source) *Dest++ = *Source++;
	return Dest;
}

#define lstat stat
#endif

static ml_value_t *console_global_get(console_t *Console, const char *Name) {
	return stringmap_search(Console->Globals, Name) ?: (Console->ParentGetter)(Console->ParentGlobals, Name);
}

static char *console_read(console_t *Console) {
	if (!Console->Input) return 0;
	char *Line = Console->Input;
	for (char *End = Console->Input; *End; ++End) {
		if (*End == '\n') {
			int Length = End - Console->Input;
			Console->Input = End + 1;
			char *Buffer = snew(Length + 2);
			memcpy(Buffer, Line, Length);
			Buffer[Length] = '\n';
			Buffer[Length + 1] = 0;
			return Buffer;
		}
	}
	Console->Input = 0;
	printf("Line = <%s>\n", Line);
	return Line;
}

void console_log(console_t *Console, ml_value_t *Value) {
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	if (Value->Type == MLErrorT) {
		char *Buffer;
		int Length = asprintf(&Buffer, "Error: %s\n", ml_error_message(Value));
		gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ErrorTag, NULL);
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Value, I, &Source, &Line); ++I) {
			Length = asprintf(&Buffer, "\t%s:%d\n", Source, Line);
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ErrorTag, NULL);
		}
	} else {
		ml_value_t *String = ml_call(StringMethod, 1, &Value);
		if (String->Type == MLStringT) {
			const char *Buffer = ml_string_value(String);
			int Length = ml_string_length(String);
			if (g_utf8_validate(Buffer, Length, NULL)) {
				gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ResultTag, NULL);
			} else {
				gtk_text_buffer_insert_with_tags(LogBuffer, End, "<", 1, Console->BinaryTag, NULL);
				for (int I = 0; I < Length; ++I) {
					static char HexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
					char Bytes[4] = " ??";
					unsigned char Byte = Buffer[I];
					Bytes[1] = HexChars[Byte >> 4];
					Bytes[2] = HexChars[Byte & 15];
					gtk_text_buffer_insert_with_tags(LogBuffer, End, Bytes, 3, Console->BinaryTag, NULL);
				}
				gtk_text_buffer_insert_with_tags(LogBuffer, End, " >", 2, Console->BinaryTag, NULL);
			}
			gtk_text_buffer_insert_with_tags(LogBuffer, End, "\n", 1, Console->ResultTag, NULL);
		} else {
			char *Buffer;
			int Length = asprintf(&Buffer, "<%s>\n", Value->Type->Name);
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ResultTag, NULL);
		}
	}
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(Console->LogView), Console->EndMark);
}

static void console_submit(GtkWidget *Button, console_t *Console) {
	GtkTextBuffer *InputBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView));
	GtkTextIter InputStart[1], InputEnd[1];
	gtk_text_buffer_get_bounds(InputBuffer, InputStart, InputEnd);
	Console->Input = gtk_text_buffer_get_text(InputBuffer, InputStart, InputEnd, FALSE);

	int HistoryEnd = Console->HistoryEnd;
	Console->History[HistoryEnd] = GC_strdup(Console->Input);
	Console->HistoryIndex = Console->HistoryEnd = (HistoryEnd + 1) % MAX_HISTORY;

	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_source_buffer_create_source_mark(GTK_SOURCE_BUFFER(LogBuffer), NULL, "result", End);
	gtk_text_buffer_insert_range(LogBuffer, End, InputStart, InputEnd);
	gtk_text_buffer_insert(LogBuffer, End, "\n", -1);
	gtk_text_buffer_set_text(InputBuffer, "", 0);

	mlc_scanner_t *Scanner = Console->Scanner;
	mlc_on_error(Console->Context) {
		char *Buffer;
		int Length = asprintf(&Buffer, "Error: %s\n", ml_error_message(Console->Context->Error));
		gtk_text_buffer_get_end_iter(LogBuffer, End);
		gtk_text_buffer_insert(LogBuffer, End, Buffer, Length);
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Console->Context->Error, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
		ml_scanner_reset(Scanner);
	} else {
		for (;;) {
			mlc_expr_t *Expr = ml_accept_command(Scanner, Console->Globals);
			if (Expr == (mlc_expr_t *)-1) break;
			ml_value_t *Closure = ml_compile(Expr, NULL, Console->Context);
			ml_value_t *Result = ml_call(Closure, 0, NULL);
			Result = Result->Type->deref(Result);
			console_log(Console, Result);
		}
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(Console->LogView), Console->EndMark);
	}
	gtk_widget_grab_focus(Console->InputView);
}

static void console_clear(GtkWidget *Button, console_t *Console) {
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	GtkTextIter Start[1], End[1];
	gtk_text_buffer_get_start_iter(LogBuffer, Start);
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_text_buffer_delete(LogBuffer, Start, End);
}

static void console_style_changed(GtkComboBoxText *Widget, console_t *Console) {
	const char *StyleId = gtk_combo_box_text_get_active_text(Widget);
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme *StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, StyleId);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), StyleScheme);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView))), StyleScheme);
}

static void console_font_changed(GtkFontChooser *Widget, console_t *Console) {
	gchar *FontName = gtk_font_chooser_get_font(Widget);
	PangoFontDescription *FontDescription = pango_font_description_from_string(FontName);
	gtk_widget_override_font(Console->InputView, FontDescription);
	gtk_widget_override_font(Console->LogView, FontDescription);
}

static gboolean console_keypress(GtkWidget *Widget, GdkEventKey *Event, console_t *Console) {
	switch (Event->keyval) {
	case GDK_KEY_Return: {
		if (Event->state & GDK_SHIFT_MASK) {
			console_submit(NULL, Console);
			return TRUE;
		}
		break;
	}
	case GDK_KEY_Up: {
		if (Event->state & GDK_SHIFT_MASK) {
			int HistoryIndex = (Console->HistoryIndex + MAX_HISTORY - 1) % MAX_HISTORY;
			if (Console->History[HistoryIndex]) {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), Console->History[HistoryIndex], -1);
				Console->HistoryIndex = HistoryIndex;
			}
			return TRUE;
		}
		break;
	}
	case GDK_KEY_Down: {
		if (Event->state & GDK_SHIFT_MASK) {
			int HistoryIndex = (Console->HistoryIndex + 1) % MAX_HISTORY;
			if (Console->History[HistoryIndex]) {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), Console->History[HistoryIndex], -1);
				Console->HistoryIndex = HistoryIndex;
			}
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}


void console_show(console_t *Console, GtkWindow *Parent) {
	gtk_window_set_transient_for(GTK_WINDOW(Console->Window), Parent);
	gtk_widget_show_all(Console->Window);
	gtk_widget_grab_focus(Console->InputView);
}

void console_append(console_t *Console, const char *Buffer, int Length) {
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);

	if (g_utf8_validate(Buffer, Length, NULL)) {
		gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->OutputTag, NULL);
	} else {
		gtk_text_buffer_insert_with_tags(LogBuffer, End, "<", 1, Console->BinaryTag, NULL);
		for (int I = 0; I < Length; ++I) {
			static char HexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
			char Bytes[4] = " ??";
			unsigned char Byte = Buffer[I];
			Bytes[1] = HexChars[Byte >> 4];
			Bytes[2] = HexChars[Byte & 15];
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Bytes, 3, Console->BinaryTag, NULL);
		}
		gtk_text_buffer_insert_with_tags(LogBuffer, End, " >", 2, Console->BinaryTag, NULL);
	}
	while (gtk_events_pending()) gtk_main_iteration();
}

ml_value_t *console_print(console_t *Console, int Count, ml_value_t **Args) {
	ml_value_t *StringMethod = ml_method("string");
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_call(StringMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		const char *String = ml_string_value(Result);
		int Length = ml_string_length(Result);
		if (g_utf8_validate(String, Length, NULL)) {
			gtk_text_buffer_insert_with_tags(LogBuffer, End, String, Length, Console->OutputTag, NULL);
		} else {
			gtk_text_buffer_insert_with_tags(LogBuffer, End, "<", 1, Console->BinaryTag, NULL);
			for (int I = 0; I < Length; ++I) {
				static char HexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
				char Bytes[4] = " ??";
				unsigned char Byte = String[I];
				Bytes[1] = HexChars[Byte >> 4];
				Bytes[2] = HexChars[Byte & 15];
				gtk_text_buffer_insert_with_tags(LogBuffer, End, Bytes, 3, Console->BinaryTag, NULL);
			}
			gtk_text_buffer_insert_with_tags(LogBuffer, End, " >", 2, Console->BinaryTag, NULL);
		}
	}
	while (gtk_events_pending()) gtk_main_iteration();
	return MLNil;
}

void console_printf(console_t *Console, const char *Format, ...) {
	char *Buffer;
	va_list Args;
	va_start(Args, Format);
	int Length = vasprintf(&Buffer, Format, Args);
	va_end(Args);
	console_append(Console, Buffer, Length);
	free(Buffer);
}

static ml_value_t *console_set_font(console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	PangoFontDescription *FontDescription = pango_font_description_new();
	pango_font_description_set_family(FontDescription, ml_string_value(Args[0]));
	pango_font_description_set_size(FontDescription, PANGO_SCALE * ml_integer_value(Args[1]));
	gtk_widget_override_font(Console->InputView, FontDescription);
	gtk_widget_override_font(Console->LogView, FontDescription);
	return MLNil;
}

static ml_value_t *console_set_style(console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme *StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, ml_string_value(Args[0]));
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), StyleScheme);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView))), StyleScheme);
	return MLNil;
}

console_t *console_new(ml_getter_t GlobalGet, void *Globals) {
	StringMethod = ml_method("string");

	console_t *Console = new(console_t);
	Console->ParentGetter = GlobalGet;
	Console->ParentGlobals = Globals;
	Console->Input = 0;
	Console->HistoryIndex = 0;
	Console->HistoryEnd = 0;
	Console->Context->GlobalGet = (ml_getter_t)console_global_get;
	Console->Context->Globals = Console;
	Console->Scanner = ml_scanner("Console", Console, (void *)console_read, Console->Context);
	GtkWidget *Container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	Console->InputView = gtk_source_view_new();

	PangoFontDescription *FontDescription = pango_font_description_new();
	pango_font_description_set_family(FontDescription, "Cascadia Code,monospace");

	gtk_widget_override_font(Console->InputView, FontDescription);

	GtkSourceLanguageManager *LanguageManager = gtk_source_language_manager_get_default();
	GtkSourceLanguage *Language = gtk_source_language_manager_get_language(LanguageManager, "minilang");
	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), Language);
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();
	GtkSourceStyleScheme *StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, "tango");
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), StyleScheme);
	GtkTextTagTable *TagTable = gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)));
	Console->OutputTag = gtk_text_tag_new("log-output");
	Console->ResultTag = gtk_text_tag_new("log-result");
	Console->ErrorTag = gtk_text_tag_new("log-error");
	Console->BinaryTag = gtk_text_tag_new("log-binary");
	g_object_set(Console->OutputTag,
		"background", "#FFFFF0",
	NULL);
	g_object_set(Console->ResultTag,
		"background", "#FFF0F0",
		"foreground", "#303030",
		"indent", 10,
		"pixels-below-lines", 20,
	NULL);
	g_object_set(Console->ErrorTag,
		"background", "#FFF0F0",
		"foreground", "#FF0000",
		"indent", 10,
	NULL);
	g_object_set(Console->BinaryTag,
		"background", "#F0F0FF",
		"foreground", "#FF8000",
	NULL);
	gtk_text_tag_table_add(TagTable, Console->OutputTag);
	gtk_text_tag_table_add(TagTable, Console->ResultTag);
	gtk_text_tag_table_add(TagTable, Console->ErrorTag);
	gtk_text_tag_table_add(TagTable, Console->BinaryTag);
	GtkSourceBuffer *LogBuffer = gtk_source_buffer_new(TagTable);
	Console->LogView = gtk_source_view_new_with_buffer(LogBuffer);
	gtk_widget_override_font(Console->LogView, FontDescription);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(Console->LogView), FALSE);
	gtk_source_buffer_set_style_scheme(LogBuffer, StyleScheme);

	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(Console->LogView), 4);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(Console->LogView), 4);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(Console->LogView), 4);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(Console->LogView), 4);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(Console->LogView), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(Console->LogView), TRUE);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(Console->LogView), 4);
	GtkTextIter End[1];
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(LogBuffer), End);
	Console->EndMark = gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(LogBuffer), "end", End, FALSE);

	GtkSourceMarkAttributes *MarkAttributes = gtk_source_mark_attributes_new();
	GdkPixbuf *MarkPixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
	gdk_pixbuf_fill(MarkPixbuf, 0xFF8000FF);
	gtk_source_mark_attributes_set_pixbuf(MarkAttributes, MarkPixbuf);
	gtk_source_view_set_mark_attributes(GTK_SOURCE_VIEW(Console->LogView), "result", MarkAttributes, 10);
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(Console->LogView), TRUE);

	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(Console->InputView), 4);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(Console->InputView), 4);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(Console->InputView), 4);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(Console->InputView), 4);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(Console->InputView), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(Console->InputView), TRUE);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(Console->InputView), 4);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(Console->InputView), TRUE);

	GtkWidget *InputPanel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *SubmitButton = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(SubmitButton), gtk_image_new_from_icon_name("go-jump-symbolic", GTK_ICON_SIZE_BUTTON));
	GtkWidget *ClearButton = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(ClearButton), gtk_image_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON));
	Console->LogScrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(Console->LogScrolled), Console->LogView);
	gtk_box_pack_start(GTK_BOX(InputPanel), Console->InputView, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(InputPanel), SubmitButton, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(InputPanel), ClearButton, FALSE, FALSE, 2);

	GtkActionBar *ActionBar = GTK_ACTION_BAR(gtk_action_bar_new());
	GtkWidget *StyleCombo = gtk_combo_box_text_new();
	for (const gchar * const * StyleId = gtk_source_style_scheme_manager_get_scheme_ids(StyleManager); StyleId[0]; ++StyleId) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(StyleCombo), StyleId[0]);
	}
	g_signal_connect(G_OBJECT(StyleCombo), "changed", G_CALLBACK(console_style_changed), Console);
	gtk_action_bar_pack_start(ActionBar, StyleCombo);

	GtkWidget *FontButton = gtk_font_button_new();
	g_signal_connect(G_OBJECT(FontButton), "font-set", G_CALLBACK(console_font_changed), Console);
	gtk_action_bar_pack_start(ActionBar, FontButton);


	gtk_box_pack_start(GTK_BOX(Container), GTK_WIDGET(ActionBar), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(Container), Console->LogScrolled, TRUE, TRUE, 2);
	GtkWidget *InputFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(InputFrame), InputPanel);
	gtk_box_pack_start(GTK_BOX(Container), InputFrame, FALSE, TRUE, 2);
	g_signal_connect(G_OBJECT(Console->InputView), "key-press-event", G_CALLBACK(console_keypress), Console);
	g_signal_connect(G_OBJECT(SubmitButton), "clicked", G_CALLBACK(console_submit), Console);
	g_signal_connect(G_OBJECT(ClearButton), "clicked", G_CALLBACK(console_clear), Console);
	Console->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add(GTK_CONTAINER(Console->Window), Container);
	gtk_window_set_default_size(GTK_WINDOW(Console->Window), 640, 480);
	g_signal_connect(G_OBJECT(Console->Window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), Console);

	stringmap_insert(Globals, "set_font", ml_function(Console, (ml_callback_t)console_set_font));
	stringmap_insert(Globals, "set_style", ml_function(Console, (ml_callback_t)console_set_style));

	GError *Error = 0;
	g_irepository_require(NULL, "Gtk", NULL, 0, &Error);
	g_irepository_require(NULL, "GtkSource", NULL, 0, &Error);
	stringmap_insert(Globals, "Console", ml_gir_instance_get(Console->Window));
	stringmap_insert(Globals, "InputView", ml_gir_instance_get(Console->InputView));
	stringmap_insert(Globals, "LogView", ml_gir_instance_get(Console->LogView));

	return Console;
}
