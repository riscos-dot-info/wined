/* fontpick.h

   Handles picking of fonts from a menu. Memory allocation etc for the
   font menu is handled internally

 */


#ifndef __fontpick_h
#define __fontpick_h

#include "DeskLib:Wimp.h"

/* Create a font menu; the string is the identifier of the font
   (as passed to Font_FindFont) to tick
   Returns NULL if fails */
extern menu_ptr fontpick_makemenu(char *fontname);

/* Free memory used by menu tree (and set pointer to NULL) */
extern void fontpick_freemenu(void);

/* Find full string for font selected from menu */
extern void fontpick_findselection(int *selection,char *buffer);

/* Extract font's \F field from full string */
extern void fontpick_findname(char *fullstring,char *buffer);

#endif
