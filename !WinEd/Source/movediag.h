/* movediag.h */

/* Handles icon movement dialogue box */

#ifndef __wined_movediag_h
#define __wined_movediag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void movediag_init(void);

/* Open movediag box */
void movediag_open(browser_winentry *winentry);

/* Close movediag */
void movediag_close(void);

/* winentry being edited */
extern browser_winentry *movediag_winentry;

#endif
