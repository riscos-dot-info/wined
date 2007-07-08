/* spcdiag.h */

/* Handlesicon spacing dialogue box */

#ifndef __wined_spcdiag_h
#define __wined_spcdiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void spcdiag_init(void);

/* Open spcdiag box */
void spcdiag_open(browser_winentry *winentry);

/* Close spcdiag */
void spcdiag_close(void);

/* winentry being edited */
extern browser_winentry *spcdiag_winentry;

#endif
