/* shortcuts.h */

/* Deals with the adjust-click "shortcuts" iconbar menu */


#ifndef __wined_shortcuts_h
#define __wined_shortcuts_h

#include "common.h"

#define shortcut_VALUELEN 128

/* Generate & open shortcuts menu */
BOOL shortcuts_createmenu(int mousex);

#endif
