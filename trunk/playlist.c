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

#include "playlist.h"
#include "command_queue.h"

GList *files;
pthread_mutex_t files_mutex;

GList *folders;
pthread_mutex_t folders_mutex;

GHashTable *tags;
pthread_mutex_t tags_mutex;

char *read_line(VFSFile *f);

void load_folders_threaded(gboolean clear_tags) {
  
  gtk_object_set( GTK_OBJECT(reload_button), "label", "Reloading...", NULL);

  pthread_create(&load_thread, NULL, load_folders, (void*)clear_tags);
}

void load_folders(gboolean clear_tags) {
  DEBUG_MSG("loading folders (not threaded)");
  GList *l;

  pthread_mutex_lock(&folders_mutex);

  l = g_list_first(folders);

  pthread_mutex_lock(&quit_mutex);
  while ( (l != NULL) && !quit ) {
    pthread_mutex_unlock(&quit_mutex);
    add_dir((char*)(l->data));
    l = g_list_next(l);
    pthread_mutex_lock(&quit_mutex);
  }
  pthread_mutex_unlock(&quit_mutex);

  pthread_mutex_unlock(&folders_mutex);
  
  if( clear_tags )
    cleanup_tags();
    
  sort_files();

  cq_enqueue(CQ_LISTUPDATE, NULL, NULL);  

  //gtk_object_set( GTK_OBJECT(reload_button), "label", "Reload folders", NULL);

  pthread_mutex_lock(&loading_mutex);
  loading = FALSE;
  pthread_mutex_unlock(&loading_mutex);
}

/* sorts the files-list according to the filenames */
void sort_files() {
	files = g_list_sort(files, compare);
}

/* compares two entrys in the files-List */
gint compare(gpointer a, gpointer b)
{
	struct entry * entrya;
	struct entry * entryb;
	
	entrya = a;
	entryb = b;
	
	if ( entrya == NULL || entryb == NULL ) return 0;
	
	return strcmp((char *)entrya->filename,(char *)entryb->filename);
}

void add_folder(gchar *folder) {
  pthread_mutex_lock(&folders_mutex);
 
  folders = g_list_append(folders, g_strdup(folder));

  pthread_mutex_unlock(&folders_mutex);
}

char *read_line(VFSFile *f) {
  char *b = NULL;
  int s = 0;

  while( !feof(f)) {
    s++;
    b = (char*)realloc(b, s*sizeof(char));
    b[s-1] = (char)fgetc(f);

    if( b[s-1] == '\n' ) {
      b[s-1] = '\0';
      break;
    }

  }

  if( feof(f) )
    b[s-1] = '\0';

  return b;
}

void *load_list(void *a) {
  VFSFile *f;
  struct entry *e;
  GList *l;
  char *line;
  gchar *filename = 
    g_strdup((gchar*)a);

  f = vfs_fopen(filename, "r");

  if( f != NULL ) {
    /* Clear the list first */
    //  free_files(); -- or rather, don't, why should we?
    
    while( !feof(f) ) {
      line = read_line(f);
      
      add_file(line);
      free(line);
    }
    
    vfs_fclose(f);
    
    cq_enqueue(CQ_LISTUPDATE, NULL, NULL);
  }

  g_free(filename);

  loading = FALSE;
  return 0;
}

void save_list(gpointer data) {
  VFSFile *f;
  struct entry *e;
  GList *l;
  gchar *filename;

  if( data != NULL) {
    filename = g_strdup(data);
  }

  f = vfs_fopen(filename, "w");  

  l = g_list_first(files);

  while( l != NULL ) {
    e = l->data;
    fprintf(f, "%s\n", e->filename);
    l = g_list_next(l);
  }

  vfs_fclose(f);
  free(filename);
}

void save_tags_hash(gpointer key, gpointer value, gpointer data) {
  struct tags_entry *entry = value;
  VFSFile *f = data;

  if( entry == NULL || value == NULL || data == NULL || data == NULL)
    DEBUG_MSG("Something is NULL---WARNING!");

  fprintf(f, "%s\n%s\n", entry->filename, entry->title);
}

