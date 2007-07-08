/* icnedit.h */

/* Handles editing of icons.
   Font handling routines here are concerned with real fonts, whereas
   tempfont contains routines for logical fonts. */

#ifndef __wined_icnedit_h
#define __wined_icnedit_h

#include "DeskLib:Font.h"

#include "minsize.h"
#include "tempdefs.h"


/* Find real font handle for given font name/size and update font array */
font_handle icnedit_findfont(template_fontinfo *fontinfo,
	    		     font_array **fontarray);

/* Lose font and update font array */
void icnedit_losefont(font_handle font,font_array **fontarray);

/* Free an icon's physical indirected data; flags and data are given
   separately so it can be used with title */
void icnedit_freeindirected(icon_flags *flags,icon_data *data);

/* Process an icon block ready for display ie create fonts, indirected data.
   Pass NULL workarearect for title.
   Returns FALSE if insufficient memory for indirected data */
BOOL icnedit_processicon(browser_winentry *winentry,
     			 icon_flags *flags,icon_data *data,
     			 wimp_rect *workarearect,BOOL editable);

/* Does the opposite of icnedit_processicon() ie lose physical font and
   indirected data */
void icnedit_unprocessicon(browser_winentry *winentry,
     			   icon_flags *flags,icon_data *data);

/* Changes offset pointers for all a winentry's indirected icons.
   Offsets >= 'after' have 'by' added */
void icnedit_updatepointers(browser_winentry *winentry,int after,int by);

/* Delete an icon's logical indirected data and fix all affected pointers
   icon = -1 for title */
void icnedit_deleteindirected(browser_winentry *winentry,int icon);

/* Change an existing icon's data/flags to new values, fix indirected stuff,
   update physical icon etc. Not suitable for title.
   Any indirected data pointers in 'data' should point to caller's disposable
   buffers, data is copied to new buffers.
   Font handle must be a logical font handle, already set up before calling
 */
void icnedit_changedata(browser_winentry *winentry,int icon,
     			icon_block *data);

/* Create a new logical icon. Entry conditions as above. Returns new icon's
   logical handle */
int icnedit_create(browser_winentry *winentry,icon_block *data);

/* Delete a logical icon and fix all affected pointers, font array */
void icnedit_deleteicon(browser_winentry *winentry,int icon);

/* Copy a logical icon, optionally to a new winentry */
/* Returns the icon handle of the new icon */
icon_handle icnedit_copyicon(browser_winentry *source,int icon,
     		      browser_winentry *dest,wimp_rect *bounds);

/* Move/resize an icon (pass logical icon) */
void icnedit_moveicon(browser_winentry *winentry,int icon,wimp_rect *box);

/* If new icon data is indirected, add its strings to winentry */
void icnedit_processnewicondata(browser_winentry *winentry,
     				icon_flags *flags,icon_data *data);

/* Returns index to '6' if a logical icon's validation string contains R6;
   otherwise returns NULL */
char *icnedit_valid6(browser_winentry *winentry,int icon);

/* Returns an icon's border type */
int icnedit_bordertype(browser_winentry *winentry, int icon);

/* Width of an icon's border */
int icnedit_borderwidth(browser_winentry *winentry, int icon);

/* Forces redraw of a logical icon, including extended border if R6 */
void icnedit_forceredraw(browser_winentry *winentry,int icon);

/* Add string to logical indirected data, returning index */
int icnedit_addindirectstring(browser_winentry *winentry,char *string);

/* Remove a string from logical indirected data */
void icnedit_removeindirectstring(browser_winentry *winentry,int index);

/* Return how much 'expanded' indirected data is needed by icon data */
int icnedit_count_indirected(browser_winentry *winentry,
    			     icon_data *data,icon_flags *flags);

/* Modify icon flags for use in an editable window */
void icnedit_makeeditableflags(browser_winentry *winentry, icon_flags *flags);

/* Margin needed by L validated icons */
#define LMARGIN 56



#endif
