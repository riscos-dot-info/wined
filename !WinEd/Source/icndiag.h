/* icndiag.h */

/* Handles icon editing dialogue box */

#ifndef __wined_icndiag_h
#define __wined_icndiag_h

#include "choices.h"
#include "tempdefs.h"

/* Load dialogue boxes */
void icndiag_init(void);

/* Open icndiag box; icon = -1 for title */
void icndiag_open(browser_winentry *winentry,icon_handle icon);

/* Close icndiag */
void icndiag_close(void);

/* Respond to changes in choices */
void icndiag_responder(choices_str *old_ch, choices_str *new_ch);

/* winentry and icon being edited */
extern browser_winentry *icndiag_winentry;
extern icon_handle icndiag_icon;

#endif
