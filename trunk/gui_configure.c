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

#include "gui_configure.h"

static GtkWidget *configure_win = NULL;
static GtkWidget *auto_add_checkbox;
static GtkWidget *clear_cache_checkbox;
static GtkWidget *dir_browser;
static GtkWidget *folder_list;
static GtkWidget *auto_add_option_frame;
static GtkWidget *change_entry;

static gint folder_count;
static gboolean config_random_song;
static gboolean config_clear_cache;
static gint config_random_song_change;
static gint selected_row;

void set_random_option_frame_status( gboolean s );

static void dir_browser_handler( gchar *dir ) {
  char *text[1];

  if( dir == NULL )
    return;

  text[0] = g_strdup(dir);
  gtk_clist_append(GTK_CLIST(folder_list), text);
  folder_count++;

  dir_browser = NULL;
}

static void folder_list_select_row(GtkCList *list, gint row, gint column,
				   GdkEventButton *event, gpointer data) {
  DEBUG_MSG("Selected row %i, column %i", row, column);
  selected_row = row;
}

static void auto_add_clicked(GtkButton *button, gpointer data) {
  gboolean status = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_add_checkbox));

  set_random_option_frame_status(status);
}

static void remove_dir_button_clicked(GtkButton *button, gpointer data) {
  DEBUG_MSG("About to remove row %i", selected_row);
  gtk_clist_remove(GTK_CLIST(folder_list), selected_row);
  folder_count--;
}

static void add_dir_button_clicked(GtkButton *button, gpointer data) {
  char wd[] = "/home/proguy/mp3";

  if( !dir_browser ) {
    dir_browser = xmms_create_dir_browser("Add directory", wd, NULL, 
					  GTK_SIGNAL_FUNC(dir_browser_handler));
    gtk_widget_show(dir_browser);
  } else {
    DEBUG_MSG("dir_browser: %X", dir_browser);
  }
}

static void ok_button_clicked(GtkButton *button, gpointer data) {
  ConfigFile *cfgfile;
  int i;
  char *t;

  random_song = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_add_checkbox));
  auto_clear_tags = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(clear_cache_checkbox));

  t = gtk_editable_get_chars(GTK_EDITABLE(change_entry), 0, -1);
  random_song_change = atoi(t);
  g_free(t);

  if( running ) {
    DEBUG_MSG("Updating config");
    if( random_song )
      init_timer();
    else
      done_timer();
  } else
    DEBUG_MSG("Just saving config");
  

  cfgfile = xmms_cfg_open_default_file();

  xmms_cfg_write_boolean(cfgfile, "media_library", "random_song", random_song);
  xmms_cfg_write_int(cfgfile, "media_library", "random_song_change",
		     random_song_change);
  xmms_cfg_write_boolean(cfgfile, "media_library", "autoclear_cache", auto_clear_tags);
  xmms_cfg_write_int(cfgfile, "media_library", "folder_count", folder_count);
  for(i = 0; i<folder_count; i++) {
    char *s, *v;

    gtk_clist_get_text(GTK_CLIST(folder_list), i, 0, &v);

    s = g_strdup_printf("folder%i", i);
    xmms_cfg_write_string(cfgfile, "media_library", s, v);
    
    DEBUG_MSG("Writing folder %s <- %s", s, v);

    g_free(s);
  }

  xmms_cfg_write_default_file(cfgfile);

  xmms_cfg_free(cfgfile);

  DEBUG_MSG("Destroying window...");
  gtk_widget_destroy(configure_win);
  configure_win = NULL;
  DEBUG_MSG("Done");


  DEBUG_MSG("Last part");
  if( running ) {
    DEBUG_MSG("Re-reading configuration");
    pthread_mutex_unlock(&loading_mutex);
    free_folders();
    free_files();
    read_config();
    load_folders_threaded(TRUE);
  }

  DEBUG_MSG("Finished");

}

static void cancel_button_clicked(GtkButton *button, gpointer data) {
  gtk_widget_destroy(configure_win);
  configure_win = NULL;
}

void gui_done_configure() {
  if( configure_win )
    gtk_widget_destroy(configure_win);
}

