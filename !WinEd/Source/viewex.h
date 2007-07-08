/* viewex.h */

/* Holds definitions of a couple of viewer functions for windiag and picker
   to refer back to */

#ifndef __wined_vwrreopen_h
#define __wined_vwrreopen_h

#include "tempdefs.h"

/* Remove a viewer from the screen and all its temporary data from memory */
void viewer_close(browser_winentry *winentry);

/* Reopen viewer after editing, processing new title data if necessary.
   If title indirected data has changed the data must already be set up in
   winentry with relevant offsets in 'block'. Similarly for fonts, block
   must contain logical font handle */
void viewer_reopen(browser_winentry *winentry,BOOL newtitle);

/* mallocs a winblock the right size, and makes it suitable for opening */
BOOL viewer_makewinblock(browser_winentry *winentry,
     			 BOOL wineditable,BOOL icneditable,
     			 browser_winblock **winblock);

/* Create a window from a browser_winblock */
BOOL viewer_createwindow(browser_winentry *winentry,
     			 browser_winblock *winblock);

/* Claim events for editable window */
void viewer_claimeditevents(browser_winentry *winentry);

#endif
