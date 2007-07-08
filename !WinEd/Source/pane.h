/* pane.h */

/* Handles panes */

#ifndef __wined_pane_h
#define __wined_pane_h

#include "DeskLib:Wimp.h"

/* Find offset of pane from parent (call immediately after creating) */
void pane_findoffset(window_handle parent,window_handle pane,
     		     wimp_point *offset);

/* Open parent at openblock with pane in front of it */
void pane_open(window_handle parent,window_handle pane,wimp_point *offset,
     	       window_openblock *openblock,BOOL resetscroll);

#endif
