/* terminator.c */

#include <limits.h>

#include "tempdefs.h"

static void term_string(char *str, char term, int len)
{
  int i;

/*Error_Report(0, str);*/
  for (i = 0; i < len; i++)
  {
    if (str[i] < 32)
    {
      str[i] = term;
      return;
    }
  }
}

static void ft_icon(browser_winblock *winblock, char term,
	icon_data *data, icon_flags *flags)
{

  if (flags->data.indirected)
  {
    if (flags->data.text ||
    	(flags->data.sprite && data->indirectsprite.nameisname))
    {
      term_string((char *) winblock + (int) data->indirecttext.buffer, term,
      	data->indirecttext.bufflen);
    }
    if (flags->data.text)
    {
      term_string((char *) winblock + (int) data->indirecttext.validstring,
      	term, INT_MAX);
    }
  }
  else
    term_string(data->text, term, 12);
}

void force_terminators(browser_fileinfo *browser,char term)
{
  browser_winentry *winentry;
  int i;

  for (winentry = LinkList_NextItem(&browser->winlist); winentry;
       winentry = LinkList_NextItem(winentry))
  {
    icon_block *ib;

    /* Template identifier */
/*Error_Report(0, "ID %s", winentry->identifier);*/
    term_string(winentry->identifier, term, 12);

    /* Window title icon */
/*Error_Report(0, "Title icon");*/
    ft_icon(winentry->window, term,
    	&winentry->window->window.title, &winentry->window->window.titleflags);

    /* Other icons */
    for (ib = winentry->window->icon, i = 0;
    	i < winentry->window->window.numicons;
    	i++, ib++)
    {
/*Error_Report(0, "Icon %d/%d", i, winentry->window->window.numicons);*/
      ft_icon(winentry->window, term, &ib->data, &ib->flags);
    }
  }

  /* Font info */
  for (i = 0; i < browser->numfonts; i++)
  {
    term_string(browser->fontinfo[i].name, term, 40);
  }
}
