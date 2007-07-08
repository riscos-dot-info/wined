/* stats.h */

/* Handles dynamically updated statistics windows */

#ifndef __wined_stats_h
#define __wined_stats_h

#include "tempdefs.h"

/* Load template */
void stats_init(void);

/* Create & open stats window for a browser */
void stats_open(browser_fileinfo *browser);

/* Destroy stats window */
void stats_close(browser_fileinfo *browser);

/* Updtae stats display */
void stats_update(browser_fileinfo *browser);

#endif
