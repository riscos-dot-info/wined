/* viewer.h */

/* Handles display of preview and editing windows */

#ifndef __wined_viewer_h
#define __wined_viewer_h

#include "choices.h"
#include "tempdefs.h"

/* Create menu etc */
void viewer_init(void);

/* Remove a viewer from the screen and all its temporary data from memory */
void viewer_close(browser_winentry *winentry);

/* The 'editable' switch below is TRUE if the window is for editing */

/* Open a new viewer */
void viewer_open(browser_winentry *winentry,BOOL editable);

/* Clear all icons in any window */
void viewer_clearselection(void);

/* Respond to choices */
void viewer_responder(browser_winentry *winentry,
                      choices_str *old,choices_str *new_ch);

/* Force user sprites on */
void viewer_forceusrsprt(browser_winentry *winentry);

/* Change resolution of fonts after a mode change */
void viewer_changefonts(browser_winentry *winentry);

/* Sets selection state, also temporarily setting fill flag */
void viewer_setselect(browser_winentry *winentry,int icon,BOOL state);

extern BOOL viewer_selection_withmenu;

extern BOOL monitor_dragging;
extern wimp_rect drag_sidemove;


#endif
