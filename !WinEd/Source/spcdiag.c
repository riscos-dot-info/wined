/* spcdiag.c */

#include "common.h"

#include "DeskLib:Hourglass.h"

#include "diagutils.h"
#include "icnedit.h"
#include "spcdiag.h"
#include "title.h"

typedef enum {
  spcdiag_SPACE,
  spcdiag_CANCEL,
  spcdiag_LEFT,
  spcdiag_RIGHT,
  spcdiag_TOP,
  spcdiag_BOTTOM,
  spcdiag_GAP = 7
} spcdiag_icons;

/* Button handlers */
BOOL spcdiag_clickspace(event_pollblock *event,void *reference);
BOOL spcdiag_clickcancel(event_pollblock *event,void *reference);
BOOL spcdiag_keypress(event_pollblock *event,void *reference);

/* Simple sorts - quicksort not particularly advantageous for such small lists
   so bubble sort is adequate */
void spcdiag_sorthoriz(icon_handle *table,int number);
void spcdiag_sortvert(icon_handle *table,int number);


window_handle spcdiag_window;
browser_winentry *spcdiag_winentry;


void spcdiag_sorthoriz(icon_handle *table,int number)
{
  register int a,b,t;

  for (a = 1; a < number; a++)
    for (b = number - 1; b >= a; b--)
    {
      if (spcdiag_winentry->window->icon[table[b - 1]].workarearect.min.x +
      	  spcdiag_winentry->window->icon[table[b - 1]].workarearect.max.x >
      	  spcdiag_winentry->window->icon[table[b]].workarearect.min.x +
      	  spcdiag_winentry->window->icon[table[b]].workarearect.max.x)
      {
        t = table[b - 1];
        table[b - 1] = table[b];
        table[b] = t;
      }
    }
}

void spcdiag_sortvert(icon_handle *table,int number)
{
  register int a,b,t;

  for (a = 1; a < number; a++)
    for (b = number - 1; b >= a; b--)
    {
      if (spcdiag_winentry->window->icon[table[b - 1]].workarearect.min.y +
      	  spcdiag_winentry->window->icon[table[b - 1]].workarearect.max.y >
      	  spcdiag_winentry->window->icon[table[b]].workarearect.min.y +
      	  spcdiag_winentry->window->icon[table[b]].workarearect.max.y)
      {
        t = table[b - 1];
        table[b - 1] = table[b];
        table[b] = t;
      }
    }
}

void spcdiag_init()
{
  window_block *templat;

  templat = templates_load("Space",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&spcdiag_window));
  spcdiag_winentry = 0;
  Event_Claim(event_OPEN,spcdiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,spcdiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,spcdiag_window,spcdiag_SPACE,spcdiag_clickspace,0);
  Event_Claim(event_CLICK,spcdiag_window,spcdiag_CANCEL,spcdiag_clickcancel,0);
  Event_Claim(event_KEY,spcdiag_window,event_ANY,spcdiag_keypress,0);
  diagutils_bumpers(spcdiag_window,spcdiag_GAP,0,256,-1);
  help_claim_window(spcdiag_window,"SPC");
}

void spcdiag_open(browser_winentry *winentry)
{
  spcdiag_winentry = winentry;
  if (Icon_WhichRadioInEsg(spcdiag_window,1) == -1)
    Icon_Select(spcdiag_window,spcdiag_LEFT);
  Window_Show(spcdiag_window,open_UNDERPOINTER);
  Icon_SetCaret(spcdiag_window,spcdiag_GAP);
}

BOOL spcdiag_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock ev;

    ev.data.mouse.button.value = button_SELECT;
    return spcdiag_clickspace(&ev,reference);
  }
  else if (event->data.key.code == 27)
    spcdiag_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

BOOL spcdiag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    spcdiag_close();
  return TRUE;
}

