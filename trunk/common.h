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

#ifndef COMMON_H
#define COMMON_H 1

#include "config.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "bmp/plugin.h"
#include "bmp/configfile.h"
#include "bmp/beepctrl.h"
#include "bmp/formatter.h"
#include "bmp/titlestring.h"
#include "bmp/vfs.h"

#ifdef DEBUG
#warning "Debugging enabled"
#define DEBUG_MSG(msg, ...) do { fprintf(stderr, "%s:%i :: ", __FILE__, __LINE__); fprintf(stderr, msg, ## __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); } while(0)
#define DEBUG_N_MSG(msg, ...) do { fprintf(stderr, "%s:%i :: ", __FILE__, __LINE__); fprintf(stderr, msg, ## __VA_ARGS__); fflush(stderr); } while(0)
#else
#define DEBUG_MSG(msg, ...)
#endif

/**** VARIABLES ****/
extern gboolean loading;
extern pthread_mutex_t loading_mutex;

extern gboolean running;

// Keep track of all the threads we have launched
extern int threads;

// Mutex for the threads variable
extern pthread_mutex_t threads_mutex;

/* Are we about to quit? */
extern gboolean quit;

extern gboolean update_display;
extern pthread_mutex_t update_display_mutex;

extern pthread_t load_thread;
extern pthread_t info_thread;
extern pthread_t remote_playlist_thread;
extern pthread_mutex_t lookup_list_mutex;
extern pthread_mutex_t quit_mutex;
extern pthread_cond_t lookup_list_cond;
extern gboolean random_song;
extern gboolean auto_clear_tags;
extern gint random_song_change;

/* Some debugging variables */
extern gboolean cq_running;
extern gboolean info_running;

/**** FUNCTIONS ****/

/** XMMS **/
/* Some prototypes for some functions related to the input plugins*/
gboolean input_check_file(const gchar * filename, gboolean show_warning);
void input_get_song_info(gchar * filename, gchar ** title, gint * length);

/** library.c **/
/* Updates the visual list (does filtering too)*/
extern void update_list(void);
extern void init(void);
extern void cleanup(void);
extern void init_timer(void);
extern void done_timer(void);

/** gui_configure.h **/
extern void gui_init_configure();
extern void gui_done_configure();

/**** XMMS GeneralPlugin variable cue_gp *****/
extern GeneralPlugin cue_gp;

#endif
