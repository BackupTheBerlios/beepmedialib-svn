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

#ifndef LIBRARY_H
#define LIBRARY_H 1

#include "common.h"
#include "playlist.h"
#include "command_queue.h"
#include "gui_playlist.h"

// Check if we are in the end of the playlist every half second (500msecs)
#define TIMEOUT 500

/**** Variables ****/


/**** Our own prototypes ****/

/* Adds the filename to the cuelist (puts an add_file request on the command
   queue */
void add_file(gchar *filename);

/* Adds an directory (calls add_file for each file)*/
void add_directory(gchar *dir);

/* Playlist chekker (timeout function) */
gint playlist_check(gpointer data);

/*** info thread related stuff ***/
/* Put a filename on the queue, for which info has to be read */
void info_enqueue(gchar *filename);
/* the function which runs in the info_thread  */
void *info_thread_func(void *a);

#endif
