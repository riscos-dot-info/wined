/* coorddiag.h */

/* Handlesicon spacing dialogue box */

#ifndef __wined_coorddiag_h
#define __wined_coorddiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void coorddiag_init(void);

/* Open coorddiag box */
void coorddiag_open(browser_winentry *winentry);

/* Close coorddiag */
void coorddiag_close(void);

/* winentry being edited */
extern browser_winentry *coorddiag_winentry;

#endif
