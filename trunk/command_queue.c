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

#include "command_queue.h"
#include "playlist.h"

GList *cq_list;
pthread_t cq_thread;
pthread_mutex_t cq_list_mutex;
pthread_cond_t cq_list_cond;

void cq_enqueue(unsigned char type, gchar *filename, gchar *title) {
  struct cq_list_entry *e;

  if( quit )
    return;

  e = malloc(sizeof(struct cq_list_entry));
  e->type = type;

  if( filename != NULL )
    e->filename = g_strdup(filename);
  else {
    e->filename = NULL;
    if( type != CQ_LISTUPDATE ) {
      free(e);
      DEBUG_MSG("WARNING: Empty command put on stack, ignoring");
      return;
    }
  }
  
  if( title != NULL )
    e->title = g_strdup(title); 
  else
    e->title = NULL;
  
  pthread_mutex_lock(&cq_list_mutex);
  cq_list = g_list_append(cq_list, e);
  pthread_cond_broadcast(&cq_list_cond);
  pthread_mutex_unlock(&cq_list_mutex);
}

void *cq_thread_func(void *a) {
  GList *e;
  struct cq_list_entry *entry;

  DEBUG_MSG("Initializing command queue...");

  pthread_mutex_lock(&threads_mutex);
  threads++;
  pthread_mutex_unlock(&threads_mutex);

  DEBUG_MSG("Command queue initialization complete");

  cq_running = TRUE;

  pthread_mutex_lock(&quit_mutex);
  while(!quit) {

    pthread_mutex_lock(&cq_list_mutex);    
    if( g_list_length(cq_list) < 1  && !quit ) {
      pthread_mutex_unlock(&quit_mutex);

      pthread_cond_wait(&cq_list_cond, &cq_list_mutex);

      pthread_mutex_lock(&quit_mutex);
    }
    pthread_mutex_unlock(&cq_list_mutex);

    if( quit )
      break;
    
    pthread_mutex_unlock(&quit_mutex);

    pthread_mutex_lock(&cq_list_mutex);
    if( g_list_length(cq_list) > 0 )
      e = g_list_first(cq_list);
    else
      e = NULL;       

    if( e != NULL ) {
      entry = (struct cq_list_entry*)e->data;
      if( entry != NULL ) {
	switch(entry->type) {
	case CQ_ADD:
	  //	  DEBUG_MSG("cq_thread_func: CQ_ADD");
	  _cq_add(entry);
	  break;
	case CQ_REMOVE:
	  //	  DEBUG_MSG("cq_thread_func: CQ_REMOVE");
	  _cq_remove(entry);
	  break;
	case CQ_UPDATE:
	  //	  DEBUG_MSG("cq_thread_func: CQ_UPDATE");
	  _cq_update(entry);
	  break;
	case CQ_LISTUPDATE:
	  //	  DEBUG_MSG("cq_thread_func: CQ_LISTUPDATE");
	  //	  update_list();
	  pthread_mutex_lock(&update_display_mutex);
	  update_display = TRUE;
	  pthread_mutex_unlock(&update_display_mutex);
	  break;
	default:
	  fprintf(stderr, "UNKNOWN type: %i\n", entry->type);
	}
	       
	g_free(entry->filename);
	g_free(entry->title);

	cq_list = g_list_remove(cq_list, entry);
	g_free(entry);
      } else {
	DEBUG_MSG("WARNING: entry was NULL on command queue");
      }
    } else
      DEBUG_MSG("WARNING: NULL found on command queue");
    
    pthread_mutex_unlock(&cq_list_mutex);
    pthread_mutex_lock(&quit_mutex);
  }
  pthread_mutex_unlock(&quit_mutex);

  /* Free all of the remaining elements in the list*/

  e = g_list_first(cq_list);

  while( e != NULL ) {
    entry = (struct cq_list_entry*)e->data;
    g_free(entry->filename);
    g_free(entry->title);
    g_free(entry);
    e = g_list_next(e);
  }

  g_list_free(cq_list);

  pthread_mutex_lock(&threads_mutex);
  threads--;
  pthread_mutex_unlock(&threads_mutex);
  
  DEBUG_MSG("Command queue thread finished");
  cq_running = FALSE;
}


