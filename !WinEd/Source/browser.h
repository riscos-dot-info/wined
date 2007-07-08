/* browser.h */
/* Handles browser windows and management of template files */

#ifndef __wined_browser_h
#define __wined_browser_h

#include "DeskLib:Sprite.h"
#include "DeskLib:Wimp.h"

#include "tempdefs.h"

/* Initialise browser system */
void browser_init(void);

/* Open a new browser window */
browser_fileinfo *browser_newbrowser(void);

/* Load a file in a new window; reference is there just for internal
   consistency */
BOOL browser_load(char *filename,int filesize,void *reference);

/* Save a file (in entirety or selection only) */
BOOL browser_save(char *filename,void *ref,BOOL selection);

/* Check not overwriting a different file, then call browser_save */
BOOL browser_save_check(char *filename,void *ref,BOOL selection);

/* Warns of unsaved data or quits */
void browser_preselfquit(void);

/* Returns size of file for datatrans */
int browser_estsize(browser_fileinfo *);

/* Sprites for tool panes */
extern sprite_area browser_sprites;

/* 'Commands' available from tool bar */
void browcom_create(BOOL submenu, int x, int y, void *ref);
	/* To open 'create' dboxette */
void browcom_copy(BOOL submenu, int x, int y, void *ref);
	/* To open 'copy' dboxette */
void browcom_rename(BOOL submenu, int x, int y, void *ref);
	/* To open 'rename' dboxette */
void browcom_selall(browser_fileinfo *browser);
void browcom_clearsel(void);
void browser_clearselection(void);
void browcom_save(browser_fileinfo *browser,int x,int y,BOOL leaf,BOOL seln);
  /* Coords are where to open dialogue, leaf is whether opening from menu */
void browcom_export(browser_fileinfo *browser,int x,int y,BOOL leaf);
void browcom_delete(browser_fileinfo *browser);
void browcom_stats(browser_fileinfo *browser);
void browcom_view(browser_fileinfo *browser,BOOL editable);

/* Whether quit is desired */
extern BOOL browser_userquit;

extern BOOL browser_selection_withmenu;

/* overwrite window icon numbers */
#define overwrite_OVERWRITE 1
#define overwrite_CANCEL 2

#endif
