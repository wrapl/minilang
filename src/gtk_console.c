#include "gtk_console.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtksourceview/gtksource.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <girepository.h>
#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include "ml_compiler.h"
#include <sys/stat.h>

#include "ml_gir.h"
#include "ml_runtime.h"
#include "ml_bytecode.h"
#include "ml_debugger.h"
#include "ml_object.h"
#include "gtk_console_completion.h"

#undef ML_CATEGORY
#define ML_CATEGORY "gtk_console"

#define MAX_HISTORY 128

struct gtk_console_t {
	ml_state_t Base;
	const char *Name;
	GtkWidget *Window, *LogScrolled, *LogView, *InputView;
	GtkWidget *DebugButtons, *SourceView, *FrameView, *ThreadView, *Paned;
	GtkListStore *ThreadStore;
	GtkTreeStore *FrameStore;
	GtkNotebook *Notebook;
	GtkSourceLanguage *Language;
	GtkSourceStyleScheme *StyleScheme;
	GtkLabel *MemoryBar;
	GtkTextTag *OutputTag, *ResultTag, *ErrorTag, *BinaryTag;
	GtkTextMark *EndMark;
	GtkSourceBuffer *SourceBuffer;
	ml_getter_t ParentGetter;
	void *ParentGlobals;
	interactive_debugger_t *Debugger;
	const char *ConfigPath;
	const char *FontName;
	GKeyFile *Config;
	PangoFontDescription *FontDescription;
	ml_parser_t *Parser;
	ml_compiler_t *Compiler;
	char *History[MAX_HISTORY];
	int HistoryIndex, HistoryEnd;
	stringmap_t Globals[1];
	stringmap_t OpenFiles[1];
	stringmap_t Cycles[1];
	stringmap_t Combos[1];
	gint WindowSize[2];
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

static ml_value_t *console_global_get(gtk_console_t *Console, const char *Name, const char *Source, int Line, int Eval) {
	if (Console->Debugger) {
		ml_value_t *Value = interactive_debugger_get(Console->Debugger, Name);
		if (Value) return Value;
	}
	ml_value_t *Value = stringmap_search(Console->Globals, Name);
	if (Value) return Value;
	return (Console->ParentGetter)(Console->ParentGlobals, Name, Source, Line, Eval);
}

void gtk_console_log(gtk_console_t *Console, ml_value_t *Value) {
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	if (ml_is_error(Value)) {
		char *Buffer;
		int Length = GC_asprintf(&Buffer, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ErrorTag, NULL);
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			Length = GC_asprintf(&Buffer, "\t%s:%d\n", Source.Name, Source.Line);
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ErrorTag, NULL);
		}
	} else {
		ml_value_t *String = ml_simple_inline(MLStringT, 1, Value);
		if (ml_is(String, MLStringT)) {
			const char *Buffer = ml_string_value(String);
			int Length = ml_string_length(String);
			if (Length > 10240) {
				char Text[32];
				int TextLength = sprintf(Text, "<%d bytes>", Length);
				gtk_text_buffer_insert_with_tags(LogBuffer, End, Text, TextLength, Console->ResultTag, NULL);
			} else if (g_utf8_validate(Buffer, Length, NULL)) {
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
			int Length = GC_asprintf(&Buffer, "<%s>\n", ml_typeof(Value)->Name);
			gtk_text_buffer_insert_with_tags(LogBuffer, End, Buffer, Length, Console->ResultTag, NULL);
		}
	}
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(Console->LogView), Console->EndMark);
}

ML_TYPE(ConsoleT, (), "console");
//!internal

static __attribute__ ((noinline)) void console_new_line(gtk_console_t *Console) {
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_text_buffer_insert(LogBuffer, End, "\n", 1);
}

static void ml_console_repl_run(gtk_console_t *Console, ml_value_t *Result) {
	if (Result == MLEndOfInput) {
		gtk_widget_grab_focus(Console->InputView);
		return;
	}
	gtk_console_log(Console, Result);
	console_new_line(Console);
	if (ml_is_error(Result)) {
		gtk_widget_grab_focus(Console->InputView);
		return;
	}
	return ml_command_evaluate((ml_state_t *)Console, Console->Parser, Console->Compiler);
}

static void console_step_in(GtkWidget *Button, gtk_console_t *Console) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, "step_in()");
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

static void console_step_over(GtkWidget *Button, gtk_console_t *Console) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, "step_over()");
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

static void console_step_out(GtkWidget *Button, gtk_console_t *Console) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, "step_out()");
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

static void console_continue(GtkWidget *Button, gtk_console_t *Console) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, "continue()");
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

static void console_continue_all(GtkWidget *Button, gtk_console_t *Console) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, "continue_all()");
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

