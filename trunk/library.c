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

#include "library.h"

guint timer_id, timer1_id;
GList *lookup_list = NULL;
char *initial_playlist;
gboolean random_song;
gint random_song_change;

gboolean auto_clear_tags;
pthread_t load_thread;
pthread_t info_thread;
pthread_mutex_t lookup_list_mutex;
pthread_mutex_t quit_mutex;
pthread_cond_t lookup_list_cond;
pthread_mutex_t threads_mutex;
int threads;
gboolean quit;

pthread_mutex_t loading_mutex;
gboolean loading;
gboolean running = FALSE;
gboolean cq_running;
gboolean info_running;

gchar *tags_file;

gboolean update_display = FALSE;
pthread_mutex_t update_display_mutex;


GeneralPlugin cue_gp =
{
	0,			/* handle */
	0,			/* filename */
	-1,		 	/* xmms_session */
	"BeepMediaLibrary-0.1",			/* Description */
	init,
	NULL,
	gui_init_configure,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
	cue_gp.description = g_strdup_printf("%s %s", PACKAGE, VERSION);
	return &cue_gp;
}

void read_config(gboolean folders_only) {
	DEBUG_MSG("read_config");
  ConfigFile *cfgfile;
  gint folder_count;

  folder_count = 0;

  if( (cfgfile = xmms_cfg_open_default_file()) != NULL ) {
    int i;
    char *s, *v;

    if( !folders_only) {
      random_song_change = 0;
      xmms_cfg_read_boolean(cfgfile, "media_library", "random_song", &random_song);
      xmms_cfg_read_boolean(cfgfile, "media_library", "autoclear_cache", &auto_clear_tags);
      xmms_cfg_read_int(cfgfile, "media_library", "random_song_change", &random_song_change);
    }

    xmms_cfg_read_int(cfgfile, "media_library", "folder_count", &folder_count);

    for(i = 0; i<folder_count; i++) {
      s = g_strdup_printf("folder%i", i);
      xmms_cfg_read_string(cfgfile, "media_library", s, &v);
      DEBUG_MSG("Read folder: %s", v);
      add_folder(v);
      g_free(s);
      g_free(v);
    }
    
    //load_folders_threaded(FALSE);
    
    xmms_cfg_free(cfgfile);
  }
}

void add_file(gchar *filename) {
  cq_enqueue(CQ_ADD, filename, NULL); 

  /* The enqueue function now takes care of lookup if necesarry*/
  //  info_enqueue(filename);
}

void add_dir(char *directory) {
  DIR *dir;
  struct dirent *entry;
  struct stat statbuf;

  dir = opendir(directory);

  if( stat(directory, &statbuf) != 0 ) {
    DEBUG_MSG("Could not stat %s, aborting", directory);
    return;
  }
  
  pthread_mutex_lock(&quit_mutex);
  while( (entry = readdir(dir)) != NULL  && !quit ) {
    char *filename;
    char *filename1;

    pthread_mutex_unlock(&quit_mutex);

    filename1 = g_strconcat(directory, "/", NULL);
    filename = g_strconcat(filename1, entry->d_name, NULL);
    g_free(filename1);

    if( stat(filename, &statbuf) == 0 && entry->d_name[0] != '.') {
      
      if( S_ISDIR(statbuf.st_mode) ) {
	add_dir(filename);
      } else
      if( input_check_file(filename, FALSE) ) {
	add_file(filename);
      }
    }

    g_free(filename);
    pthread_mutex_lock(&quit_mutex);
  }
  pthread_mutex_unlock(&quit_mutex);

  closedir(dir);
}

gint update_function(gpointer data) {
DEBUG_MSG("update_function");
  pthread_mutex_lock(&loading_mutex);
  if( !loading ) {
    pthread_mutex_unlock(&loading_mutex);
    gtk_object_set( GTK_OBJECT(reload_button), "label", "Reload folders", NULL);
  } else
    pthread_mutex_unlock(&loading_mutex);

  pthread_mutex_lock(&update_display_mutex);
  if( update_display ) {
    update_display = FALSE;
    pthread_mutex_unlock(&update_display_mutex);
    update_list();
  } else
    pthread_mutex_unlock(&update_display_mutex);

  return TRUE;
}
			     
