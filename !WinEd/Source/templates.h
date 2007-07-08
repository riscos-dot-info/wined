/* templates.h */
/* Template loading and memory allocation */

#ifndef __wined_templates_h
#define __wined_templates_h

#include "DeskLib:Wimp.h"

/* Loads a template definition, returns pointer to it which can be used to
   free() the memory used. 'indbuffer' and 'indsize' are used to return the
   location and size of the indirected data, but are ignored if NULL. Similarly
   for 'index' (0 assumed). If 'font' is NULL, -1 is used.
*/
window_block *templates_load(char *name,int *index,font_array *font,
                             char **indbuffer,int *indsize);

#endif