void gtk_console_evaluate(gtk_console_t *Console, const char *Text) {
	ml_parser_t *Parser = Console->Parser;
	ml_compiler_t *Compiler = Console->Compiler;
	ml_parser_reset(Parser);
	ml_parser_input(Parser, Text);
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

static void console_submit(GtkWidget *Button, gtk_console_t *Console) {
	GtkTextBuffer *InputBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView));
	GtkTextIter InputStart[1], InputEnd[1];
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(InputBuffer), FALSE);
	gtk_text_buffer_get_bounds(InputBuffer, InputStart, InputEnd);
	const char *Text = gtk_text_buffer_get_text(InputBuffer, InputStart, InputEnd, FALSE);

	int HistoryEnd = Console->HistoryEnd;
	Console->History[HistoryEnd] = GC_strdup(Text);
	Console->HistoryIndex = Console->HistoryEnd = (HistoryEnd + 1) % MAX_HISTORY;

	GtkTextIter End[1];

	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_source_buffer_create_source_mark(GTK_SOURCE_BUFFER(LogBuffer), NULL, "result", End);
	gtk_text_buffer_insert_range(LogBuffer, End, InputStart, InputEnd);
	gtk_text_buffer_insert(LogBuffer, End, "\n", -1);
	gtk_text_buffer_set_text(InputBuffer, "", 0);

	GtkTextBuffer *SourceBuffer = GTK_TEXT_BUFFER(Console->SourceBuffer);
	gtk_text_buffer_get_end_iter(SourceBuffer, End);
	gtk_text_buffer_insert(SourceBuffer, End, Text, -1);
	gtk_text_buffer_insert(SourceBuffer, End, "\n", -1);
gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(InputBuffer), TRUE);


	//GtkTextIter End[1];
	//GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_text_buffer_insert(LogBuffer, End, "\n", 1);
	gtk_console_evaluate(Console, Text);
}

typedef struct {
	gtk_console_t *Console;
	ml_value_t *Name;
	GtkWidget *View;
} console_open_file_t;

static void console_breakpoint_toggle(GtkSourceView *View, GtkTextIter *Iter, GdkEvent *Event, console_open_file_t *OpenFile) {
	GtkTextBuffer *Buffer = gtk_text_iter_get_buffer(Iter);
	GSList *Marks = gtk_source_buffer_get_source_marks_at_iter(GTK_SOURCE_BUFFER(Buffer), Iter, "breakpoint");
	interactive_debugger_t *Debugger = OpenFile->Console->Debugger;
	ml_value_t *BreakpointFn = NULL;
	if (Marks) {
		GtkTextMark *Mark = GTK_TEXT_MARK(Marks->data);
		gtk_text_buffer_delete_mark(Buffer, Mark);
		if (Debugger) BreakpointFn = interactive_debugger_get(Debugger, "breakpoint_clear");
		g_slist_free(Marks);
	} else {
		gtk_source_buffer_create_source_mark(GTK_SOURCE_BUFFER(Buffer), NULL, "breakpoint", Iter);
		if (Debugger) BreakpointFn = interactive_debugger_get(Debugger, "breakpoint_set");
	}
	if (BreakpointFn) {
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = OpenFile->Name;
		Args[1] = ml_integer(gtk_text_iter_get_line(Iter) + 1);
		ml_simple_call(BreakpointFn, 2, Args);
	}
}

static GtkWidget *console_open_source(gtk_console_t *Console, const char *SourceName) {
	console_open_file_t **Slot = (console_open_file_t **)stringmap_slot(Console->OpenFiles, SourceName);
	if (!Slot[0]) {
		console_open_file_t *OpenFile = Slot[0] = new(console_open_file_t);
		OpenFile->Console = Console;
		OpenFile->Name = ml_string(SourceName, -1);
		GtkSourceBuffer *Buffer = gtk_source_buffer_new_with_language(Console->Language);
		gtk_source_buffer_set_style_scheme(Buffer, Console->StyleScheme);
		GtkTextIter End[1];
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(Buffer), End);
		FILE *File = fopen(SourceName, "r");
		if (File) {
			char Text[128];
			size_t Length;
			do {
			 Length = fread(Text, 1, 128, File);
			 gtk_text_buffer_insert(GTK_TEXT_BUFFER(Buffer), End, Text, Length);
			} while (Length == 128);
			fclose(File);
		}
		GtkWidget *View = gtk_source_view_new_with_buffer(Buffer);
		OpenFile->View = View;
		GtkWidget *Scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(Scrolled), View);
		gtk_text_view_set_monospace(GTK_TEXT_VIEW(View), TRUE);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(View), FALSE);

		gtk_widget_override_font(View, Console->FontDescription);
		gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(View), 4);
		gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(View), TRUE);
		gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(View), TRUE);
		gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(View), TRUE);

		static GtkSourceMarkAttributes *BreakpointMarkAttributes = NULL;
		if (!BreakpointMarkAttributes) {
			BreakpointMarkAttributes = gtk_source_mark_attributes_new();
			gtk_source_mark_attributes_set_icon_name(BreakpointMarkAttributes, "media-record");
		}
		gtk_source_view_set_mark_attributes(GTK_SOURCE_VIEW(View), "breakpoint", BreakpointMarkAttributes, 0);

		g_signal_connect(G_OBJECT(View), "line-mark-activated", G_CALLBACK(console_breakpoint_toggle), OpenFile);

		gtk_notebook_append_page(Console->Notebook, Scrolled, gtk_label_new(SourceName));
		gtk_widget_show_all(GTK_WIDGET(Console->Notebook));
	}
	return Slot[0]->View;
}

