/* wadiag.h */

/* Handles work area editing dialogue box */

#ifndef __wined_wadiag_h
#define __wined_wadiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void wadiag_init(void);

/* Open wadiag box */
void wadiag_open(browser_winentry *winentry);

/* Close wadiag */
void wadiag_close(void);

/* winentry being edited */
extern browser_winentry *wadiag_winentry;

#endif
