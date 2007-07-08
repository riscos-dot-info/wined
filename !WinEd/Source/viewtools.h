/* viewtools.h */

/* Handles viewer tool pane */

#ifndef __wined_viewtools_h
#define __wined_viewtools_h

#include "tempdefs.h"

/* Load template */
void viewtools_init(void);

/* Create a new tool pane (its handle is stored in winentry) */
void viewtools_newpane(browser_winentry *winentry);

/* Open pane, given openblock of parent */
void viewtools_open(window_openblock *openblock,window_handle pane);

/* Close and delete tool pane */
void viewtools_deletepane(browser_winentry *winentry);

/* Unshade tools */
void viewtools_unshade(window_handle pane);

/* Shade appropriate tools if no selection */
void viewtools_shadenosel(window_handle pane);

/* Shade appropriate tools if one selection */
void viewtools_shadeonesel(window_handle pane);

/* Shade appropriate tools depending on selection */
void viewtools_shadeapp(browser_winentry *winentry);

#endif