void gui_read_config() {
  ConfigFile *cfgfile;
  char *text[1];

  folder_count = 0;

  if( (cfgfile = xmms_cfg_open_default_file()) != NULL ) {
    int i;
    char *s, *v;

    xmms_cfg_read_boolean(cfgfile, "media_library", "random_song", 
			  &config_random_song);
    xmms_cfg_read_int(cfgfile, "media_library", "random_song_change", 
		      &config_random_song_change);

    xmms_cfg_read_boolean(cfgfile, "media_library", "autoclear_cache", 
			  &config_clear_cache);
    xmms_cfg_read_int(cfgfile, "media_library", "folder_count", &folder_count);

    DEBUG_MSG("Folder_count is %i", folder_count);

    for(i = 0; i<folder_count; i++) {
      s = g_strdup_printf("folder%i", i);
      xmms_cfg_read_string(cfgfile, "media_library", s, &v);

      text[0] = g_strdup(v);
      DEBUG_MSG("Adding %s to folder_list", v);
      gtk_clist_append(GTK_CLIST(folder_list), text);

      g_free(s);
      g_free(v);
    }
    
    xmms_cfg_free(cfgfile);
  }
  
}

void set_random_option_frame_status( gboolean s ) {
  gtk_widget_set_sensitive( GTK_WIDGET(auto_add_option_frame), s);
}

void change_entry_insert_text(GtkEditable *editable, gchar *new_text,
			      gint new_text_length, gint *position,
			      gpointer user_data) {
  char *n;
  long int value;
  char *p;

  n = g_strndup(new_text, new_text_length);

  value = strtol(n, &p, 10);

  if( (p < n + new_text_length) && !(n[0] == '-' && (*position)==0 ))
    gtk_signal_emit_stop_by_name( GTK_OBJECT(editable), "insert-text");
}

