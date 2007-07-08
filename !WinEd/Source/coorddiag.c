/* coorddiag.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Hourglass.h"

#include "coorddiag.h"
#include "diagutils.h"
#include "icnedit.h"
#include "round.h"
#include "title.h"

#define SMALLEST 1

typedef enum {
  coorddiag_LEFT = 2,
  coorddiag_DECLEFT,
  coorddiag_INCLEFT,
  coorddiag_TOP,
  coorddiag_DECTOP,
  coorddiag_INCTOP,
  coorddiag_WIDTH = 10,
  coorddiag_DECWIDTH,
  coorddiag_INCWIDTH,
  coorddiag_HEIGHT,
  coorddiag_DECHEIGHT,
  coorddiag_INCHEIGHT,
  coorddiag_OK = 16,
  coorddiag_CANCEL
} coorddiag_icons;

/* Button handlers */
BOOL coorddiag_clickok(event_pollblock *event,void *reference);
BOOL coorddiag_clickcancel(event_pollblock *event,void *reference);
BOOL coorddiag_keypress(event_pollblock *event,void *reference);


window_handle coorddiag_window;
browser_winentry *coorddiag_winentry;


void coorddiag_init()
{
  window_block *templat;

  templat = templates_load("Coords",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&coorddiag_window));
  coorddiag_winentry = 0;
  Event_Claim(event_OPEN,coorddiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,coorddiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,coorddiag_window,coorddiag_OK,coorddiag_clickok,0);
  Event_Claim(event_CLICK,coorddiag_window,coorddiag_CANCEL,
  	coorddiag_clickcancel,0);
  Event_Claim(event_KEY,coorddiag_window,event_ANY,coorddiag_keypress,0);
  diagutils_bumpers(coorddiag_window,coorddiag_LEFT,0,99999996,-1);
  diagutils_bumpers(coorddiag_window,coorddiag_TOP,-99999996,0,-1);
  diagutils_bumpers(coorddiag_window,coorddiag_WIDTH,4,99999996,-1);
  diagutils_bumpers(coorddiag_window,coorddiag_HEIGHT,4,99999996,-1);
  help_claim_window(coorddiag_window,"CRD");
}

void coorddiag_open(browser_winentry *winentry)
{
  char numstring[8];
  char buffer[40];
  icon_handle selected = Icon_WhichRadioInEsg(winentry->handle,1);

  coorddiag_winentry = winentry;
  Icon_SetInteger(coorddiag_window,coorddiag_LEFT,
  		  winentry->window->icon[selected].workarearect.min.x);
  Icon_SetInteger(coorddiag_window,coorddiag_TOP,
  		  winentry->window->icon[selected].workarearect.max.y);
  Icon_SetInteger(coorddiag_window,coorddiag_WIDTH,
  		  winentry->window->icon[selected].workarearect.max.x -
  		  winentry->window->icon[selected].workarearect.min.x);
  Icon_SetInteger(coorddiag_window,coorddiag_HEIGHT,
  		  winentry->window->icon[selected].workarearect.max.y -
  		  winentry->window->icon[selected].workarearect.min.y);

  sprintf(numstring,"%d",selected);
  MsgTrans_LookupPS(messages,"CrdTitle",buffer,40,numstring,0,0,0);
  Window_SetTitle(coorddiag_window,buffer);

  Window_Show(coorddiag_window,open_UNDERPOINTER);
  Icon_SetCaret(coorddiag_window,coorddiag_LEFT);
}

BOOL coorddiag_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock ev;

    ev.data.mouse.button.value = button_SELECT;
    return coorddiag_clickok(&ev,reference);
  }
  else if (event->data.key.code == 27)
    coorddiag_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

BOOL coorddiag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    coorddiag_close();
  return TRUE;
}

BOOL coorddiag_clickok(event_pollblock *event,void *reference)
{
  int icon;
  int width, height;
  wimp_rect newrect;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  icon = Icon_WhichRadioInEsg(coorddiag_winentry->handle,1);
  newrect.min.x = Icon_GetInteger(coorddiag_window,coorddiag_LEFT);
  newrect.max.y = Icon_GetInteger(coorddiag_window,coorddiag_TOP);
  width = Icon_GetInteger(coorddiag_window,coorddiag_WIDTH);
  height = Icon_GetInteger(coorddiag_window,coorddiag_HEIGHT);
  if (width < SMALLEST) width = SMALLEST;
  if (height < SMALLEST) height = SMALLEST;
  newrect.max.x = newrect.min.x + width;
  newrect.min.y = newrect.max.y - height;
  /* Remove round_down_box as we allow this window to be used to make smaller-than-4 size adjustments
  round_down_box(&newrect); */
  icnedit_moveicon(coorddiag_winentry,icon,&newrect);

  browser_settitle(coorddiag_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    coorddiag_close();

  return TRUE;
}

void coorddiag_close()
{
  return_caret(coorddiag_window, coorddiag_winentry->handle);
  Wimp_CloseWindow(coorddiag_window);
}
