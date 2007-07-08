/* rszdiag.h */

/* Handlesicon spacing dialogue box */

#ifndef __wined_rszdiag_h
#define __wined_rszdiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void rszdiag_init(void);

/* Open rszdiag box */
void rszdiag_open(browser_winentry *winentry);

/* Close rszdiag */
void rszdiag_close(void);

/* winentry being edited */
extern browser_winentry *rszdiag_winentry;

#endif