void save_tags(gchar *filename) {
  VFSFile *f;

  f = vfs_fopen(filename, "w");

  pthread_mutex_lock(&tags_mutex);
  g_hash_table_foreach(tags, save_tags_hash, f);
  pthread_mutex_unlock(&tags_mutex);

  vfs_fclose(f);
}

void load_tags(gchar *filename) {
  VFSFile *f;
  char *file;
  char *val;
  struct tags_entry *entry;

  f = vfs_fopen(filename, "r"); 

  DEBUG_MSG("**LOADING TAGS**");

  if( f == NULL )
    return;

  while( !feof(f) ) {
    file = read_line(f);
    if( feof(f) ) {
      free(file);
      break;
    }
    val = read_line(f);
    
    entry = (struct tags_entry*)malloc(sizeof(struct tags_entry));
    entry->filename = g_filename_to_utf8(g_strdup(file), -1, NULL, NULL, NULL);
    entry->title = filename_to_utf8(g_strdup(val), -1, NULL, NULL, NULL);
    entry->used = FALSE;
    entry->used_by = NULL;

    pthread_mutex_lock(&tags_mutex);
    g_hash_table_insert(tags, entry->filename, entry);
    pthread_mutex_unlock(&tags_mutex);

    free(val);
    free(file);
  }

  vfs_fclose(f);

  DEBUG_MSG("**TAGS LOADED**");

}

gboolean empty_tags_hash(gpointer key, gpointer value, gpointer data) {
  struct tags_entry *entry = value;
  
  g_free(entry->filename);
  g_free(entry->title);
  return TRUE;
}

void free_tags(void) {
  pthread_mutex_lock(&tags_mutex);
  g_hash_table_foreach_remove(tags, empty_tags_hash, NULL);
  pthread_mutex_unlock(&tags_mutex);
}

gboolean cleanup_tags_hash(gpointer key, gpointer value, gpointer data) {
  struct tags_entry *entry = value;
  
  if( entry->used )
    return FALSE;
  else {
    DEBUG_MSG("Removing information for %s", entry->filename);
    return TRUE;
  }
}

void cleanup_tags(void) {
  pthread_mutex_lock(&tags_mutex);
  g_hash_table_foreach_remove(tags, cleanup_tags_hash, NULL);
  pthread_mutex_unlock(&tags_mutex);
}

void unset_tags_hash(gpointer key, gpointer value, gpointer data) {
  struct tags_entry *entry = value;

  entry->used = FALSE;
  entry->used_by = NULL;
}

void free_files(void) {
  struct entry *e;
  GList *l;
  gint count = 0;

  pthread_mutex_lock(&files_mutex);

  l = g_list_first(files);

  while( l != NULL ) {
    e = l->data;
    
    g_free(e->filename);  
    g_free(e->title);
    g_free(e->search_title);
    g_free(e->search_filename);
    g_free(e);

    l = g_list_next(l);
    count++;
  }

  g_list_free(files);

  files = NULL;

  pthread_mutex_unlock(&files_mutex);

  pthread_mutex_lock(&tags_mutex);
  g_hash_table_foreach(tags, unset_tags_hash, NULL);
  pthread_mutex_unlock(&tags_mutex);
}

void free_folders(void) {
  GList *l;
  gint count = 0;

  pthread_mutex_lock(&folders_mutex);

  l = g_list_first(folders);

  while( l != NULL ) {
    g_free(l->data);

    l = g_list_next(l);
    count++;
  }

  g_list_free(folders);

  folders = NULL;

  pthread_mutex_unlock(&folders_mutex);
}

void add_to_playlist_absolute(gint n) {
  DEBUG_MSG("add to playlist absolute");
  GList *list = NULL;
  gchar * filename = ((struct entry*)g_list_nth_data(files, n))->filename;
  list = g_list_append(list, filename);
  DEBUG_MSG(filename);
  xmms_remote_playlist_add(cue_gp.xmms_session, list);
  DEBUG_MSG("add to playlist absolute - finished"); 
}
