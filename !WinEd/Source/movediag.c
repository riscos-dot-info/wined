/* movediag.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Hourglass.h"

#include "movediag.h"
#include "diagutils.h"
#include "icnedit.h"
#include "round.h"
#include "title.h"

#define SMALLEST 1

typedef enum {
  movediag_HORIZONTAL = 2,
  movediag_DECHORIZONTAL,
  movediag_INCHORIZONTAL,
  movediag_VERTICAL,
  movediag_DECVERTICAL,
  movediag_INCVERTICAL,
  movediag_OK = 8,
  movediag_CANCEL
} movediag_icons;

/* Button handlers */
BOOL movediag_clickok(event_pollblock *event,void *reference);
BOOL movediag_clickcancel(event_pollblock *event,void *reference);
BOOL movediag_keypress(event_pollblock *event,void *reference);


window_handle movediag_window;
browser_winentry *movediag_winentry;


void movediag_init()
{
  window_block *templat;

  templat = templates_load("Move",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&movediag_window));
  movediag_winentry = 0;
  Event_Claim(event_OPEN,movediag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,movediag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,movediag_window,movediag_OK,movediag_clickok,0);
  Event_Claim(event_CLICK,movediag_window,movediag_CANCEL,
  	movediag_clickcancel,0);
  Event_Claim(event_KEY,movediag_window,event_ANY,movediag_keypress,0);
  diagutils_bumpers(movediag_window,movediag_HORIZONTAL,-99999996,99999996,-1);
  diagutils_bumpers(movediag_window,movediag_VERTICAL,-99999996,99999996,-1);
  help_claim_window(movediag_window,"MOV");
}

void movediag_open(browser_winentry *winentry)
{
  char numstring[8];
  char buffer[40];
  icon_handle selected = Icon_WhichRadioInEsg(winentry->handle,1);

  movediag_winentry = winentry;
//  Icon_SetInteger(coorddiag_window,coorddiag_LEFT,
//  		  winentry->window->icon[selected].workarearect.min.x);
//  Icon_SetInteger(coorddiag_window,coorddiag_TOP,
//  		  winentry->window->icon[selected].workarearect.max.y);

  Window_Show(movediag_window,open_UNDERPOINTER);
  Icon_SetCaret(movediag_window,movediag_HORIZONTAL);
}

BOOL movediag_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock ev;

    ev.data.mouse.button.value = button_SELECT;
    return movediag_clickok(&ev,reference);
  }
  else if (event->data.key.code == 27)
    movediag_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

BOOL movediag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    movediag_close();
  return TRUE;
}

BOOL movediag_clickok(event_pollblock *event,void *reference)
{
  int icon;
  int width, height;
  wimp_rect newrect;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

//  icon = Icon_WhichRadioInEsg(coorddiag_winentry->handle,1);
//  newrect.min.x = Icon_GetInteger(coorddiag_window,coorddiag_LEFT);
//  newrect.max.y = Icon_GetInteger(coorddiag_window,coorddiag_TOP);
//  width = Icon_GetInteger(coorddiag_window,coorddiag_WIDTH);
//  height = Icon_GetInteger(coorddiag_window,coorddiag_HEIGHT);
//  if (width < SMALLEST) width = SMALLEST;
//  if (height < SMALLEST) height = SMALLEST;
//  newrect.max.x = newrect.min.x + width;
//  newrect.min.y = newrect.max.y - height;
//  /* Remove round_down_box as we allow this window to be used to make smaller-than-4 size adjustments
//  round_down_box(round_STEP_FOUR, &newrect); */
//  icnedit_moveicon(coorddiag_winentry,icon,&newrect);

  browser_settitle(movediag_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    movediag_close();

  return TRUE;
}

void movediag_close()
{
  return_caret(movediag_window, movediag_winentry->handle);
  Wimp_CloseWindow(movediag_window);
}
