/* visarea.h */

/* Handlesicon spacing dialogue box */

#ifndef __wined_visarea_h
#define __wined_visarea_h

#include "tempdefs.h"

/* Load dialogue boxes */
void visarea_init(void);

/* Open visarea box */
void visarea_open(browser_winentry *winentry);

/* Close visarea */
void visarea_close(void);

/* winentry being edited */
extern browser_winentry *visarea_winentry;

#endif