static int console_debug_set_breakpoints(const char *SourceName, console_open_file_t *OpenFile, ml_value_t *BreakpointSet) {
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = OpenFile->Name;
	GtkTextBuffer *Buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(OpenFile->View));
	GtkTextIter Iter[1];
	gtk_text_buffer_get_start_iter(Buffer, Iter);
	while (gtk_source_buffer_forward_iter_to_source_mark(GTK_SOURCE_BUFFER(Buffer), Iter, "breakpoint")) {
		Args[1] = ml_integer(gtk_text_iter_get_line(Iter) + 1);
		ml_simple_call(BreakpointSet, 2, Args);
	}
	return 0;
}

static void console_show_value(GtkTreeStore *Store, GtkTreeIter *Iter, const char *Name, ml_value_t *Value) {
	typeof(console_show_value) *function = ml_typed_fn_get(ml_typeof(Value), console_show_value);
	if (function) return function(Store, Iter, Name, Value);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_simple_append(Buffer, Value);
	char *Display;
	if (Buffer->Length < 64) {
		Display = ml_stringbuffer_get_string(Buffer);
	} else {
		Display = snew(68);
		memcpy(Display, Buffer->Head->Chars, 64);
		strcpy(Display + 64, "...");
	}
	gtk_tree_store_insert_with_values(Store, NULL, Iter, -1, 0, Name, 1, Display, -1);
}

static void ML_TYPED_FN(console_show_value, MLListT, GtkTreeStore *Store, GtkTreeIter *Iter, const char *Name, ml_value_t *Value) {
	GtkTreeIter Child[1];
	char *Display;
	GC_asprintf(&Display, "list[%d]", ml_list_length(Value));
	gtk_tree_store_insert_with_values(Store, Child, Iter, -1, 0, Name, 1, Display, -1);
	int Index = 0;
	ML_LIST_FOREACH(Value, Iter) {
		if (++Index > 20) break;
		GC_asprintf(&Display, "[%d]", Index);
		console_show_value(Store, Child, Display, Iter->Value);
	}
}

static void ML_TYPED_FN(console_show_value, MLMapT, GtkTreeStore *Store, GtkTreeIter *Iter, const char *Name, ml_value_t *Value) {
	GtkTreeIter Child[1];
	char *Display;
	GC_asprintf(&Display, "map[%d]", ml_map_size(Value));
	gtk_tree_store_insert_with_values(Store, Child, Iter, -1, 0, Name, 1, Display, -1);
	int Index = 0;
	ML_MAP_FOREACH(Value, Iter) {
		if (++Index > 20) break;
		GC_asprintf(&Display, "[%d]", Index);
		GtkTreeIter Child2[1];
		gtk_tree_store_insert_with_values(Store, Child2, Child, -1, 0, Display, -1);
		console_show_value(Store, Child2, "key", Iter->Key);
		console_show_value(Store, Child2, "value", Iter->Value);
	}
}

typedef struct {
	GtkTreeStore *Store;
	GtkTreeIter *Child;
} console_show_field_t;

static int console_show_field(const char *Name, ml_value_t *Value, console_show_field_t *Show) {
	console_show_value(Show->Store, Show->Child, Name, Value);
	return 0;
}

static void ML_TYPED_FN(console_show_value, MLObjectT, GtkTreeStore *Store, GtkTreeIter *Iter, const char *Name, ml_value_t *Value) {
	ml_type_t *Class = ml_typeof(Value);
	GtkTreeIter Child[1];
	gtk_tree_store_insert_with_values(Store, Child, Iter, -1, 0, Name, 1, ml_type_name(Class), -1);
	console_show_field_t Show[1] = {{Store, Child}};
	ml_object_foreach(Value, Show, (void *)console_show_field);
}