BOOL spcdiag_clickspace(event_pollblock *event,void *reference)
{
  icon_handle *table; /* Table of selected icons */
  wimp_rect moveto;   /* ...for each icon being moved */
  int size;    	      /*   "   */
  int gap = Icon_GetInteger(spcdiag_window,spcdiag_GAP);
  int selections = count_selections(spcdiag_winentry->handle);
  int icon;
  int lastedge;

  if (selections == 1) /* Space single icon from edge of visible part of WA */
  {
    icon = Icon_WhichRadioInEsg(spcdiag_winentry->handle,1);
    moveto = spcdiag_winentry->window->icon[icon].workarearect;
    switch (Icon_WhichRadioInEsg(spcdiag_window,1))
    {
      case -1:
        Icon_Select(spcdiag_window,spcdiag_LEFT);
      case spcdiag_LEFT:
        size = moveto.max.x - moveto.min.x;
        moveto.min.x = spcdiag_winentry->window->window.workarearect.min.x +
        	       spcdiag_winentry->window->window.scroll.x + gap;
        moveto.max.x = moveto.min.x + size;
        break;
      case spcdiag_RIGHT:
        size = moveto.max.x - moveto.min.x;
        moveto.max.x = spcdiag_winentry->window->window.workarearect.min.x +
        	       spcdiag_winentry->window->window.scroll.x +
        	       spcdiag_winentry->window->window.screenrect.max.x -
        	       spcdiag_winentry->window->window.screenrect.min.x - gap;
        moveto.min.x = moveto.max.x - size;
        break;
      case spcdiag_TOP:
        size = moveto.max.y - moveto.min.y;
        moveto.max.y = spcdiag_winentry->window->window.workarearect.max.y +
        	       spcdiag_winentry->window->window.scroll.y - gap;
        moveto.min.y = moveto.max.y - size;
        break;
      case spcdiag_BOTTOM:
        size = moveto.max.y - moveto.min.y;
        moveto.min.y = spcdiag_winentry->window->window.workarearect.max.y +
        	       spcdiag_winentry->window->window.scroll.y -
        	       spcdiag_winentry->window->window.screenrect.max.y +
        	       spcdiag_winentry->window->window.screenrect.min.y + gap;
        moveto.max.y = moveto.min.y + size;
        break;
    }
    icnedit_moveicon(spcdiag_winentry,icon,&moveto);
  }
  else
  {
    Hourglass_On();
    table = malloc((selections + 1) * sizeof(icon_handle));
    if (!table)
    {
      WinEd_MsgTrans_Report(0,"NoStore",FALSE);
      return TRUE;
    }
    Wimp_WhichIcon(spcdiag_winentry->handle,table,icon_SELECTED,icon_SELECTED);
    switch (Icon_WhichRadioInEsg(spcdiag_window,1))
    {
      case -1:
        Icon_Select(spcdiag_window,spcdiag_LEFT);
      case spcdiag_LEFT:
        spcdiag_sorthoriz(table,selections);
        lastedge = spcdiag_winentry->window->icon[table[0]].workarearect.max.x +
        	   gap;
        for (icon = 1;icon < selections;icon++)
        {
          moveto = spcdiag_winentry->window->icon[table[icon]].workarearect;
          size = moveto.max.x - moveto.min.x;
          moveto.min.x = lastedge;
          moveto.max.x = lastedge + size;
          icnedit_moveicon(spcdiag_winentry,table[icon],&moveto);
          lastedge += size + gap;
        }
        break;
      case spcdiag_RIGHT:
        spcdiag_sorthoriz(table,selections);
        lastedge =
          spcdiag_winentry->window->
            icon[table[selections - 1]].workarearect.min.x - gap;
        for (icon = selections - 2;icon >= 0;icon--)
        {
          moveto = spcdiag_winentry->window->icon[table[icon]].workarearect;
          size = moveto.max.x - moveto.min.x;
          moveto.max.x = lastedge;
          moveto.min.x = lastedge - size;
          icnedit_moveicon(spcdiag_winentry,table[icon],&moveto);
          lastedge -= size + gap;
        }
        break;
      case spcdiag_TOP:
        spcdiag_sortvert(table,selections);
        lastedge =
          spcdiag_winentry->window->
            icon[table[selections - 1]].workarearect.min.y - gap;
        for (icon = selections - 2;icon >= 0;icon--)
        {
          moveto = spcdiag_winentry->window->icon[table[icon]].workarearect;
          size = moveto.max.y - moveto.min.y;
          moveto.max.y = lastedge;
          moveto.min.y = lastedge - size;
          icnedit_moveicon(spcdiag_winentry,table[icon],&moveto);
          lastedge -= size + gap;
        }
        break;
      case spcdiag_BOTTOM:
        spcdiag_sortvert(table,selections);
        lastedge = spcdiag_winentry->window->icon[table[0]].workarearect.max.y +
         	   gap;
        for (icon = 1;icon < selections;icon++)
        {
          moveto = spcdiag_winentry->window->icon[table[icon]].workarearect;
          size = moveto.max.y - moveto.min.y;
          moveto.min.y = lastedge;
          moveto.max.y = lastedge + size;
          icnedit_moveicon(spcdiag_winentry,table[icon],&moveto);
          lastedge += size + gap;
        }
        break;
    }
    Hourglass_Off();
    free(table);
  }

  browser_settitle(spcdiag_winentry->browser,NULL,TRUE);
  if (event->data.mouse.button.data.select)
    spcdiag_close();

  return TRUE;
}

void spcdiag_close()
{
  return_caret(spcdiag_window, spcdiag_winentry->handle);
  Wimp_CloseWindow(spcdiag_window);
}