gint playlist_check(gpointer data) {
  gint pos, length, f_length;
  gint total, played;

  if( !xmms_remote_is_playing(cue_gp.xmms_session) ) 
    return TRUE;
  
  pos = xmms_remote_get_playlist_pos(cue_gp.xmms_session);
  length = xmms_remote_get_playlist_length(cue_gp.xmms_session);
  f_length = g_list_length(files);

  total = xmms_remote_get_playlist_time(cue_gp.xmms_session, pos);
  played = xmms_remote_get_output_time(cue_gp.xmms_session);

  if( (pos == length-1 && f_length > 0) &&
      ((float)played/(float)total)*100 >= random_song_change ) {
    int r = ((float)rand()/(float)RAND_MAX) * f_length;
    add_to_playlist_absolute( r );
  }  

  return TRUE;
}

void init(void)
{
  DEBUG_MSG("init");
  tags_file = g_strdup_printf("%s/.beepMediaLibrary", getenv("HOME"));
  
  pthread_mutex_init(&cq_list_mutex, NULL);
  pthread_mutex_init(&tags_mutex, NULL);
  pthread_mutex_init(&lookup_list_mutex, NULL);
  pthread_mutex_init(&quit_mutex, NULL);
  pthread_mutex_init(&threads_mutex, NULL);
  pthread_mutex_init(&files_mutex, NULL);
  pthread_mutex_init(&folders_mutex, NULL);
  pthread_mutex_init(&loading_mutex, NULL);
  pthread_mutex_init(&updating_mutex, NULL);
  pthread_mutex_init(&update_display_mutex, NULL);

  pthread_cond_init(&cq_list_cond, NULL);
  pthread_cond_init(&lookup_list_cond, NULL);

  running = TRUE;
  loading = FALSE;
  updating = FALSE;

  folders = NULL;
  files = NULL;
  tags = g_hash_table_new(g_str_hash, g_str_equal);
  cq_list = NULL;
  quit = FALSE;

  cq_running = FALSE;
  info_running = FALSE;

  read_config(FALSE);
  // load_tags(tags_file);
  // doesn't work yet

  init_gui_playlist();

  /* Setup the initial_playlist variable */
  //  initial_playlist = g_strdup_printf("%s%s", getenv("HOME"), INITIAL_PLAYLIST);
  //load_list_init(initial_playlist);


  /* Setup the timeout function, that will check if the playlist is near end*/
  if( random_song )
    init_timer();

  pthread_mutex_lock(&threads_mutex);
  threads = 0;
  pthread_mutex_unlock(&threads_mutex);

  pthread_mutex_lock(&quit_mutex);
  quit = FALSE;
  pthread_mutex_unlock(&quit_mutex);

  /* Initializes the info-lookup-thread*/
  pthread_create(&info_thread, NULL, info_thread_func, NULL);
  pthread_create(&cq_thread, NULL, cq_thread_func, NULL);

  pthread_mutex_lock(&loading_mutex);
  loading = TRUE;
  pthread_mutex_unlock(&loading_mutex);

  timer1_id = gtk_timeout_add(1000, update_function, NULL);

  load_folders_threaded(FALSE);
}

void info_enqueue(gchar *filename) {
DEBUG_MSG("info_enqueue");
  gchar *f;

  f = g_strdup(filename);

  pthread_mutex_lock(&lookup_list_mutex);
  lookup_list = g_list_append(lookup_list, f);
  pthread_cond_broadcast(&lookup_list_cond);

  pthread_mutex_unlock(&lookup_list_mutex);
}

void *info_thread_func(void *a) {
DEBUG_MSG("info_thread_func");
  GList *e;
  gchar *filename;
  gchar *title;
  gint length;

  pthread_mutex_lock(&threads_mutex);
  threads++;
  pthread_mutex_unlock(&threads_mutex);

  info_running = TRUE;

  pthread_mutex_lock(&quit_mutex);
  while(!quit) {

    pthread_mutex_lock(&lookup_list_mutex);
    if( g_list_length(lookup_list) < 1 && !quit ) {
      pthread_mutex_unlock(&quit_mutex);

      pthread_cond_wait(&lookup_list_cond, &lookup_list_mutex);

      pthread_mutex_lock(&quit_mutex);
    }

    pthread_mutex_unlock(&lookup_list_mutex);
    	
    if( quit )
      break;
    
    pthread_mutex_unlock(&quit_mutex);

    pthread_mutex_lock(&lookup_list_mutex);
    if( g_list_length(lookup_list) > 0 ) {
      gchar *temp, *ext;
      TitleInput *input;
      /*      XMMS_NEW_TITLEINPUT(input);

      e = g_list_first(lookup_list);
      filename = (gchar*)e->data;
      pthread_mutex_unlock(&lookup_list_mutex);

      temp = g_strdup(filename);
      ext = strrchr(temp, '.');
      if(ext)
	*ext = '\0';

      input->file_name = g_basename(temp);
      input->file_ext = ext ? ext+1: NULL;
      input->file_path = temp;
      
      title = xmms_get_titlestring("%F -- %a: %t", input);

      g_free(temp);
      g_free(input);*/

      e = g_list_first(lookup_list);
      filename = (gchar*)e->data;

      pthread_mutex_unlock(&lookup_list_mutex);
      
      if (input_check_file(filename, FALSE)) {
      	input_get_song_info(filename, &title, &length);

      	cq_enqueue(CQ_UPDATE, filename, title);
      	//      g_free(filename);
      	g_free(title);
      }	
    
      pthread_mutex_lock(&lookup_list_mutex);
      lookup_list = g_list_remove(lookup_list, e->data);
    }
    pthread_mutex_unlock(&lookup_list_mutex);    
    pthread_mutex_lock(&quit_mutex);
  }
  pthread_mutex_unlock(&quit_mutex);

  DEBUG_MSG("Finished loop, freeing list...");

  /* Free all of the remaining elements in the list*/

  e = g_list_first(lookup_list);

  while( e != NULL ) {
    g_free(e->data);
    e = g_list_next(e);
  }

  g_list_free(lookup_list);

  pthread_mutex_lock(&threads_mutex);
  threads--;
  pthread_mutex_unlock(&threads_mutex);

  info_running = FALSE;
}