static void console_show_thread(gtk_console_t *Console, const char *SourceName, int Line) {
	GtkWidget *SourceView;
	if (!strcmp(SourceName, Console->Name)) {
		SourceView = Console->SourceView;
	} else {
		SourceView = console_open_source(Console, SourceName);
	}
	ml_value_t *BreakpointSet = interactive_debugger_get(Console->Debugger, "breakpoint_set");
	stringmap_foreach(Console->OpenFiles, BreakpointSet, (void *)console_debug_set_breakpoints);
	int PageNum = gtk_notebook_page_num(Console->Notebook, gtk_widget_get_parent(SourceView));
	gtk_notebook_set_current_page(Console->Notebook, PageNum);
	GtkTextBuffer *Buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(SourceView));
	GtkTextIter LineBeg[1], LineEnd[1];
	gtk_text_buffer_get_iter_at_line(Buffer, LineBeg, Line - 1);
	gtk_text_buffer_get_iter_at_line(Buffer, LineEnd, Line);
	//gtk_text_buffer_apply_tag(Buffer, PausedTag, LineBeg, LineEnd);
	gtk_text_buffer_place_cursor(Buffer, LineBeg);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(SourceView), LineBeg, 0.0, FALSE, 0.0, 0.0);
	gtk_tree_store_clear(Console->FrameStore);
	ml_value_t *FramesGet = interactive_debugger_get(Console->Debugger, "frames");
	ml_value_t *LocalsGet = interactive_debugger_get(Console->Debugger, "locals");
	ml_value_t *Frames = ml_simple_call(FramesGet, 0, NULL);
	ml_value_t **Args = ml_alloc_args(1);
	int Depth = 0;
	ML_LIST_FOREACH(Frames, Iter1) {
		char *Source;
		GC_asprintf(&Source, "%s:%ld",
			ml_string_value(ml_tuple_get(Iter1->Value, 1)),
			ml_integer_value(ml_tuple_get(Iter1->Value, 2))
		);
		GtkTreeIter TreeIter[1];
		gtk_tree_store_insert_with_values(Console->FrameStore, TreeIter, NULL, -1, 0, Source, -1);
		Args[0] = ml_integer(Depth++);
		ml_value_t *Locals = ml_simple_call(LocalsGet, 1, Args);
		ML_MAP_FOREACH(Locals, Iter2) {
			console_show_value(Console->FrameStore, TreeIter, ml_string_value(Iter2->Key), ml_deref(Iter2->Value));
		}
	}
	gtk_tree_view_expand_all(GTK_TREE_VIEW(Console->FrameView));
}

static void console_thread_activated(GtkTreeView *ThreadView, GtkTreePath *Path, GtkTreeViewColumn *Column, gtk_console_t *Console) {
	const char *SourceName;
	int Index, Line;
	GtkTreeIter Iter[1];
	gtk_tree_model_get_iter(GTK_TREE_MODEL(Console->ThreadStore), Iter, Path);
	gtk_tree_model_get(GTK_TREE_MODEL(Console->ThreadStore), Iter, 0, &Index, 1, &SourceName, 2, &Line, -1);
	ml_value_t *ThreadSet = interactive_debugger_get(Console->Debugger, "thread");
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = ml_integer(Index);
	ml_simple_call(ThreadSet, 1, Args);
	console_show_thread(Console, SourceName, Line);
}

static void console_debug_enter(gtk_console_t *Console, interactive_debugger_t *Debugger, ml_source_t Source, int Index) {
	gtk_widget_show(Console->DebugButtons);
	Console->Debugger = Debugger;
	gtk_console_printf(Console, "Debug break [%d]: %s:%d\n", Index, Source.Name, Source.Line);
	GtkTreeIter Iter[1];
	gtk_list_store_insert_with_values(Console->ThreadStore, Iter, -1, 0, Index, 1, Source.Name, 2, Source.Line, -1);
	GtkTreeSelection *Selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(Console->ThreadView));
	gtk_tree_selection_select_iter(Selection, Iter);
	console_show_thread(Console, Source.Name, Source.Line);
}

static void console_debug_exit(gtk_console_t *Console, interactive_debugger_t *Debugger, ml_state_t *Caller, int Index) {
	GtkTreeIter Iter[1];
	GtkTreeModel *Model = GTK_TREE_MODEL(Console->ThreadStore);
	if (gtk_tree_model_get_iter_first(Model, Iter)) do {
		int Index0;
		gtk_tree_model_get(Model, Iter, 0, &Index0, -1);
		if (Index0 == Index) {
			gtk_list_store_remove(Console->ThreadStore, Iter);
			break;
		}
	} while (gtk_tree_model_iter_next(Model, Iter));
	gtk_tree_store_clear(Console->FrameStore);
	if (!gtk_tree_model_iter_n_children(Model, NULL)) {
		gtk_widget_hide(Console->DebugButtons);
	} else {
		// TODO: Select another thread
		GtkTreeSelection *Selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(Console->ThreadView));
		gtk_tree_selection_unselect_all(Selection);
	}
	return interactive_debugger_resume(Debugger, Index);
}

