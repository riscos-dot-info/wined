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

/* Default & Current move coordinates */
int movediag_xby = 0;
int movediag_yby = 0;

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
  movediag_winentry = winentry;
  Icon_SetInteger(movediag_window,movediag_HORIZONTAL,movediag_xby);
  Icon_SetInteger(movediag_window,movediag_VERTICAL,movediag_yby);
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
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  movediag_xby = Icon_GetInteger(movediag_window,movediag_HORIZONTAL);
  movediag_yby = Icon_GetInteger(movediag_window,movediag_VERTICAL);
  int selections = count_selections(movediag_winentry->handle);

  if (movediag_xby != 0 || movediag_yby != 0)
  {
    icon_handle *table = malloc((selections + 1) * sizeof(icon_handle));
    if (!table)
    {
      WinEd_MsgTrans_Report(0,"NoStore",FALSE);
      return TRUE;
    }
    Wimp_WhichIcon(movediag_winentry->handle, table, icon_SELECTED, icon_SELECTED);

    Hourglass_On();

    for (int icon = 0; icon < selections; icon++)
    {
      wimp_rect newrect = {
        .min.x = movediag_winentry->window->icon[table[icon]].workarearect.min.x + movediag_xby,
        .min.y = movediag_winentry->window->icon[table[icon]].workarearect.min.y + movediag_yby,
        .max.x = movediag_winentry->window->icon[table[icon]].workarearect.max.x + movediag_xby,
        .max.y = movediag_winentry->window->icon[table[icon]].workarearect.max.y + movediag_yby
      };

      icnedit_moveicon(movediag_winentry, table[icon], &newrect);
    }

    free(table);

    Hourglass_Off();
    browser_settitle(movediag_winentry->browser, 0, TRUE);
  }

  if (event->data.mouse.button.data.select)
    movediag_close();

  return TRUE;
}

void movediag_close()
{
  return_caret(movediag_window, movediag_winentry->handle);
  Wimp_CloseWindow(movediag_window);
}
