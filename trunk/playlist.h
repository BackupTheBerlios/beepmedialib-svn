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

/*
  playlist.h -- Playlist mangement
*/
#ifndef PLAYLIST_H
#define PLAYLIST_H 1

#include "common.h"
#include "library.h"

/**** Structures ****/
/* The entry that we save in out GList of files*/
struct entry {
  gchar *filename;
  gchar *title;

  /* Main path this file belongs to*/
  gchar *path;
  
  /* We also store the title in lowercase to make comparison easier*/
  gchar *search_title;
  gchar *search_filename;

  gboolean selected;
  gboolean visible;
  int row;
};

/* tags structure */
struct tags_entry {
  gchar *filename;
  gchar *title;

  gboolean used;
  GList *used_by;
};

/**** Variables ****/
// GList that contains the playlist
extern GList *files;
extern pthread_mutex_t files_mutex;

extern GList *folders;
extern pthread_mutex_t folders_mutex;

extern GHashTable *tags;
extern pthread_mutex_t tags_mutex;

/**** Functions ****/
void add_folder(gchar *folder);
void *load_list(void *a);
void save_list(gpointer data);
void free_files(void);
void free_folders(void);
void free_tags(void);

/* Remove id3 information from memory that is not used */
void cleanup_tags(void);

/* Save/load the tags "library" to/from filename*/
void save_tags(gchar *filename);
void load_tags(gchar *filename);

void add_to_playlist_absolute(gint n);
void load_folders(gboolean clear_tags);

gint compare(gpointer a, gpointer b);
void sort_files();

#endif