static void console_clear(GtkWidget *Button, gtk_console_t *Console) {
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	GtkTextIter Start[1], End[1];
	gtk_text_buffer_get_start_iter(LogBuffer, Start);
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	gtk_text_buffer_delete(LogBuffer, Start, End);
}

static void toggle_layout(GtkWidget *Button, gtk_console_t *Console) {
	switch (gtk_orientable_get_orientation(GTK_ORIENTABLE(Console->Paned))) {
	case GTK_ORIENTATION_HORIZONTAL:
		gtk_orientable_set_orientation(GTK_ORIENTABLE(Console->Paned), GTK_ORIENTATION_VERTICAL);
		break;
	case GTK_ORIENTATION_VERTICAL:
		gtk_orientable_set_orientation(GTK_ORIENTABLE(Console->Paned), GTK_ORIENTATION_HORIZONTAL);
		break;
	}
}

static void console_style_changed(GtkSourceStyleSchemeChooser *Widget, GParamSpec *Spec, gtk_console_t *Console) {
	Console->StyleScheme = gtk_source_style_scheme_chooser_get_style_scheme(Widget);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), Console->StyleScheme);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView))), Console->StyleScheme);

	const char *StyleId = gtk_source_style_scheme_get_id(Console->StyleScheme);

	g_key_file_set_string(Console->Config, "gtk-console", "style", StyleId);
	g_key_file_save_to_file(Console->Config, Console->ConfigPath, NULL);
}

static void console_font_changed(GtkFontChooser *Widget, gtk_console_t *Console) {
	gchar *FontName = gtk_font_chooser_get_font(Widget);
	Console->FontName = FontName;
	Console->FontDescription = pango_font_description_from_string(FontName);
	gtk_widget_override_font(Console->InputView, Console->FontDescription);
	gtk_widget_override_font(Console->LogView, Console->FontDescription);

	g_key_file_set_string(Console->Config, "gtk-console", "font", FontName);
	g_key_file_save_to_file(Console->Config, Console->ConfigPath, NULL);
}

static void console_size_allocate(GtkWindow *Window, GdkRectangle *Allocation, gtk_console_t *Console) {
	gint Width, Height;
	gtk_window_get_size(Window, &Width, &Height);
	if (Width != Console->WindowSize[0] || Height != Console->WindowSize[1]) {
		Console->WindowSize[0] = Width;
		Console->WindowSize[1] = Height;
		g_key_file_set_integer_list(Console->Config, "gtk-console", "size", Console->WindowSize, 2);
		g_key_file_save_to_file(Console->Config, Console->ConfigPath, NULL);
	}
}

#ifdef __APPLE__
#define COMMAND_MASK GDK_META_MASK
#else
#define COMMAND_MASK GDK_CONTROL_MASK
#endif

static gboolean console_keypress(GtkWidget *Widget, GdkEventKey *Event, gtk_console_t *Console) {
	switch (Event->keyval) {
	case GDK_KEY_Return:
		Console->NumChars = 0;
		if (Event->state & COMMAND_MASK) {
			console_submit(NULL, Console);
			return TRUE;
		}
		break;
	case GDK_KEY_Up:
		Console->NumChars = 0;
		if (Event->state & COMMAND_MASK) {
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
		if (Event->state & COMMAND_MASK) {
			int HistoryIndex = (Console->HistoryIndex + 1) % MAX_HISTORY;
			if (Console->History[HistoryIndex]) {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), Console->History[HistoryIndex], -1);
				Console->HistoryIndex = HistoryIndex;
			} else {
				gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)), "", 0);
				Console->HistoryIndex = Console->HistoryEnd;
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

void gtk_console_show(gtk_console_t *Console, GtkWindow *Parent) {
	gtk_window_set_transient_for(GTK_WINDOW(Console->Window), Parent);
	gtk_widget_show_all(Console->Window);
	gtk_widget_hide(Console->DebugButtons);
	gtk_widget_grab_focus(Console->InputView);
}

int gtk_console_append(gtk_console_t *Console, const char *Buffer, int Length) {
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
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(Console->LogView), Console->EndMark);
	while (gtk_events_pending()) gtk_main_iteration();
	return 0;
}

ml_value_t *gtk_console_print(gtk_console_t *Console, int Count, ml_value_t **Args) {
	GtkTextIter End[1];
	GtkTextBuffer *LogBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView));
	gtk_text_buffer_get_end_iter(LogBuffer, End);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_simple_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
	}
	ml_stringbuffer_foreach(Buffer, Console, (void *)gtk_console_append);
	while (gtk_events_pending()) gtk_main_iteration();
	return MLNil;
}

