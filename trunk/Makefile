PLUGIN = libMediaLibrary.so
CC = gcc
OBJ = command_queue.o gui_configure.o gui_playlist.o library.o playlist.o
FLAGS = -Wall -rdynamic -fPIC #-g -DDEBUG
LIBS = -shared -lpthread
GTKFLAGS = `pkg-config gtk+-2.0 --cflags`
GTKLIBS = `pkg-config gtk+-2.0 --libs`
XMMSFLAGS = `pkg-config bmp --cflags`
XMMSLIBS = `pkg-config bmp --libs`

#PREFIX = `xmms-config --prefix`
PLUGIN_INSTALL_DIR = `pkg-config bmp --variable=general_plugin_dir`
PLUGIN_INSTALL_DIR_HOME = $(HOME)/.bmp/Plugins/General

all: $(PLUGIN)

$(OBJ): command_queue.c gui_configure.c gui_playlist.c library.c playlist.c
	$(CC) $(FLAGS) $(GTKFLAGS) $(XMMSFLAGS) -c $*.c

$(PLUGIN): $(OBJ)
	$(CC) -o $(PLUGIN) $(GTKLIBS) $(XMMSLIBS) $(LIBS) $(OBJ)

install: $(PLUGIN)
	 install -D $(PLUGIN) $(PLUGIN_INSTALL_DIR)/$(PLUGIN) 
	 
install-home: $(PLUGIN)
	 install -D $(PLUGIN) $(PLUGIN_INSTALL_DIR_HOME)/$(PLUGIN) 

uninstall:
	  rm -f $(PLUGIN_INSTALL_DIR)/$(PLUGIN)

uninstall-home:
	  rm -f $(PLUGIN_INSTALL_DIR_HOME)/$(PLUGIN)

clean:
	rm -f $(PLUGIN) $(REMOTE) $(OBJ) *~
