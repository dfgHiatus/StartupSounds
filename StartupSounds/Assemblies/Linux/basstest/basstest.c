/*
	BASS simple playback test
	Copyright (c) 1999-2021 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "bass.h"

#define UIFILE "basstest.ui"
GtkBuilder *builder;

GtkWidget *win;
GtkWidget *filesel;

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

#define GetWidget(id) GTK_WIDGET(gtk_builder_get_object(builder,id))

void AddListEntry(const char *list, DWORD handle, const char *text)
{
	GtkTreeView *tree = GTK_TREE_VIEW(GetWidget(list));
	GtkListStore *tm = GTK_LIST_STORE(gtk_tree_view_get_model(tree));
	GtkTreeIter it;
	gtk_list_store_append(tm, &it);
	gtk_list_store_set(tm, &it, 0, text, 1, handle, -1);
}

DWORD GetSelectedHandle(int mode)
{
	DWORD handle;
	char list[10];
	GtkTreeSelection *ts;
	GtkTreeModel *tm;
	GtkTreeIter it;
	sprintf(list, "list%d", mode);
	ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GetWidget(list)));
	if (!gtk_tree_selection_get_selected(ts, &tm, &it)) return 0;
	gtk_tree_model_get(tm, &it, 1, &handle, -1);
	return handle;
}

void WindowDestroy(GtkObject *obj, gpointer data)
{
	gtk_main_quit();
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data, info->filename, 0, NULL, 0);
}

void AddClicked(GtkButton *obj, gpointer data)
{
	int resp = gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp == GTK_RESPONSE_ACCEPT) {
		char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
		int mode = atoi(objname + 3); // get mode from button name ("addX")
		switch (mode) {
			case 1: // add a stream
				{
					HSTREAM handle = BASS_StreamCreateFile(FALSE, file, 0, 0, BASS_SAMPLE_FLOAT);
					if (handle)
						AddListEntry("list1", handle, strrchr(file, '/') + 1);
					else
						Error("Can't stream the file");
				}
				break;
			case 2: // add a MOD music
				{
					HMUSIC handle = BASS_MusicLoad(FALSE, file, 0, 0, BASS_MUSIC_RAMP | BASS_SAMPLE_FLOAT, 1);
					if (handle)
						AddListEntry("list2", handle, strrchr(file, '/') + 1);
					else
						Error("Can't load the file");
				}
				break;
			case 3: // add a sample
				{
					// give the sample a max of 3 simultaneous playbacks using position as override decider
					HSAMPLE sam = BASS_SampleLoad(FALSE, file, 0, 0, 3, BASS_SAMPLE_OVER_POS);
					if (sam)
						AddListEntry("list3", sam, strrchr(file, '/') + 1);
					else
						Error("Can't load the file");
				}
				break;
		}
		g_free(file);
	}
}

void RemoveClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int mode = atoi(objname + 6); // get mode from button name ("removeX")
	char list[10];
	sprintf(list, "list%d", mode); // associated list name
	GtkTreeSelection *ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GetWidget(list)));
	GtkTreeModel *tm;
	GtkTreeIter it;
	if (gtk_tree_selection_get_selected(ts, &tm, &it)) {
		DWORD handle;
		gtk_tree_model_get(tm, &it, 1, &handle, -1);
		switch (mode) {
			case 1:
				BASS_StreamFree(handle); // free stream
				break;
			case 2:
				BASS_MusicFree(handle); // free MOD music
				break;
			case 3:
				BASS_SampleFree(handle); // free sample
				break;
		}
		gtk_list_store_remove(GTK_LIST_STORE(tm), &it); // remove list entry
	}
}

void PlayClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int mode = atoi(objname + 4); // get mode from button name ("playX")
	DWORD handle = GetSelectedHandle(mode); // get selected stream/music/sample handle
	if (mode == 3) { // sample: get a channel and set volume=50% and pan=random
		handle = BASS_SampleGetChannel(handle, FALSE);
		BASS_ChannelSetAttribute(handle, BASS_ATTRIB_VOL, 0.5f);
		BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, ((rand() % 201) - 100) / 100.f);
	}
	if (!BASS_ChannelPlay(handle, FALSE)) // play it
		Error("Can't play channel");
}

void StopClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int mode = atoi(objname + 4); // get mode from button name ("playX")
	DWORD handle = GetSelectedHandle(mode); // get selected stream/music/sample handle
	BASS_ChannelStop(handle); // stop it
}

void RestartClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(obj));
	int mode = atoi(objname + 7); // get mode from button name ("restartX")
	DWORD handle = GetSelectedHandle(mode); // get selected stream/music/sample handle
	if (!BASS_ChannelPlay(handle, TRUE)) // restart it
		Error("Can't play channel");
}

void GVolumeChanged(GtkRange *range, gpointer data)
{
	const gchar *objname = gtk_buildable_get_name(GTK_BUILDABLE(range));
	int mode = atoi(objname + 7); // get mode from name ("gvolumeX")
	double value = gtk_range_get_value(range);
	switch (mode) {
		case 1:
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, value * 100); // global stream volume (0-10000)
			break;
		case 2:
			BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, value * 100); // global MOD music volume (0-10000)
			break;
		case 3:
			BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, value * 100); // global sample volume (0-10000)
			break;
	}
}

void VolumeChanged(GtkRange *range, gpointer data)
{
	double value = gtk_range_get_value(range);
	BASS_SetVolume(value / 100.f); // output volume (0-1)
}

void StopAllClicked(GtkButton *obj, gpointer data)
{
	BASS_Pause(); // pause output
}

void ResumeAllClicked(GtkButton *obj, gpointer data)
{
	if (!BASS_Start()) // resume output
		Error("Can't start output");
}

void ThreadsToggled(GtkToggleButton *obj, gpointer data)
{
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, obj->active ? 2 : 1); // set 1 or 2 update threads
}

gboolean TimerProc(gpointer data)
{
	// update the CPU usage display
	char text[16];
	sprintf(text, "CPU%% %.2f", BASS_GetCPU());
	gtk_label_set_text(GTK_LABEL(GetWidget("cpu")), text);
	// update volume slider in case it's been changed outside of the app
	gtk_range_set_value(GTK_RANGE(GetWidget("volume")), BASS_GetVolume() * 100);
	return TRUE;
}

int main(int argc, char* argv[])
{
	regex_t fregex[2];

	gtk_init(&argc, &argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize default output device
	if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// initialize GUI
	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, UIFILE, NULL)) {
		char path[PATH_MAX];
		readlink("/proc/self/exe", path, sizeof(path));
		strcpy(strrchr(path, '/') + 1, UIFILE);
		if (!gtk_builder_add_from_file(builder, path, NULL)) {
			Error("Can't load UI");
			return 0;
		}
	}
	win = GetWidget("window1");
	gtk_builder_connect_signals(builder, NULL);

	{ // setup lists
		int a;
		for (a = 1; a <= 3; a++) {
			char name[8];
			sprintf(name, "list%d", a);
			GtkTreeView *list = GTK_TREE_VIEW(GetWidget(name));
			gtk_tree_view_append_column(list, gtk_tree_view_column_new_with_attributes("", gtk_cell_renderer_text_new(), "text", 0, NULL));
			GtkListStore *liststore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
			gtk_tree_view_set_model(list, GTK_TREE_MODEL(liststore));
			g_object_unref(liststore);
		}
	}

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "Streamable files (wav/aif/mp3/mp2/mp1/ogg)");
		regcomp(&fregex[0], "\\.(mp[1-3]|ogg|wav|aif)$", REG_ICASE | REG_NOSUB | REG_EXTENDED);
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, FileExtensionFilter, &fregex[0], NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "MOD music files (mo3/xm/mod/s3m/it/mtm/umx)");
		regcomp(&fregex[1], "\\.(mo3|xm|mod|s3m|it|umx)$", REG_ICASE | REG_NOSUB | REG_EXTENDED);
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, FileExtensionFilter, &fregex[1], NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
	}

	g_timeout_add(500, TimerProc, NULL);

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex[0]);
	regfree(&fregex[1]);

	BASS_Free();

	return 0;
}