void gtk_console_printf(gtk_console_t *Console, const char *Format, ...) {
	char *Buffer;
	va_list Args;
	va_start(Args, Format);
	int Length = vasprintf(&Buffer, Format, Args);
	va_end(Args);
	gtk_console_append(Console, Buffer, Length);
	free(Buffer);
}

static ml_value_t *console_set_font(gtk_console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	Console->FontDescription = pango_font_description_new();
	pango_font_description_set_family(Console->FontDescription, ml_string_value(Args[0]));
	pango_font_description_set_size(Console->FontDescription, PANGO_SCALE * ml_integer_value(Args[1]));
	gtk_widget_override_font(Console->InputView, Console->FontDescription);
	gtk_widget_override_font(Console->LogView, Console->FontDescription);
	return MLNil;
}

static ml_value_t *console_set_style(gtk_console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	GtkSourceStyleSchemeManager *StyleManager = gtk_source_style_scheme_manager_get_default();
	Console->StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, ml_string_value(Args[0]));
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView))), Console->StyleScheme);
	gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->LogView))), Console->StyleScheme);
	return MLNil;
}

static ml_value_t *console_add_cycle(gtk_console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	for (int I = 1; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		stringmap_insert(Console->Cycles, ml_string_value(Args[I - 1]), (void *)ml_string_value(Args[I]));
	}
	stringmap_insert(Console->Cycles, ml_string_value(Args[Count - 1]), (void *)ml_string_value(Args[0]));
	return MLNil;
}

static ml_value_t *console_add_combo(gtk_console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	stringmap_insert(Console->Combos, ml_string_value(Args[0]), (void *)ml_string_value(Args[1]));
	stringmap_insert(Console->Cycles, ml_string_value(Args[1]), (void *)ml_string_value(Args[0]));
	return MLNil;
}

static void console_included_run(ml_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	return ml_call(Caller, Value, 0, NULL);
}

static void console_include_fnx(ml_state_t *Caller, gtk_console_t *Console, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->Context = Caller->Context;
	State->run = console_included_run;
	return ml_load_file(State, (ml_getter_t)ml_compiler_lookup, Console->Compiler, ml_string_value(Args[0]), NULL);
}

