/*  Media Library - A XMMS/Beep-Media-Player plugin
 *  Copyright (C) 2002 Paul Fleischer <proguy@proguy.dk>
 *        and (C) 2004 Alexander Kaiser <medialibrary@alexkaiser.de>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef GUI_PLAYLIST_H
#define GUI_PLAYLIST_H

#include "common.h"
#include "command_queue.h"
#include "playlist.h"

#define NEW_PLAYLIST

/**** VARIABLES ****/
extern GtkWidget *library_window;
extern GtkWidget *file_list;
extern GtkWidget *table;
extern GtkWidget *add_button;
extern GtkWidget *add_dir_button;
extern GtkWidget *layout_box;
extern GtkWidget *filew;
extern GtkWidget *dir_browser;
extern GtkWidget *search_field;
extern GtkWidget *search_button;
extern GtkWidget *save_list_button;
extern GtkWidget *load_list_button;
extern GtkWidget *list_dialog;
extern GtkWidget *reload_button;

extern gint selected_row;
extern gint row_count;

extern gboolean updating;
extern pthread_mutex_t updating_mutex;


/**** FUNCTIONS ****/

/** Event handlers **/
void save_list_button_clicked( GtkWidget *widget, gpointer data);
void load_list_button_clicked( GtkWidget *widget, gpointer data);
void cancel_list_dialog(gpointer data);
void file_list_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);

void close_file_dialog(GtkWidget *widget, gpointer data);
void accept_file_dialog(GtkWidget *widget, gpointer data);
void add_dir_button_clicked( GtkWidget *widget, gpointer data);

void remove_selected(void);
void load_list_init(gpointer data);
void save_list_init(gpointer data);
void list_add_file_dialog( GtkWidget *widget, gpointer data);

void dir_browser_handler( gchar *dir );
void add_selected_to_playlist(void);
void add_selected_to_playlist_and_play(void);
gboolean file_list_pressed(GtkWidget *widget, GdkEventButton *event, gpointer data);
void search_modify(GtkEditable *editable, gpointer data);
void file_list_unselect(GtkList *clist, gint row, gint column, 
			GdkEventButton *event, gpointer data);
void file_list_select(GtkList *clist, gint row, gint column, 
		      GdkEventButton *event, gpointer data);
void add_to_playlist(gint row);
void add_to_playlist_and_play(gint row);
void free_row_data(gpointer data);
void update_list(void);

/* The function that construct the whole thing*/
void init_gui_playlist(void);

/* The desctructor of it all ;) */
void done_gui_playlist(void);

#endif
