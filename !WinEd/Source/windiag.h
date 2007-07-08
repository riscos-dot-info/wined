/* windiag.h */

/* Handles window editing dialogue box */

#ifndef __wined_windiag_h
#define __wined_windiag_h

#include "choices.h"
#include "tempdefs.h"

/* Load dialogue boxes */
void windiag_init(void);

/* Open windiag box */
void windiag_open(browser_winentry *winentry);

/* Close windiag */
void windiag_close(void);

/* Respond to changes in choices */
void windiag_responder(choices_str *old, choices_str *new_ch);

/* winentry being edited */
extern browser_winentry *windiag_winentry;

#endif
