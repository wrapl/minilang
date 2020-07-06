#include "gtk_console.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtksourceview/gtksource.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <girepository.h>
#include <gc/gc.h>
#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include "ml_compiler.h"
#include <sys/stat.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

#include "ml_gir.h"
#include "ml_runtime.h"
#include "ml_bytecode.h"
#include "ml_debugger.h"

#define MAX_HISTORY 128

typedef struct console_debugger_t console_debugger_t;

struct console_debugger_t {
	console_debugger_t *Prev;
	interactive_debugger_t *Debugger;
};

struct console_t {
	GtkWidget *Window, *LogScrolled, *LogView, *InputView;
	GtkLabel *MemoryBar;
	GtkTextTag *OutputTag, *ResultTag, *ErrorTag, *BinaryTag;
	GtkTextMark *EndMark;
	ml_getter_t ParentGetter;
	void *ParentGlobals;
	console_debugger_t *Debugger;
	const char *ConfigPath;
	const char *FontName;
	GKeyFile *Config;
	mlc_scanner_t *Scanner;
	char *Input;
	char *History[MAX_HISTORY];
	int HistoryIndex, HistoryEnd;
	stringmap_t Globals[1];
	stringmap_t Cycles[1];
	stringmap_t Combos[1];
	char Chars[32];
	int NumChars;
};

#ifdef MINGW
static char *stpcpy(char *Dest, const char *Source) {
	while (*Source) *Dest++ = *Source++;
	return Dest;
}

#define lstat stat
#endif

static ml_value_t *console_global_get(console_t *Console, const char *Name) {
	if (Console->Debugger) {
		ml_value_t *Value = interactive_debugger_get(Console->Debugger->Debugger, Name);
		if (Value) return Value;
	}
	ml_value_t *Value = stringmap_search(Console->Globals, Name);
	if (Value) return Value;
	Value = (Console->ParentGetter)(Console->ParentGlobals, Name);
	if (Value) return Value;
	Value = ml_uninitialized();
	stringmap_insert(Console->Globals, Name, Value);
	return Value;
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
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			Length = asprintf(&Buffer, "\t%s:%d\n", Source.Name, Source.Line);
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ErrorTag, NULL);
		}
	} else {
		ml_value_t *String = ml_string_of(Value);
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

typedef struct {
	ml_state_t Base;
	console_t *Console;
} ml_console_repl_state_t;

static ml_value_t ConsoleBreak[1] = {{MLAnyT}};

static void ml_console_repl_run(ml_console_repl_state_t *State, ml_value_t *Result) {
	if (!Result || Result == ConsoleBreak) {
		gtk_widget_grab_focus(State->Console->InputView);
		return;
	}
	console_log(State->Console, Result);
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(State->Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_text_buffer_insert(LogBuffer, End, "\n", 1);
	if (Result->Type == MLErrorT) {
		gtk_widget_grab_focus(State->Console->InputView);
		return;
	}
	return ml_command_evaluate((ml_state_t *)State, State->Console->Scanner, State->Console->Globals);
}

static void console_submit(GtkWidget *Button, console_t *Console) {
	GtkTextBuffer *InputBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView));
	GtkTextIter InputStart[1], InputEnd[1];

	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(InputBuffer), FALSE);
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

	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(InputBuffer), TRUE);

	mlc_scanner_t *Scanner = Console->Scanner;
	ml_console_repl_state_t *State = new(ml_console_repl_state_t);
	State->Base.run = (ml_state_fn)ml_console_repl_run;
	State->Base.Context = &MLRootContext;
	State->Console = Console;
	ml_scanner_reset(Scanner);
	ml_command_evaluate((ml_state_t *)State, Scanner, Console->Globals);
}

static void console_debug_enter(console_t *Console, interactive_debugger_t *Debugger) {
	console_debugger_t *ConsoleDebugger = new(console_debugger_t);
	ConsoleDebugger->Prev = Console->Debugger;
	ConsoleDebugger->Debugger = Debugger;
	Console->Debugger = ConsoleDebugger;
}

static void console_debug_exit(ml_state_t *Caller, console_t *Console) {
	interactive_debugger_t *Debugger = Console->Debugger->Debugger;
	Console->Debugger = Console->Debugger->Prev;
	return interactive_debugger_resume(Debugger);
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

	g_key_file_set_string(Console->Config, "gtk-console", "style", StyleId);
	g_key_file_save_to_file(Console->Config, Console->ConfigPath, NULL);
}

