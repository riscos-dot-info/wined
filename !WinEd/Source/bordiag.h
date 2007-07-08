/* bordiag.h */

/* Handles bordering dialogue box */

#ifndef __wined_bordiag_h
#define __wined_bordiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void bordiag_init(void);

/* Open bordiag box */
void bordiag_open(browser_winentry *winentry);

/* Close bordiag */
void bordiag_close(void);

/* winentry being edited */
extern browser_winentry *bordiag_winentry;

#endif
