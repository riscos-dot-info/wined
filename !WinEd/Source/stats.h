/* stats.h */

/* Handles dynamically updated statistics windows */

#ifndef __wined_stats_h
#define __wined_stats_h

#include "tempdefs.h"

/* Load template */
void stats_init(void);

/**
 * Create & open stats window for a browser.
 *
 * If the winentry index pointer is not NULL on return, the returned pointer
 * is to an old index which the caller should free().
 *
 * \param *browser    Pointer to the browser instance for which to open the
 *                    stats window.
 * \param ***index    Pointer to a pointer to the winentry index which the
 *                    stats window should use.
 */

void stats_open(browser_fileinfo *browser, browser_winentry ***index);

/**
 * Close & delete the stats window for a browser.
 *
 * The stats_index is freed as part of this action.
 *
 * \param *browser    Pointer to the browser instance for which the stats
 *                    window is to be closed.
 */
void stats_close(browser_fileinfo *browser);

/**
 * Update the winentry index for a stats window.
 *
 * If the winentry index pointer is not NULL on return, the returned pointer
 * is to the old index which the caller should free().
 *
 * \param *browser    Pointer to the browser instance for which to update the
 *                    stats window.
 * \param ***index    Pointer to a pointer to the winentry index which the
 *                    stats window should use from now on.
 */
void stats_updateindex(browser_fileinfo *browser, browser_winentry ***index);

/**
 * Recalculate the statistics for a stats window, and force a redraw.
 *
 * \param *browser    Pointer to the browser instance for which to update the
 *                    stats window.
 */
void stats_update(browser_fileinfo *browser);

#endif