void init_timer(void) {
DEBUG_MSG("init_timer");
  done_timer();

  timer_id = gtk_timeout_add(TIMEOUT, playlist_check, NULL);
  DEBUG_MSG("Enabling timer function");
}

void done_timer(void) {
DEBUG_MSG("done_timer");
  gtk_timeout_remove(timer_id);
  DEBUG_MSG("Disabling timer function");
}

void cleanup(void)
{
  DEBUG_MSG("cleanup");
  int t;

  /* It might be wise with a mutex here too */
  running = FALSE;

  /* signal the quit */
  pthread_mutex_lock(&quit_mutex);
  quit = TRUE;
  pthread_mutex_unlock(&quit_mutex);

  DEBUG_MSG("Destroying timer...");
  if( timer_id )
    gtk_timeout_remove(timer_id);
  DEBUG_MSG("Done");

  if( timer1_id )
    gtk_timeout_remove(timer1_id);

  /* broadcast some conditions, so that the threads get closed */
  pthread_mutex_lock(&lookup_list_mutex);
  pthread_cond_broadcast(&lookup_list_cond);
  pthread_mutex_unlock(&lookup_list_mutex);

  pthread_mutex_lock(&cq_list_mutex);
  pthread_cond_broadcast(&cq_list_cond);
  pthread_mutex_unlock(&cq_list_mutex);

  //  save_list(initial_playlist);

  pthread_mutex_lock(&threads_mutex);
  t = threads;
  pthread_mutex_unlock(&threads_mutex);

  DEBUG_MSG("Waiting for threads to close...");

  while( t > 0 ) {
    //    sleep(1);
    pthread_mutex_lock(&threads_mutex);
    t = threads;
    pthread_mutex_unlock(&threads_mutex);
    DEBUG_MSG(".");
    DEBUG_MSG("t er %i", t);
    
    if( cq_running ) {
      DEBUG_MSG("Command queue is running");
    } else {
      DEBUG_MSG("Command Queue has finished");
    }

    if( info_running ) {
      DEBUG_MSG("Info thread is running");
    } else {
      DEBUG_MSG("Info thread has finished");
    }  
  }

  DEBUG_MSG("Done");
  DEBUG_MSG("Freeing files and folders...");

  DEBUG_MSG("files");
  free_files();
  DEBUG_MSG("folders");
  free_folders();
  //  save_tags(tags_file);
  free_tags();

  DEBUG_MSG("Done");

  DEBUG_MSG("Destroying mutexes and conditions...");

  pthread_mutex_destroy(&cq_list_mutex);
  pthread_mutex_destroy(&lookup_list_mutex);
  pthread_mutex_destroy(&quit_mutex);
  pthread_mutex_destroy(&threads_mutex);
  pthread_mutex_destroy(&files_mutex);
  pthread_mutex_destroy(&folders_mutex);
  pthread_mutex_destroy(&loading_mutex);
  pthread_mutex_destroy(&updating_mutex);
  pthread_mutex_destroy(&tags_mutex);

  pthread_cond_destroy(&lookup_list_cond);
  pthread_cond_destroy(&cq_list_cond);
  DEBUG_MSG("Done\n");

  DEBUG_MSG("destroying gtk window...\n");
  done_gui_playlist();
  DEBUG_MSG("Done\n");

  DEBUG_MSG("Cleanup finished\n");
  g_free(tags_file);
}


