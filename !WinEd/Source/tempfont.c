/* tempfont.c */

#include <stdio.h>

#include "Mem.h"
#include "DeskLib:Str.h"

#include "globals.h"
#include "tempfont.h"

/* Amend all a browser's icons' font handles after a font has been deleted.
   All fontinfo's after 'deleted' should have been shifted down to fill
   gap left by deleted font, so what the function does is decrement all
   font handles higher than this */
void tempfont_amendhandles(browser_fileinfo *browser,int deleted);

/*
void tempfont_diagnose(browser_fileinfo *browser)
{
  int index;
  for (index = 0;index < browser->numfonts;index++)
    fprintf(stderr,"%d\t%d\t%s\n",index+1,browser->fontcount[index+1],
    	browser->fontinfo[index].name);
}
*/

int tempfont_findfont(browser_fileinfo *browser,template_fontinfo *fontinfo)
{
  int index;

  /* If no fonts already, make for first time */
  if (browser->numfonts == 0)
  {
    if (!flex_alloc((flex_ptr) &browser->fontinfo,
                   sizeof(template_fontinfo)))
    {
      MsgTrans_Report(messages,"FontMem",FALSE);
      return 0;
    }
    browser->fontinfo[0] = *fontinfo;
    browser->fontcount[1] = 1;
/*Error_Report(0, "Made first font");*/
    return browser->numfonts = 1;
  }

  /* Look for match; note returned font handle is index + 1 */
  for (index = 0;index < browser->numfonts;index++)
    if (!strcmpcr(browser->fontinfo[index].name,fontinfo->name) &&
        browser->fontinfo[index].size.x == fontinfo->size.x &&
        browser->fontinfo[index].size.y == fontinfo->size.y)
    {
      browser->fontcount[++index]++;
/*Error_Report(0, "Found matching font");*/
      return index;
    }
  /* Not found, make it */
  if (browser->numfonts >= 255)
  {
    MsgTrans_Report(messages,"TMFonts",FALSE);
    return 0;
  }
  if (!flex_extend((flex_ptr) &browser->fontinfo,
      		     flex_size((flex_ptr) &browser->fontinfo) +
      		     sizeof(template_fontinfo)))
  {
    MsgTrans_Report(messages,"FontMem",FALSE);
    return 0;
  }
  /* Old browser->numfonts indexes new fontinfo entry; new
     browser->numfonts is font handle */
  browser->fontinfo[browser->numfonts] = *fontinfo;
  browser->fontcount[++browser->numfonts] = 1;
/*Error_Report(0, "Adding new font");*/
  return browser->numfonts;
}

void tempfont_amendhandles(browser_fileinfo *browser,int deleted)
{
  browser_winentry *winentry;
  int icon;

  for (winentry = LinkList_NextItem(&browser->winlist);
       winentry; winentry = LinkList_NextItem(winentry))
  {
    /* Title */
    if (winentry->window->window.titleflags.data.font &&
    	winentry->window->window.titleflags.font.handle > deleted)
      winentry->window->window.titleflags.font.handle--;
    /* Other icons */
    for (icon = 0; icon < winentry->window->window.numicons; icon++)
      if (winentry->window->icon[icon].flags.data.font &&
      	  winentry->window->icon[icon].flags.font.handle > deleted)
      	winentry->window->icon[icon].flags.font.handle--;
  }
}

BOOL tempfont_losefont(browser_fileinfo *browser,int font)
{
  BOOL amend = FALSE;
  if (!--browser->fontcount[font])
  {
    if (!--browser->numfonts)
    {
      flex_free((flex_ptr) &browser->fontinfo);
      browser->fontinfo = 0;
    }
    else
    {
      int index;
      flex_midextend((flex_ptr) &browser->fontinfo,
      		    (int) sizeof(template_fontinfo) * font,
      		    (int) -sizeof(template_fontinfo));
      for (index = font; index <= browser->numfonts; index++)
        browser->fontcount[index] = browser->fontcount[index+1];
      tempfont_amendhandles(browser,font);
      amend = TRUE;
    }
  }
  return amend;
}

void tempfont_losewindow(browser_fileinfo *browser,
     			 browser_winentry *winentry)
{
  int icon;

  /* Title */
  if (winentry->window->window.titleflags.data.font)
    tempfont_losefont(browser,
    		      winentry->window->window.titleflags.font.handle);
  /* Other icons */
  for (icon = 0; icon < winentry->window->window.numicons; icon++)
    if (winentry->window->icon[icon].flags.data.font)
      tempfont_losefont(browser,
      			winentry->window->icon[icon].flags.font.handle);
}
