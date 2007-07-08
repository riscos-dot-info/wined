/* multiicon.h */

/* Handles multiple icon flag editing dialogue box */

#ifndef __wined_multiicon_h
#define __wined_multiicon_h

#include "tempdefs.h"

/* Load dialogue box */
void multiicon_init(void);

/* Open dialogue box */
void multiicon_open(browser_winentry *winentry);

/* Close dialogue */
void multiicon_close(void);

/* winentry being edited */
extern browser_winentry *multiicon_winentry;

#endif
