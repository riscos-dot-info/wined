/* browtools.h */

/* Handles browser tool pane */

#ifndef __wined_browtools_h
#define __wined_browtools_h

#include "tempdefs.h"

/* Load template */
void browtools_init(void);

/* Create a new tool pane (its handle is stored in browser) */
void browtools_newpane(browser_fileinfo *browser);

/* Open pane, given openblock of parent */
void browtools_open(window_openblock *openblock,browser_fileinfo *browser,
	BOOL parent);	/* Optionally also (re)open parent */

/* Close and delete tool pane */
void browtools_deletepane(browser_fileinfo *browser);

/* Unshade tools */
void browtools_unshade(window_handle pane);

/* Shade appropriate tools if no selection */
void browtools_shadenosel(window_handle pane);

/* Shade appropriate tools if multiple selection */
void browtools_shademulsel(window_handle pane);

/* Shade appropriate tools depending on selection */
void browtools_shadeapp(browser_fileinfo *browser);

/* Height of pane */
extern int browtools_paneheight;

#endif