static gboolean console_update_status(gtk_console_t *Console) {
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

gtk_console_t *gtk_console(ml_context_t *Context, ml_getter_t GlobalGet, void *Globals) {
	gtk_init(0, 0);
	gtk_console_t *Console = new(gtk_console_t);
	Console->Base.Type = ConsoleT;
	Console->Base.run = (ml_state_fn)ml_console_repl_run;
	Console->Base.Context = Context;
	Console->Name = strdup("<console>");
	Console->ParentGetter = GlobalGet;
	Console->ParentGlobals = Globals;
	Console->HistoryIndex = 0;
	Console->HistoryEnd = 0;
	Console->Parser = ml_parser(NULL, NULL);
	Console->Compiler = ml_compiler((ml_getter_t)console_global_get, Console);
	ml_parser_source(Console->Parser, (ml_source_t){Console->Name, 0});
	Console->Notebook = GTK_NOTEBOOK(gtk_notebook_new());

	GC_asprintf((char **)&Console->ConfigPath, "%s/%s", g_get_user_config_dir(), "minilang.conf");
	Console->Config = g_key_file_new();
	g_key_file_load_from_file(Console->Config, Console->ConfigPath, G_KEY_FILE_NONE, NULL);

	GtkSourceLanguageManager *LanguageManager = gtk_source_language_manager_get_default();
	Console->Language = gtk_source_language_manager_get_language(LanguageManager, "minilang");

	GtkSourceBuffer *InputBuffer = gtk_source_buffer_new_with_language(Console->Language);
	Console->InputView = gtk_source_view_new_with_buffer(InputBuffer);
	GtkSourceCompletion *Completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(Console->InputView));
	GtkSourceCompletionProvider *Provider = gtk_console_completion_provider(Console->Compiler);
	gtk_source_completion_add_provider(Completion, Provider, NULL);
	GtkTextTagTable *TagTable = gtk_text_buffer_get_tag_table(GTK_TEXT_BUFFER(InputBuffer));
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
	Console->SourceBuffer = gtk_source_buffer_new_with_language(Console->Language);

	Console->LogScrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(Console->LogScrolled), Console->LogView);

	Console->ThreadStore = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(Console->ThreadStore), 0, GTK_SORT_ASCENDING);
	GtkWidget *ThreadView = Console->ThreadView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(Console->ThreadStore));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ThreadView), -1, "Thread", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ThreadView), -1, "Source", gtk_cell_renderer_text_new(), "text", 1, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(ThreadView), -1, "Line", gtk_cell_renderer_text_new(), "text", 2, NULL);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(ThreadView), TRUE);
	g_signal_connect(G_OBJECT(ThreadView), "row-activated", G_CALLBACK(console_thread_activated), Console);

	Console->FrameStore = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkWidget *FrameView = Console->FrameView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(Console->FrameStore));
	//gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(FrameView), 20);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(FrameView), -1, "Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(FrameView), -1, "Value", gtk_cell_renderer_text_new(), "text", 1, NULL);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(FrameView), TRUE);

	GtkWidget *Debugging = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	GtkWidget *Scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(Scrolled), ThreadView);
	gtk_paned_add1(GTK_PANED(Debugging), Scrolled);
	Scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(Scrolled), FrameView);
	gtk_paned_add2(GTK_PANED(Debugging), Scrolled);

	gtk_paned_set_position(GTK_PANED(Debugging), 100);

	GtkWidget *OutputPane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_pack1(GTK_PANED(OutputPane), GTK_WIDGET(Console->Notebook), TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(OutputPane), Debugging, TRUE, TRUE);
	gtk_paned_set_position(GTK_PANED(OutputPane), 200);

	Console->Paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	GtkWidget *InputPanel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *DebugButtons = Console->DebugButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	GtkWidget *StepInButton = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(StepInButton), "In");
	gtk_box_pack_start(GTK_BOX(DebugButtons), StepInButton, FALSE, FALSE, 2);
	GtkWidget *StepOverButton = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(StepOverButton), "Over");
	gtk_box_pack_start(GTK_BOX(DebugButtons), StepOverButton, FALSE, FALSE, 2);
	GtkWidget *StepOutButton = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(StepOutButton), "Out");
	gtk_box_pack_start(GTK_BOX(DebugButtons), StepOutButton, FALSE, FALSE, 2);
	GtkWidget *ContinueButton = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(ContinueButton), "Run");
	gtk_box_pack_start(GTK_BOX(DebugButtons), ContinueButton, FALSE, FALSE, 2);
	GtkWidget *ContinueAllButton = gtk_button_new();
	gtk_button_set_label(GTK_BUTTON(ContinueAllButton), "Run All");
	gtk_box_pack_start(GTK_BOX(DebugButtons), ContinueAllButton, FALSE, FALSE, 2);
	g_signal_connect(G_OBJECT(StepInButton), "clicked", G_CALLBACK(console_step_in), Console);
	g_signal_connect(G_OBJECT(StepOverButton), "clicked", G_CALLBACK(console_step_over), Console);
	g_signal_connect(G_OBJECT(StepOutButton), "clicked", G_CALLBACK(console_step_out), Console);
	g_signal_connect(G_OBJECT(ContinueButton), "clicked", G_CALLBACK(console_continue), Console);
	g_signal_connect(G_OBJECT(ContinueAllButton), "clicked", G_CALLBACK(console_continue_all), Console);
	gtk_box_pack_start(GTK_BOX(InputPanel), DebugButtons, FALSE, FALSE, 2);
	GtkWidget *SubmitButton = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(SubmitButton), gtk_image_new_from_icon_name("go-jump-symbolic", GTK_ICON_SIZE_BUTTON));
	GtkWidget *ClearButton = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(ClearButton), gtk_image_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON));
	gtk_box_pack_start(GTK_BOX(InputPanel), Console->InputView, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(InputPanel), SubmitButton, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(InputPanel), ClearButton, FALSE, FALSE, 2);

	GtkWidget *StyleCombo = gtk_source_style_scheme_chooser_button_new();

	g_signal_connect(G_OBJECT(StyleCombo), "notify::style-scheme", G_CALLBACK(console_style_changed), Console);

	GtkWidget *FontButton = gtk_font_button_new();
	g_signal_connect(G_OBJECT(FontButton), "font-set", G_CALLBACK(console_font_changed), Console);

	GtkWidget *SourceView = Console->SourceView = gtk_source_view_new_with_buffer(Console->SourceBuffer);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(SourceView), TRUE);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(SourceView), 4);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(SourceView), TRUE);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(SourceView), TRUE);
	GtkWidget *SourceScrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(SourceScrolled), SourceView);
	gtk_notebook_append_page(Console->Notebook, SourceScrolled, gtk_label_new("<console>"));

	GtkWidget *InputFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(InputFrame), InputPanel);
	g_signal_connect(G_OBJECT(Console->InputView), "key-press-event", G_CALLBACK(console_keypress), Console);
	g_signal_connect(G_OBJECT(SubmitButton), "clicked", G_CALLBACK(console_submit), Console);
	g_signal_connect(G_OBJECT(ClearButton), "clicked", G_CALLBACK(console_clear), Console);
	Console->Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon_name(GTK_WINDOW(Console->Window), "face-smile");

	GtkWidget *ReplBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(ReplBox), Console->LogScrolled, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(ReplBox), InputFrame, FALSE, TRUE, 2);

	gtk_paned_add1(GTK_PANED(Console->Paned), ReplBox);
	gtk_paned_add2(GTK_PANED(Console->Paned), OutputPane);
	gtk_paned_set_position(GTK_PANED(Console->Paned), 600);


	GtkWidget *LayoutButton = gtk_button_new_with_label("Layout");
	g_signal_connect(G_OBJECT(LayoutButton), "clicked", G_CALLBACK(toggle_layout), Console);

	GtkWidget *MenuButton = gtk_menu_button_new();
	GtkWidget *ActionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(ActionsBox), StyleCombo, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ActionsBox), FontButton, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ActionsBox), LayoutButton, FALSE, TRUE, 0);
	GtkWidget *ActionsPopover = gtk_popover_new(MenuButton);
	gtk_container_add(GTK_CONTAINER(ActionsPopover), ActionsBox);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(MenuButton), ActionsPopover);
	gtk_widget_show_all(ActionsBox);


	GtkWidget *HeaderBar = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(HeaderBar), "Minilang");
	gtk_header_bar_set_has_subtitle(GTK_HEADER_BAR(HeaderBar), FALSE);
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(HeaderBar), TRUE);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(HeaderBar), MenuButton);
	gtk_window_set_titlebar(GTK_WINDOW(Console->Window), HeaderBar);

	GtkWidget *MemoryBar = gtk_label_new("");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(HeaderBar), MemoryBar);

	Console->MemoryBar = GTK_LABEL(MemoryBar);

	gtk_container_add(GTK_CONTAINER(Console->Window), Console->Paned);
	if (g_key_file_has_key(Console->Config, "gtk-console", "size", NULL)) {
		gsize Length = 0;
		gint *Size = g_key_file_get_integer_list(Console->Config, "gtk-console", "size", &Length, NULL);
		if (Length == 2) {
			Console->WindowSize[0] = Size[0];
			Console->WindowSize[1] = Size[1];
		}
	} else {
		Console->WindowSize[0] = 640;
		Console->WindowSize[1] = 480;
	}
	gtk_window_set_default_size(GTK_WINDOW(Console->Window), Console->WindowSize[0], Console->WindowSize[1]);
	g_signal_connect(G_OBJECT(Console->Window), "size-allocate", G_CALLBACK(console_size_allocate), Console);
	g_signal_connect(G_OBJECT(Console->Window), "delete-event", G_CALLBACK(ml_gir_loop_quit), NULL);

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
	Console->FontDescription = pango_font_description_from_string(Console->FontName);
	gtk_widget_override_font(Console->InputView, Console->FontDescription);
	gtk_widget_override_font(Console->LogView, Console->FontDescription);
	gtk_widget_override_font(SourceView, Console->FontDescription);
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(FontButton), Console->FontName);

	if (g_key_file_has_key(Console->Config, "gtk-console", "style", NULL)) {
		const char *StyleId = g_key_file_get_string(Console->Config, "gtk-console", "style", NULL);
		Console->StyleScheme = gtk_source_style_scheme_manager_get_scheme(StyleManager, StyleId);
		GtkSourceBuffer *InputBuffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(Console->InputView)));
		gtk_source_buffer_set_style_scheme(InputBuffer, Console->StyleScheme);
		gtk_source_buffer_set_style_scheme(LogBuffer, Console->StyleScheme);
		gtk_source_buffer_set_style_scheme(Console->SourceBuffer, Console->StyleScheme);
		gtk_source_style_scheme_chooser_set_style_scheme(GTK_SOURCE_STYLE_SCHEME_CHOOSER(StyleCombo), Console->StyleScheme);
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
	g_irepository_require(NULL, "Gtk", "3.0", 0, &Error);
	g_irepository_require(NULL, "GtkSource", "4", 0, &Error);
	stringmap_insert(Console->Globals, "print", ml_cfunction2(Console, (void *)gtk_console_print, ML_CATEGORY, __LINE__));
	stringmap_insert(Console->Globals, "Console", ml_gir_instance_get(Console->Window, NULL));
	stringmap_insert(Console->Globals, "InputView", ml_gir_instance_get(Console->InputView, NULL));
	stringmap_insert(Console->Globals, "LogView", ml_gir_instance_get(Console->LogView, NULL));
	stringmap_insert(Console->Globals, "idebug", interactive_debugger(
		(void *)console_debug_enter,
		(void *)console_debug_exit,
		(void *)gtk_console_log,
		Console,
		GlobalGet,
		Globals
	));

	return Console;
}

void gtk_console_load_file(gtk_console_t *Console, const char *FileName, ml_value_t *Args) {
	ml_call_state_t *State = ml_call_state((ml_state_t *)Console, 1);
	State->Args[0] = Args;
	ml_load_file((ml_state_t *)State, (void *)console_global_get, Console, FileName, NULL);
}

void gtk_console_init() {
#include "gtk_console_init.c"
}
