/* alndiag.h */

/* Handles icon alignment dialogue box */

#ifndef __wined_alndiag_h
#define __wined_alndiag_h

#include "tempdefs.h"

/* Load dialogue boxes */
void alndiag_init(void);

/* Open alndiag box */
void alndiag_open(browser_winentry *winentry);

/* Close alndiag */
void alndiag_close(void);

/* winentry being edited */
extern browser_winentry *alndiag_winentry;

#endif
