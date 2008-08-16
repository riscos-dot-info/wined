/* tempfont.h */

/* Handling of fonts in templates. These routines are concerned with
   'logical' fonts in template definitions rather than real font handles */

/* Note that font handles start from 1, but arrays of font stuff that are
   indexed by handles start from 0, therefore use (handle - 1) */

#ifndef __wined_tempfont_h
#define __wined_tempfont_h

#include "tempdefs.h"

/* Find a font in a browser's font list and increment its usage counter;
   if it doesn't exist, make it */
unsigned int tempfont_findfont(browser_fileinfo *browser,template_fontinfo *fontinfo);

/* When deleting an icon etc from a definition, call this to free its font
   usage; if it's the only icon using the font, it's removed from the
   template's fontinfo and any other icons have their font handles amended
   if necessary, and TRUE is returned so you know you have to do the same for
   any handles held in temporaries */
BOOL tempfont_losefont(browser_fileinfo *browser,int font);

/* Calls tempfont_losefont() for every anti-aliased icon in a window */
void tempfont_losewindow(browser_fileinfo *browser,
                         browser_winentry *winentry);

#endif
