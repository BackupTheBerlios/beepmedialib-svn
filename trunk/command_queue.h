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
  Command queue for the playlist.
*/
#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H 1

#include "common.h"

/**** Command queue related stuff ****/
#define CQ_ADD 0x1
#define CQ_REMOVE 0x2
#define CQ_UPDATE 0x3
#define CQ_LISTUPDATE 0x4

/* The command queue will free all chars in this strucure, so be sure
   that they are allocated especially for this strucure.
   (That is unless they are NULL, in which case they are ignored)
   Notice, that the cq_enqueue() function takes care of this for you.
 */
struct cq_list_entry {
  unsigned char type;
  gchar *filename;
  gchar *title;
};


// The queue as a GList (double linked list)
extern GList *cq_list;

/* Stuff for the command queue thread*/
extern pthread_t cq_thread;
extern pthread_mutex_t cq_list_mutex;
extern pthread_cond_t cq_list_cond;

/* The only interesting function in this file, the enqueue function ;)*/
void cq_enqueue(unsigned char type, gchar *filename, gchar *title);

/* The threads main function*/
void *cq_thread_func(void *a);

/* The next three functions are called by the cq_thread_func to do the actual
   updating.
   They should probably be placed together with the rest of the playlist code
   in playlist.c.
*/
void _cq_add(struct cq_list_entry *e);
void _cq_update(struct cq_list_entry *e);
void _cq_remove(struct cq_list_entry *e);

#endif