void _cq_remove(struct cq_list_entry *e) {
  GList *l;
  struct entry *entry;
  gboolean found;

  found = FALSE;

  pthread_mutex_lock(&files_mutex);

  l = g_list_first(files);

  while( l != NULL && !found) {
    entry = (struct entry*)l->data;

    if( strcmp(entry->filename, e->filename) == 0 ) {
      found = TRUE;
      break;
    } else
      l = g_list_next(l);
  }

  if( found ) {  
     files = g_list_remove(files, l->data);

     g_free(entry->title);

     g_free(entry->search_title);


     g_free(entry->search_filename);

     g_free(entry->filename);

    free(entry);
  }

  pthread_mutex_unlock(&files_mutex);
}

void _cq_update(struct cq_list_entry *e) {
  GList *l;
  struct entry *entry;
  struct tags_entry *t_entry;
  gboolean found;

  found = FALSE;

  pthread_mutex_lock(&files_mutex);

  l = g_list_first(files);

  while( l != NULL && !found) {
    entry = (struct entry*)l->data;

    if( strcmp(entry->filename, e->filename) == 0 ) {
      found = TRUE;
      break;
    } else
      l = g_list_next(l);
  }

  if( found ) {
    g_free(entry->title);
    g_free(entry->search_title);

    entry->title = g_strdup(e->title);
    entry->search_title = g_strdup(e->title);
    g_strdown(entry->search_title);

    t_entry = malloc( sizeof(struct tags_entry) );
    t_entry->filename = g_strdup(e->filename);
    t_entry->title = g_strdup(e->title);
    t_entry->used = TRUE;
    t_entry->used_by = entry;

    pthread_mutex_lock(&tags_mutex);
    g_hash_table_insert(tags, t_entry->filename, t_entry);
    pthread_mutex_unlock(&tags_mutex);

    //    DEBUG_MSG("Adding id3tag for %s to hash", t_entry->filename);
  }

  pthread_mutex_unlock(&files_mutex);
}

void _cq_add(struct cq_list_entry *e) {
  struct entry *new_entry;

  if( !input_check_file(e->filename, FALSE) ) {    
    DEBUG_MSG("_cq_add: Adding %s failed, no plugin support", e->filename);
    return;
  }
  
  new_entry = malloc(sizeof(struct entry));
  
  new_entry->filename = g_strdup(e->filename);
  new_entry->search_filename = g_strdup(e->filename);
  new_entry->selected = FALSE;
  new_entry->visible = FALSE;
  g_strdown(new_entry->search_filename);
  
  if( e->title != NULL )
    new_entry->title = g_strdup(e->title);
  else {
    struct tags_entry *tags_entry;
    new_entry->title = g_strdup(strrchr(e->filename, (int)'/')+1);
    //    DEBUG_MSG("Title is NULL, looking in hash");

    pthread_mutex_lock(&tags_mutex);
    tags_entry = g_hash_table_lookup(tags, e->filename);

    if( tags_entry != NULL ) {
      DEBUG_MSG("Found %s in hash", e->filename);
      new_entry->title = g_strdup(tags_entry->title);
      tags_entry->used = TRUE;
      tags_entry->used_by = new_entry;
    } else {
      DEBUG_MSG("Not found, adding to info queue");
      info_enqueue(e->filename);
    }
    pthread_mutex_unlock(&tags_mutex);
  }
  
  new_entry->search_title = g_strdup(new_entry->title);    
  g_strdown(new_entry->search_title);
  
  pthread_mutex_lock(&files_mutex);
  files = g_list_append(files, new_entry); 
  pthread_mutex_unlock(&files_mutex);
}