static void console_font_changed(GtkFontChooser *Widget, console_t *Console) {
	gchar *FontName = gtk_font_chooser_get_font(Widget);
	Console->FontName = FontName;
	PangoFontDescription *FontDescription = pango_font_description_from_string(FontName);
	gtk_widget_override_font(Console->InputView, FontDescription);
	gtk_widget_override_font(Console->LogView, FontDescription);

	g_key_file_set_string(Console->Config, "gtk-console", "font", FontName);
	g_key_file_save_to_file(Console->Config, Console->ConfigPath, NULL);
}

static gboolean console_keypress(GtkWidget *Widget, GdkEventKey *Event, console_t *Console) {
	switch (Event->keyval) {
	case GDK_KEY_Return:
		Console->NumChars = 0;
		if (Event->state & GDK_CONTROL_MASK) {
			console_submit(NULL, Console);
			return TRUE;
		}
		break;
	case GDK_KEY_Up:
		Console->NumChars = 0;
		if (Event->state & GDK_CONTROL_MASK) {
			int HistoryIndex = (Console->HistoryIndex + MAX_HISTORY - 1) % MAX_HISTORY;
			if (Console->History[HistoryIndex]) {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), Console->History[HistoryIndex], -1);
				Console->HistoryIndex = HistoryIndex;
			}
			return TRUE;
		}
		break;
	case GDK_KEY_Down:
		Console->NumChars = 0;
		if (Event->state & GDK_CONTROL_MASK) {
			int HistoryIndex = (Console->HistoryIndex + 1) % MAX_HISTORY;
			if (Console->History[HistoryIndex]) {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), Console->History[HistoryIndex], -1);
				Console->HistoryIndex = HistoryIndex;
			}
			return TRUE;
		}
		break;
	case GDK_KEY_Escape:
	case GDK_KEY_Left:
	case GDK_KEY_Right:
		Console->NumChars = 0;
		break;
	case GDK_KEY_BackSpace:
		if (Console->NumChars > 0) --Console->NumChars;
		break;
	case GDK_KEY_Tab: {
		Console->Chars[Console->NumChars] = 0;
		for (int I = 0; I < Console->NumChars; ++I) {
			const char *Cycle = stringmap_search(Console->Cycles, Console->Chars + I);
			if (Cycle) {
				GtkTextIter Start[1], End[1];
				GtkTextBuffer *InputBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView));
				GtkTextMark *Cursor = gtk_text_buffer_get_insert(InputBuffer);
				gtk_text_buffer_get_iter_at_mark(InputBuffer, Start, Cursor);
				gtk_text_buffer_get_iter_at_mark(InputBuffer, End, Cursor);
				gtk_text_iter_backward_chars(Start, g_utf8_strlen(Console->Chars + I, Console->NumChars - I));
				Console->NumChars = stpcpy(Console->Chars + I, Cycle) - Console->Chars;
				gtk_text_buffer_delete(InputBuffer, Start, End);
				gtk_text_buffer_insert(InputBuffer, Start, Console->Chars + I, Console->NumChars - I);
				return TRUE;
			}
		}
		break;
	}
	default: {
		guint32 Unichar = gdk_keyval_to_unicode(Event->keyval);
		if (!Unichar) return FALSE;
		if (Unichar <= 32) {
			Console->NumChars = 0;
			return FALSE;
		}
		Console->NumChars += g_unichar_to_utf8(Unichar, Console->Chars + Console->NumChars);
		if (Console->NumChars > 16) {
			memmove(Console->Chars, Console->Chars + Console->NumChars - 16, 16);
			Console->NumChars = 16;
		}
		Console->Chars[Console->NumChars] = 0;
		for (int I = 0; I < Console->NumChars; ++I) {
			const char *Combo = stringmap_search(Console->Combos, Console->Chars + I);
			if (Combo) {
				GtkTextIter Start[1], End[1];
				GtkTextBuffer *InputBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView));
				GtkTextMark *Cursor = gtk_text_buffer_get_insert(InputBuffer);
				gtk_text_buffer_get_iter_at_mark(InputBuffer, Start, Cursor);
				gtk_text_buffer_get_iter_at_mark(InputBuffer, End, Cursor);
				gtk_text_iter_backward_chars(Start, g_utf8_strlen(Console->Chars + I, Console->NumChars - I) - 1);
				Console->NumChars = stpcpy(Console->Chars + I, Combo) - Console->Chars;
				gtk_text_buffer_delete(InputBuffer, Start, End);
				gtk_text_buffer_insert(InputBuffer, Start, Console->Chars + I, Console->NumChars - I);
				return TRUE;
			}
		}
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
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_string_of(Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		console_append(Console, ml_string_value(Result), ml_string_length(Result));
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

static ml_value_t *console_add_cycle(console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	for (int I = 1; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		stringmap_insert(Console->Cycles, ml_string_value(Args[I - 1]), (void *)ml_string_value(Args[I]));
	}
	stringmap_insert(Console->Cycles, ml_string_value(Args[Count - 1]), (void *)ml_string_value(Args[0]));
	return MLNil;
}

static ml_value_t *console_add_combo(console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	stringmap_insert(Console->Combos, ml_string_value(Args[0]), (void *)ml_string_value(Args[1]));
	stringmap_insert(Console->Cycles, ml_string_value(Args[1]), (void *)ml_string_value(Args[0]));
	return MLNil;
}

static void console_included_run(ml_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Caller;
	if (Value->Type == MLErrorT) ML_RETURN(Value);
	return Value->Type->call(Caller, Value, 0, NULL);
}

static void console_include_fnx(ml_state_t *Caller, console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->run = console_included_run;
	return ml_load(State, (ml_getter_t)console_global_get, Console, ml_string_value(Args[0]), NULL);
}

static gboolean console_update_status(console_t *Console) {
	GC_word HeapSize, FreeBytes, UnmappedBytes, BytesSinceGC, TotalBytes;
	GC_get_heap_usage_safe(&HeapSize, &FreeBytes, &UnmappedBytes, &BytesSinceGC, &TotalBytes);
	GC_word UsedSize = HeapSize - FreeBytes;
	int UsedBase, HeapBase;
	char UsedUnits, HeapUnits;
	if (UsedSize < (1 << 10)) {
		UsedBase = UsedSize;
		UsedUnits = 'b';
	} else if (UsedSize < (1 << 20)) {
		UsedBase = UsedSize >> 10;
		UsedUnits = 'k';
	} else if (UsedSize < (1 << 30)) {
		UsedBase = UsedSize >> 20;
		UsedUnits = 'M';
	} else {
		UsedBase = UsedSize >> 30;
		UsedUnits = 'G';
	}
	if (HeapSize < (1 << 10)) {
		HeapBase = HeapSize;
		HeapUnits = 'b';
	} else if (HeapSize < (1 << 20)) {
		HeapBase = HeapSize >> 10;
		HeapUnits = 'k';
	} else if (HeapSize < (1 << 30)) {
		HeapBase = HeapSize >> 20;
		HeapUnits = 'M';
	} else {
		HeapBase = HeapSize >> 30;
		HeapUnits = 'G';
	}

	char Text[48];
	sprintf(Text, "Memory: %d%c / %d%c", UsedBase, UsedUnits, HeapBase, HeapUnits);
	gtk_label_set_text(Console->MemoryBar, Text);
	/*printf("Memory Status:\n");
	printf("\tHeapSize = %ld\n", HeapSize);
	printf("\tFreeBytes = %ld\n", FreeBytes);
	printf("\tUnmappedBytes = %ld\n", UnmappedBytes);
	printf("\tBytesSinceGC = %ld\n", BytesSinceGC);
	printf("\tTotalBytes = %ld\n", TotalBytes);*/
	return G_SOURCE_CONTINUE;
}

console_t *console_new(ml_getter_t ParentGetter, void *ParentGlobals) {
	gtk_init(0, 0);

	console_t *Console = new(console_t);
	Console->ParentGetter = ParentGetter;
	Console->ParentGlobals = ParentGlobals;
	Console->Input = 0;
	Console->HistoryIndex = 0;
	Console->HistoryEnd = 0;
	Console->Scanner = ml_scanner("<console>", Console, (void *)console_read, (ml_getter_t)console_global_get, Console);
	GtkWidget *Container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	Console->InputView = gtk_source_view_new();

	asprintf((char **)&Console->ConfigPath, "%s/%s", g_get_user_config_dir(), "minilang.conf");
	Console->Config = g_key_file_new();
	g_key_file_load_from_file(Console->Config, Console->ConfigPath, G_KEY_FILE_NONE, NULL);

	GtkSourceLanguageManager *LanguageManager = gtk_source_language_manager_get_default();
	GtkSourceLanguage *Language = gtk_source_language_manager_get_language(LanguageManager, "minilang");
	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), Language);
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
	gtk_text_view_set_editable(GTK_TEXT_VIEW(Console->LogView), FALSE);
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();

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

	GtkWidget *StyleCombo = gtk_combo_box_text_new();
	for (const gchar * const * StyleId = gtk_source_style_scheme_manager_get_scheme_ids(StyleManager); StyleId[0]; ++StyleId) {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(StyleCombo), StyleId[0], StyleId[0]);
	}

	g_signal_connect(G_OBJECT(StyleCombo), "changed", G_CALLBACK(console_style_changed), Console);

	GtkWidget *FontButton = gtk_font_button_new();
	g_signal_connect(G_OBJECT(FontButton), "font-set", G_CALLBACK(console_font_changed), Console);

	gtk_box_pack_start(GTK_BOX(Container), Console->LogScrolled, TRUE, TRUE, 2);
	GtkWidget *InputFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(InputFrame), InputPanel);
	gtk_box_pack_start(GTK_BOX(Container), InputFrame, FALSE, TRUE, 2);
	g_signal_connect(G_OBJECT(Console->InputView), "key-press-event", G_CALLBACK(console_keypress), Console);
	g_signal_connect(G_OBJECT(SubmitButton), "clicked", G_CALLBACK(console_submit), Console);
	g_signal_connect(G_OBJECT(ClearButton), "clicked", G_CALLBACK(console_clear), Console);
	Console->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon_name(GTK_WINDOW(Console->Window), "face-smile");

	GtkWidget *MenuButton = gtk_menu_button_new();
	GtkWidget *ActionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(ActionsBox), StyleCombo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ActionsBox), FontButton, FALSE, TRUE, 0);
	GtkWidget *ActionsPopover = gtk_popover_new(MenuButton);
	gtk_container_add(GTK_CONTAINER(ActionsPopover), ActionsBox);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(MenuButton), ActionsPopover);
	gtk_widget_show_all(ActionsBox);


	GtkWidget *HeaderBar = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(HeaderBar), "Minilang");
	gtk_header_bar_set_has_subtitle(GTK_HEADER_BAR(HeaderBar), FALSE);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(HeaderBar), MenuButton);
	gtk_window_set_titlebar(GTK_WINDOW(Console->Window), HeaderBar);

	GtkWidget *MemoryBar = gtk_label_new("");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(HeaderBar), MemoryBar);

	Console->MemoryBar = GTK_LABEL(MemoryBar);

	gtk_container_add(GTK_CONTAINER(Console->Window), Container);
	gtk_window_set_default_size(GTK_WINDOW(Console->Window), 640, 480);
	g_signal_connect(G_OBJECT(Console->Window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), Console);

	stringmap_insert(Console->Globals, "set_font", ml_cfunction(Console, (ml_callback_t)console_set_font));
	stringmap_insert(Console->Globals, "set_style", ml_cfunction(Console, (ml_callback_t)console_set_style));
	stringmap_insert(Console->Globals, "add_cycle", ml_cfunction(Console, (ml_callback_t)console_add_cycle));
	stringmap_insert(Console->Globals, "add_combo", ml_cfunction(Console, (ml_callback_t)console_add_combo));
	stringmap_insert(Console->Globals, "include", ml_cfunctionx(Console, (ml_callbackx_t)console_include_fnx));

	if (g_key_file_has_key(Console->Config, "gtk-console", "font", NULL)) {
		Console->FontName = g_key_file_get_string(Console->Config, "gtk-console", "font", NULL);
	} else {
		Console->FontName = "Monospace 10";
	}
	PangoFontDescription *FontDescription = pango_font_description_from_string(Console->FontName);
	gtk_widget_override_font(Console->InputView, FontDescription);
	gtk_widget_override_font(Console->LogView, FontDescription);
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(FontButton), Console->FontName);

	if (g_key_file_has_key(Console->Config, "gtk-console", "style", NULL)) {
		const char *StyleId = g_key_file_get_string(Console->Config, "gtk-console", "style", NULL);
		GtkSourceStyleScheme *StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, StyleId);
		GtkSourceBuffer *InputBuffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)));
		gtk_source_buffer_set_style_scheme(InputBuffer, StyleScheme);
		gtk_source_buffer_set_style_scheme(LogBuffer, StyleScheme);
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(StyleCombo), StyleId);
	}

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

	g_timeout_add(1000, (GSourceFunc)console_update_status, Console);

	GError *Error = 0;
	g_irepository_require(NULL, "Gtk", NULL, 0, &Error);
	g_irepository_require(NULL, "GtkSource", NULL, 0, &Error);
	stringmap_insert(Console->Globals, "Console", ml_gir_instance_get(Console->Window));
	stringmap_insert(Console->Globals, "InputView", ml_gir_instance_get(Console->InputView));
	stringmap_insert(Console->Globals, "LogView", ml_gir_instance_get(Console->LogView));

	stringmap_insert(Console->Globals, "debug", interactive_debugger(
		(void *)console_debug_enter,
		(void *)console_debug_exit,
		(void *)console_log,
		Console,
		(ml_getter_t)console_global_get,
		Console
	));

	return Console;
}
