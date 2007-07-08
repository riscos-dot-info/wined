/* banner.h */

/* Handles banner/registration details */

#ifndef __wined_banner_h
#define __wined_banner_h

/* Load banner template */
void banner_init(void);

/* Checks user file; if not registered loads banner template and displays it,
   otherwise updates username */
void banner_check(char *username);

#endif