void gui_init_configure() {
  GtkWidget *table;
  
  GtkWidget *button;
  GtkWidget *label1;  
  GtkWidget *label2;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *notebook;
  GtkWidget *folder_list_scroller;
  GtkWidget *folder_hbox;
  GtkWidget *folder_vbox;
  GtkWidget *folder_button1;
  GtkWidget *folder_button2;
  GtkWidget *general_box;

  GtkWidget *button_hbox;

  /* Random option frame widgets*/
  GtkWidget *change_label;
  GtkWidget *change_textbox;
  GtkWidget *change_hbox;
  GtkWidget *auto_add_option_frame_hbox;

  /* A little static buffer used for number-to-text-conversion */
  char *temp;
  int temp_int;
  
  if( configure_win )
    return;

  dir_browser = NULL;

  folder_count = 0;

  /* Setup main configure window, and main table */
  configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (configure_win), GDK_WINDOW_TYPE_HINT_DIALOG);
  table = gtk_table_new(2,3, FALSE);
  gtk_container_add(GTK_CONTAINER(configure_win), table);

  /* Set the notebook thingie up*/
  notebook = gtk_notebook_new();
  label1 = gtk_label_new("General");
  label2 = gtk_label_new("Paths");

  //gtk_table_attach_defaults(GTK_TABLE(table), notebook, 0, 3, 0, 1);

  general_box = gtk_vbox_new(FALSE, 1);
  /* Setup the checkbox for adding random song to playlist*/
  clear_cache_checkbox = gtk_check_button_new_with_label("Clear unused ID3-tags cached in memory when reloading (saves a little memory at times)");

  auto_add_checkbox = gtk_check_button_new_with_label("Add random file, when playlist is near end");

  gtk_signal_connect( GTK_OBJECT(auto_add_checkbox), "toggled",
		      GTK_SIGNAL_FUNC(auto_add_clicked), NULL);

  /** START SETUP: Random option frame and content (auto_add_option_frame) **/

  /* construct the content*/
  change_label = gtk_label_new("% towards the end of a song, to add a new song (0% is the beginning, 50% halfway through, and 100% at the end)");
  gtk_label_set_line_wrap(GTK_LABEL(change_label), TRUE);
  change_entry = gtk_entry_new();
  
  gtk_signal_connect( GTK_OBJECT(change_entry), "insert-text",
		      GTK_SIGNAL_FUNC(change_entry_insert_text), NULL);


  /* Construct frame and layout objects */
  auto_add_option_frame_hbox = gtk_hbox_new(FALSE, 1);
  auto_add_option_frame = gtk_frame_new("Random add options");
  gtk_container_add( GTK_CONTAINER(auto_add_option_frame), auto_add_option_frame_hbox);


  change_hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(change_hbox), change_label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(change_hbox), change_entry, TRUE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(auto_add_option_frame_hbox), change_hbox, FALSE,
		     FALSE, 2);

  gtk_widget_show(change_label);
  gtk_widget_show(change_hbox);
  gtk_widget_show(change_entry);
  gtk_widget_show(auto_add_option_frame);
  gtk_widget_show(auto_add_option_frame_hbox);

  /** END SETUP: Random option frame and content (auto_add_option_frame) **/

  gtk_box_pack_start(GTK_BOX(general_box), auto_add_checkbox, FALSE, FALSE, 1);
  gtk_box_pack_start(GTK_BOX(general_box), auto_add_option_frame, FALSE, 
		     FALSE, 2);
  gtk_box_pack_start(GTK_BOX(general_box), clear_cache_checkbox, FALSE, 
		     FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_box, label1);


  /* Setup the "Paths" notebook pane*/
  folder_list = gtk_clist_new(1);
  gtk_clist_set_column_title(GTK_CLIST(folder_list), 0, "Directory");
  gtk_clist_column_titles_passive(GTK_CLIST(folder_list));
  gtk_clist_column_titles_show(GTK_CLIST(folder_list));

  folder_hbox = gtk_hbox_new(FALSE, 0);
  folder_vbox = gtk_vbox_new(FALSE, 0);
  folder_button1 = gtk_button_new_with_label("Add directory");
  folder_button2 = gtk_button_new_with_label("Remove directory");

  gtk_signal_connect( GTK_OBJECT(folder_button1), "clicked",
		      GTK_SIGNAL_FUNC(add_dir_button_clicked), NULL);

  gtk_signal_connect( GTK_OBJECT(folder_button2), "clicked",
		      GTK_SIGNAL_FUNC(remove_dir_button_clicked), NULL);

  gtk_signal_connect( GTK_OBJECT(folder_list), "select-row",
		      GTK_SIGNAL_FUNC(folder_list_select_row), NULL);


  folder_list_scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(folder_list_scroller), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(folder_list_scroller), folder_list);

  /* Add all the components into each other ;)*/
  gtk_box_pack_start(GTK_BOX(folder_hbox), folder_list_scroller, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(folder_hbox), folder_vbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(folder_vbox), folder_button1, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(folder_vbox), folder_button2, FALSE, FALSE, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), folder_hbox, label2);

  /* Set the ok and cancel buttons up*/
  ok_button = gtk_button_new_with_label("Ok");  
  gtk_signal_connect( GTK_OBJECT(ok_button), "clicked",
		      GTK_SIGNAL_FUNC(ok_button_clicked), NULL);

  cancel_button = gtk_button_new_with_label("Cancel");
  gtk_signal_connect( GTK_OBJECT(cancel_button), "clicked",
		      GTK_SIGNAL_FUNC(cancel_button_clicked), NULL);

  button_hbox = gtk_hbox_new(TRUE, 1);

  gtk_box_pack_start( GTK_BOX(button_hbox), ok_button, TRUE, TRUE, 5);
  gtk_box_pack_start( GTK_BOX(button_hbox), cancel_button, TRUE, TRUE, 5);

  gtk_table_attach(GTK_TABLE(table), notebook, 0,3, 0,1, GTK_FILL+GTK_EXPAND,
		   GTK_FILL+GTK_EXPAND, 10,10);
  /*  gtk_box_pack_start( GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);
      gtk_box_pack_start( GTK_BOX(main_vbox), button_hbox, TRUE, TRUE, 0);*/

  /*  gtk_table_attach(GTK_TABLE(table), ok_button, 0,1, 1,2,  0,0, 0,5);
      gtk_table_attach(GTK_TABLE(table), cancel_button, 2,3, 1,2, 0,0, 0,5);*/
  gtk_table_attach( GTK_TABLE(table), button_hbox, 2,3, 1,2, 
		    GTK_FILL, GTK_FILL, 0,5);

  gtk_widget_show(button_hbox);

  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(notebook);

  gtk_widget_show(folder_hbox);
  gtk_widget_show(folder_vbox);
  gtk_widget_show(folder_list);
  gtk_widget_show(folder_button1);
  gtk_widget_show(folder_button2);
  gtk_widget_show(folder_list_scroller);

  gtk_widget_show(auto_add_checkbox);
  gtk_widget_show(general_box);
  gtk_widget_show(clear_cache_checkbox);
  gtk_widget_show(ok_button);
  gtk_widget_show(cancel_button);
  
  gtk_widget_show(table);

  gtk_widget_show(configure_win);

  gui_read_config();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_add_checkbox), config_random_song);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clear_cache_checkbox), config_clear_cache);

  temp = g_strdup_printf("%i", config_random_song_change);
  temp_int = 0;

  gtk_editable_delete_text( GTK_EDITABLE(change_entry), 0, -1);
  gtk_editable_insert_text( GTK_EDITABLE(change_entry), temp, strlen(temp),
			    &temp_int);
  g_free(temp);

  if( config_random_song )
    set_random_option_frame_status(TRUE);
  else
    set_random_option_frame_status(FALSE);
}
