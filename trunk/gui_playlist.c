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

#include "gui_playlist.h"
//#include "skin.h"

GtkWidget *library_window;
GtkWidget *file_list;
GtkWidget *table;
GtkWidget *layout_box;
GtkWidget *filew; // file selection window
GtkWidget *dir_browser; // directory browser widget
GtkWidget *search_field;
GtkWidget *search_button;
GtkWidget *reload_button;
GtkWidget *list_dialog; // load/save list dialog
GtkWidget *total_count_label;
GtkWidget *showing_count_label;
GtkWidget *play_button;

gboolean updating;
pthread_mutex_t updating_mutex;

gint selected_row;
gint row_count;

gint play_button_handler_id;

GdkPixmap *background = NULL;
GdkBitmap *background_mask = NULL;
GdkGC *pl_gc = NULL;

/* Some variables from XMMS*/
extern GList* dock_window_list; // The list of docked windows
extern GtkWidget* mainwin; // XMMS' main window 

GList *dock_add_window(GList *window_list, GtkWidget *window);

void load_list_button_clicked( GtkWidget *widget, gpointer data) {
  if( list_dialog )
    return;

  list_dialog = gtk_file_selection_new("Load list...");
  g_signal_connect( GTK_OBJECT(GTK_FILE_SELECTION(list_dialog)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(load_list_init), NULL);

  g_signal_connect( GTK_OBJECT(GTK_FILE_SELECTION(list_dialog)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC(cancel_list_dialog), NULL);

  gtk_widget_show(list_dialog);
}

void save_list_button_clicked( GtkWidget *widget, gpointer data) {
  if( list_dialog )
    return;

  list_dialog = gtk_file_selection_new("Save list...");
  g_signal_connect( GTK_OBJECT(GTK_FILE_SELECTION(list_dialog)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(save_list_init), NULL);

  g_signal_connect( GTK_OBJECT(GTK_FILE_SELECTION(list_dialog)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC(cancel_list_dialog), NULL);

  gtk_widget_show(list_dialog);
}

void cancel_list_dialog(gpointer data) {
  gtk_widget_destroy(list_dialog);
  list_dialog = NULL;
}

void file_list_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
 if( event->keyval == GDK_Return ) {
    pthread_create(&load_thread, NULL, add_selected_to_playlist_and_play, NULL);
    DEBUG_MSG("Adding all selected files");

  } else {
    gboolean forward = TRUE;

    DEBUG_MSG("Key pressed: %i", event->keyval);


    switch(event->keyval) {
    case GDK_Page_Up:
    case GDK_Page_Down:
    case GDK_Up:
    case GDK_Down:
    case GDK_Return:
      forward = FALSE;
      break;
    };
    
    if(forward) {

      DEBUG_MSG("Forwarding to search field");

      gtk_widget_event( GTK_WIDGET(search_field), (GdkEvent*)event);
      g_signal_stop_emission_by_name(GTK_OBJECT(widget), "key-press-event");
    }
    
    return TRUE;
  }
}

void remove_selected(void) {
  int i;

   for(i=0; i<row_count; i++) {
     struct entry *d = 
      (struct entry*)gtk_clist_get_row_data(GTK_CLIST(file_list), i);
    
    if( d->selected )
      cq_enqueue(CQ_REMOVE, d->filename, NULL);      
   }
   
   cq_enqueue(CQ_LISTUPDATE, NULL, NULL);
}

void save_list_init(gpointer data) {
  save_list(gtk_file_selection_get_filename(GTK_FILE_SELECTION(list_dialog)));
  
  if( list_dialog != NULL ) {
    gtk_widget_destroy(list_dialog);
    list_dialog = NULL;
  }
}

void load_list_init(gpointer data) {
  if( !loading) {
    loading = TRUE;

    if( data == NULL )
      data = (void*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(list_dialog));
    pthread_create(&load_thread, NULL, load_list, data );

    if( list_dialog != NULL ) {
      gtk_widget_destroy(list_dialog);
      list_dialog = NULL;
    }
  }
}

void close_file_dialog(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(filew);
}

void accept_file_dialog(GtkWidget *widget, gpointer data)
{
  add_file(gtk_file_selection_get_filename(GTK_FILE_SELECTION(filew)));

  update_list();
  gtk_widget_destroy(filew);
}

void dir_browser_handler( gchar *dir ) {
  dir_browser = NULL;

  add_dir(dir);
  cq_enqueue(CQ_LISTUPDATE, NULL, NULL);  
}

void reload_button_clicked( GtkWidget *widget, gpointer data) {

  pthread_mutex_lock(&loading_mutex);

  if( !loading ) {
    loading = TRUE;
    pthread_mutex_unlock(&loading_mutex);

    free_folders();
    free_files();

    read_config(auto_clear_tags);

    /* load the folders, and clear the tags database */
    load_folders_threaded(TRUE);
  } else
    pthread_mutex_unlock(&loading_mutex);
}

void play_button_clicked( GtkWidget *widget, gpointer data) {
	pthread_create(&load_thread, NULL, add_selected_to_playlist_and_play, NULL);
}

void enqueue_button_clicked( GtkWidget *widget, gpointer data) {
	pthread_create(&load_thread, NULL, add_selected_to_playlist, NULL);
}


void add_dir_button_clicked( GtkWidget *widget, gpointer data) {
  char wd[] = "/home/proguy/mp3";

  if( !dir_browser ) {
    dir_browser = xmms_create_dir_browser("Choose directory", wd, NULL, 
					  GTK_SIGNAL_FUNC(dir_browser_handler));
    gtk_widget_show(dir_browser);
  }
}

void add_random_button_clicked( GtkWidget *widget, gpointer data) {
    gint f_length = g_list_length(files);
    int r = ((float)rand()/(float)RAND_MAX) * f_length;
    add_to_playlist_absolute( r );  
}

void list_add_file_dialog( GtkWidget *widget, gpointer data) 
{

  filew = gtk_file_selection_new("Select file");
  g_signal_connect(GTK_OBJECT( GTK_FILE_SELECTION(filew)->cancel_button),
      "clicked", GTK_SIGNAL_FUNC(close_file_dialog), NULL);

  g_signal_connect(GTK_OBJECT( GTK_FILE_SELECTION(filew)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(accept_file_dialog), NULL);

  gtk_widget_show(filew);
}

void add_selected_to_playlist_and_play(void) {
  int i;
  int playnow = 1;
  
  

  for(i=0; i<row_count; i++) {
    struct entry *d = 
      (struct entry*)gtk_clist_get_row_data(GTK_CLIST(file_list), i);

    if( d->selected ) {
      	if (!playnow) 	add_to_playlist(i);
	else {
		playnow = 0;
		add_to_playlist_and_play(i);
	}
      }
  }
}

void add_selected_to_playlist(void) {
  int i;

  DEBUG_MSG("add_selected_to_playlist");
    
  for(i=0; i<row_count; i++) {
    struct entry *d = 
      (struct entry*)gtk_clist_get_row_data(GTK_CLIST(file_list), i);

    if( d->selected ) {
      	add_to_playlist(i);
    }
  }
}


void search_modify(GtkEditable *editable, gpointer data) {
  DEBUG_MSG("search_modify");
  update_list();
}

gboolean file_list_pressed(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  DEBUG_MSG("file_list_pressed");
  if( event->type == GDK_2BUTTON_PRESS ) {
    if (selected_row >= 0) pthread_create(&load_thread, NULL, add_to_playlist, (void *) selected_row);
  }
  return FALSE;
}

void file_list_select(GtkList *clist, gint row, gint column, 
		     GdkEventButton *event, gpointer data)
{
  DEBUG_MSG("file_list_select");
  selected_row = row;
  ((struct entry*)gtk_clist_get_row_data(GTK_CLIST(file_list), row))->selected = TRUE;
}

void file_list_unselect(GtkList *clist, gint row, gint column, 
		     GdkEventButton *event, gpointer data)
{
  DEBUG_MSG("file_list_select");
  ((struct entry*)gtk_clist_get_row_data(GTK_CLIST(file_list), row))->selected = FALSE;
}

void add_to_playlist(gint row)
{
  DEBUG_MSG("add_to_playlist");
  gchar *text;
  GList *list = NULL;
  struct entry *d = ((struct entry*) gtk_clist_get_row_data(GTK_CLIST(file_list),row));

  if( d == NULL ) {
    DEBUG_MSG("WRONG\n");
    return;
  }

  text = d->filename;

  list = g_list_append(list, text);
  
  xmms_remote_playlist_add(cue_gp.xmms_session, list);
  DEBUG_MSG("add_to_playlist finished");
}

void add_to_playlist_and_play(gint row) {
	xmms_remote_playlist_clear(cue_gp.xmms_session);
	add_to_playlist(row);
	xmms_remote_set_playlist_pos(cue_gp.xmms_session, 0);
	xmms_remote_play(cue_gp.xmms_session);
}

void free_row_data(gpointer data){
  free(data);
}

void update_list(void) {
  struct entry *e;
  gchar *s;
  GList *l;
  char *text[1];
  gchar *keys[50];
  gint count;
  char *ptr;
  char *p;
  gint row;
  char buf[10];

  DEBUG_MSG("update_list()");

  pthread_mutex_lock(&updating_mutex);

  if( updating ) {
    DEBUG_MSG("Allready updating... skipping update");
    pthread_mutex_unlock(&updating_mutex);
    return;
  }

  updating = TRUE;
  pthread_mutex_unlock(&updating_mutex);

  if( file_list == NULL )
    return;

  gtk_clist_freeze(GTK_CLIST(file_list));

  s = g_strdup(gtk_entry_get_text(GTK_ENTRY(search_field)));
  g_strdown(s);

  count = 0;
  p = s;

  l = g_list_first(files);
  if( l != NULL ) {
    e = l->data;
  } else
    DEBUG_MSG("First data is null (this is wrong!!)");

  pthread_mutex_lock(&files_mutex);

  while( (ptr = strchr(p, ' ')) != NULL  && count < 50) {

    if( p[0] != ' ' ) {
      keys[count] = p;
      count++;
    }

    while( *ptr == ' ' ) {
      *ptr = '\0';
      ptr++;
    }
   
    p = ptr;
  }

  if( p[0] != ' ' ) {
    keys[count] = p;
    count++;
  }

  gtk_clist_clear(GTK_CLIST(file_list));
  l = g_list_first(files);

  row_count = 0;

  while( l != NULL ) {
    int c;
    gboolean match = TRUE;

    e = l->data;

    for(c = 0; (c<count) && match; c++) {
      if( e->title != NULL ) {
	if( (strstr(e->search_title, keys[c]) == NULL) && 
	    (strstr(e->search_filename, keys[c]) == NULL)
	    )
	  match = FALSE;
      }
      else
	if( (strstr(e->search_filename, keys[c]) == NULL) )
	  match = FALSE;
    }

    if( match ) {
      /*      struct list_data *d;

      d = malloc(sizeof(struct list_data));

      d->filename = e->filename;
      d->selected = FALSE;*/

      if( e->title != NULL )
	text[0] = e->title;
      else
	text[0] = rindex(e->filename, '/')+1;

      row = gtk_clist_append(GTK_CLIST(file_list), text);
      gtk_clist_set_row_data(GTK_CLIST(file_list), row, e);

      e->row = row;
      e->visible = TRUE;
      e->selected = FALSE;

      /*      gtk_clist_set_row_data_full(GTK_CLIST(file_list), row, d, 
	      GTK_SIGNAL_FUNC(free_list_data));*/
      row_count++;

    } else {
      e->visible = FALSE;
      e->selected = FALSE;
    }
    l = g_list_next(l);
  }

  g_free(s);

  gtk_clist_thaw(GTK_CLIST(file_list));

  snprintf(buf, 9, "%i", g_list_length(files));
  gtk_label_set_text(GTK_LABEL(total_count_label), buf);

  pthread_mutex_unlock(&files_mutex);

  if( row_count == 1 )
    gtk_clist_select_row(GTK_CLIST(file_list), 0, 0);

  snprintf(buf, 9, "%i", row_count);
  gtk_label_set_text(GTK_LABEL(showing_count_label), buf);

  DEBUG_MSG("Finished updating");
  pthread_mutex_lock(&updating_mutex);
  updating = FALSE;
  pthread_mutex_unlock(&updating_mutex);

}

gboolean search_field_focus_in( GtkWidget *widget, GdkEventFocus *event,
				gpointer user_data ) {
  DEBUG_MSG("Got focus, trying to sent it to folder_list");
  gtk_widget_grab_focus(GTK_WIDGET(file_list));
}

gboolean search_field_keypress(GtkWidget *widget, GdkEventKey *event,
				gpointer data) {
  DEBUG_MSG("search_field_keypress");
  gboolean forward = FALSE;

  switch(event->keyval) {
  case GDK_Page_Up:
  case GDK_Page_Down:
  case GDK_Up:
  case GDK_Down:
  case GDK_Return:
    forward = TRUE;
    break;
  };

  if(forward) {
    DEBUG_MSG("Forwarding key: %i", event->keyval);
    gtk_widget_event( GTK_WIDGET(file_list), (GdkEvent*)event);
    g_signal_stop_emission_by_name(GTK_OBJECT(widget), "key-press-event");
    gtk_widget_set_state( GTK_WIDGET(file_list), GTK_STATE_PRELIGHT );
  }
  
  return FALSE;
}

void init_gui_playlist(void) {
  GtkWidget *li;
  GtkWidget *scroller;
  GtkWidget *box;
  GtkWidget *table1;
  GtkWidget *reload_button;
  GtkWidget * enqueue_button;

  selected_row = -1;
  dir_browser = NULL;
  list_dialog = NULL;
  files = NULL;

  /* Create and setup main window */
  library_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  //gtk_window_set_decorated(library_window, FALSE);
  
  //  gtk_widget_set_app_paintable(library_window, TRUE);
  //  gtk_window_set_policy( GTK_WINDOW(library_window), FALSE, FALSE, TRUE);

  /* Put the window into the xmms class, and hide it from the window list*/
  gtk_window_set_wmclass( GTK_WINDOW(library_window), "XMMS_MediaLibrary",
			  "xmms");
  gtk_window_set_transient_for( GTK_WINDOW(library_window), 
				GTK_WINDOW(mainwin) );
  //  hint_set_skip_winlist( library_window );

  gtk_window_set_default_size(GTK_WINDOW(library_window), 350, 350);
  gtk_window_set_title(GTK_WINDOW(library_window), "Media Library"); 

  /*  gtk_widget_set_events( library_window, GDK_FOCUS_CHANGE_MASK |
			 GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK );
  */
  //  gtk_widget_realize( library_window );

  //  gtk_widget_set_usize( library_window, 300, 200 );

  /*  background = gdk_pixmap_new( NULL, 300, 200, gdk_rgb_get_visual()->depth );

  pl_gc = gdk_gc_new( library_window->window );

  printf("init\n");
  printf("%i\n", get_skin_color(SKIN_PLEDIT_NORMALBG));

  gdk_gc_set_foreground(pl_gc, get_skin_color(SKIN_PLEDIT_NORMALBG) );
  gdk_draw_rectangle( background, pl_gc, TRUE, 0, 0, 300, 200 );

  gdk_window_set_back_pixmap( library_window->window, background, 0);*/

  /* Remove the WMs decorations */
  //  gdk_window_set_decorations( library_window->window, 0);

  /* Setup list*/
  file_list = gtk_clist_new(1);

  g_signal_connect(GTK_OBJECT(file_list), "select-row",
		     GTK_SIGNAL_FUNC(file_list_select), NULL);

  g_signal_connect(GTK_OBJECT(file_list), "unselect-row",
		     GTK_SIGNAL_FUNC(file_list_unselect), NULL);

  g_signal_connect(GTK_OBJECT(file_list), "button-press-event",
		     G_CALLBACK(file_list_pressed), NULL);

  g_signal_connect(GTK_OBJECT(file_list), "key-press-event",
		     GTK_SIGNAL_FUNC(file_list_key_press), NULL);


  gtk_clist_set_column_title(GTK_CLIST(file_list), 0, "Title");
  gtk_clist_column_titles_passive(GTK_CLIST(file_list));
  gtk_clist_column_titles_show(GTK_CLIST(file_list));

  gtk_clist_set_selection_mode(GTK_CLIST(file_list), GTK_SELECTION_EXTENDED);

  gtk_widget_grab_default( GTK_OBJECT(file_list) );
  

  /* Setup the search field */
  search_field = gtk_entry_new();
  g_signal_connect(GTK_OBJECT(search_field), "changed",
		     GTK_SIGNAL_FUNC(search_modify), NULL);
  
  g_signal_connect(GTK_OBJECT(search_field), "key-press-event",
		     G_CALLBACK(search_field_keypress), NULL);

  /*  gtk_signal_connect(GTK_OBJECT(search_field), "focus-in-event",
      GTK_SIGNAL_FUNC(search_field_focus_in), NULL);*/

  /* Setup the search button 
  search_button = gtk_button_new_with_label("Search"); */

  /* Setup the reload button*/
  reload_button = gtk_button_new_with_label("Reload"); 
  g_signal_connect( GTK_OBJECT(reload_button), "clicked",
		      GTK_SIGNAL_FUNC(reload_button_clicked), NULL);

  /* Setup the play and enqueue-buttons */
  play_button = gtk_button_new_with_label("Play");
  play_button_handler_id = g_signal_connect(GTK_OBJECT(play_button), "clicked",
  			GTK_SIGNAL_FUNC(play_button_clicked), NULL);
  
  enqueue_button = gtk_button_new_with_label("Add");
  g_signal_connect(GTK_OBJECT(enqueue_button), "clicked",
  			GTK_SIGNAL_FUNC(enqueue_button_clicked), NULL);
			
  /* Set the scroller up*/
  scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  
  gtk_container_add(GTK_CONTAINER(scroller), file_list);

  /* Setup the labels 
  label1 = gtk_label_new("Total songs:");
  label2 = gtk_label_new("Songs showing:");

  gtk_label_set_justify(GTK_LABEL(label1), GTK_JUSTIFY_RIGHT);
  gtk_label_set_justify(GTK_LABEL(label2), GTK_JUSTIFY_RIGHT);

  total_count_label = gtk_label_new("-");
  showing_count_label = gtk_label_new("-");
  gtk_label_set_justify(GTK_LABEL(total_count_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_justify(GTK_LABEL(showing_count_label), GTK_JUSTIFY_LEFT);*/

  /* Setup layout widgets*/
  layout_box = gtk_vbox_new(FALSE, 1);

  gtk_container_add(GTK_CONTAINER(library_window), layout_box);
  gtk_widget_show(layout_box);

  table = gtk_table_new(3,2, TRUE);

  gtk_box_pack_start(GTK_BOX(layout_box), table, FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(layout_box), scroller);

  box = gtk_hbox_new(FALSE, 1);
  //gtk_box_pack_start(GTK_BOX(box), search_field, TRUE, TRUE, 0);

  /* The top 'bar'*/
  table1 = gtk_table_new(1, 4, FALSE);
  gtk_table_attach_defaults(GTK_TABLE(table1), search_field, 0,1,0,1); 
  gtk_table_attach_defaults(GTK_TABLE(table1), play_button, 1,2,0,1); 
  gtk_table_attach_defaults(GTK_TABLE(table1), enqueue_button, 2,3,0,1); 
  gtk_table_attach_defaults(GTK_TABLE(table1), reload_button, 3,4,0,1); 
  
/*  gtk_table_attach_defaults(GTK_TABLE(table1), reload_button, 2, 3, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table1), add_random_button, 2, 3, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(table1), label1, 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(table1), label2, 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(table1), total_count_label, 1,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(table1), showing_count_label, 1,2, 1,2);*/

  /* Add widgets to the table */
  gtk_table_attach_defaults(GTK_TABLE(table), table1, 0, 2, 0, 2);
  //gtk_table_attach_defaults(GTK_TABLE(table), box, 0, 2, 1, 3);


  /* Show all the widgets*/
  gtk_widget_show(reload_button);
  gtk_widget_show(play_button);
  gtk_widget_show(enqueue_button);
  //gtk_widget_show(box);
  gtk_widget_show(search_field);
  gtk_widget_show(scroller);
  gtk_widget_show(table1);
  gtk_widget_show(table);
  gtk_widget_show(file_list);

  /* And finally, show the window itself*/
  gtk_widget_show(library_window);

  //  dock_add_window(dock_window_list, library_window);
}

void done_gui_playlist(void) {

  /*
    Remove the playlist window from XMMS' dock list.
    I have the feeling that this might end up crashing XMMS because
    this plugin is multithreaded.
   */

  //  dock_window_list = g_list_remove(dock_window_list, library_window);

  /* I am not quite sure, wether it is necesarry to destroy
     all children of library_window, so I don't ;)
     When I find out that this is wrong, I'll change it ;)
   */
  gtk_widget_destroy(library_window);
}
