/* visarea.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Hourglass.h"

#include "diagutils.h"
#include "round.h"
#include "viewtools.h"
#include "visarea.h"
#include "title.h"

typedef enum {
  visarea_LEFT = 2,
  visarea_DECLEFT,
  visarea_INCLEFT,
  visarea_TOP,
  visarea_DECTOP,
  visarea_INCTOP,
  visarea_WIDTH = 10,
  visarea_DECWIDTH,
  visarea_INCWIDTH,
  visarea_HEIGHT,
  visarea_DECHEIGHT,
  visarea_INCHEIGHT,
  visarea_OK = 16,
  visarea_CANCEL,
  visarea_HSCROLL = 20,
  visarea_DECHSCROLL,
  visarea_INCHSCROLL,
  visarea_VSCROLL,
  visarea_DECVSCROLL,
  visarea_INCVSCROLL
} visarea_icons;

/* Write icons */
void visarea_writeicons(wimp_rect *screenrect, wimp_point *scroll);

/* Button handlers */
BOOL visarea_clickok(event_pollblock *event,void *reference);
BOOL visarea_clickcancel(event_pollblock *event,void *reference);
BOOL visarea_keypress(event_pollblock *event,void *reference);


window_handle visarea_window;
browser_winentry *visarea_winentry;


void visarea_init()
{
  window_block *templat;

  templat = templates_load("VisArea",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&visarea_window));
  visarea_winentry = 0;
  Event_Claim(event_OPEN,visarea_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,visarea_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,visarea_window,visarea_OK,visarea_clickok,0);
  Event_Claim(event_CLICK,visarea_window,visarea_CANCEL, visarea_clickcancel,0);
  Event_Claim(event_KEY,visarea_window,event_ANY,visarea_keypress,0);
  diagutils_bumpers(visarea_window,visarea_LEFT,0,99999996,-1);
  diagutils_bumpers(visarea_window,visarea_TOP,0,99999996,-1);
  diagutils_bumpers(visarea_window,visarea_WIDTH,4,99999996,-1);
  diagutils_bumpers(visarea_window,visarea_HEIGHT,4,99999996,-1);
  diagutils_bumpers(visarea_window,visarea_HSCROLL,0,99999996,-1);
  diagutils_bumpers(visarea_window,visarea_VSCROLL,-99999996,0,-1);
  help_claim_window(visarea_window,"VSA");
}

void visarea_writeicons(wimp_rect *screenrect, wimp_point *scroll)
{
  Icon_SetInteger(visarea_window,visarea_LEFT, screenrect->min.x);
  Icon_SetInteger(visarea_window,visarea_TOP, screenrect->max.y);
  Icon_SetInteger(visarea_window,visarea_WIDTH, screenrect->max.x - screenrect->min.x);
  Icon_SetInteger(visarea_window,visarea_HEIGHT, screenrect->max.y - screenrect->min.y);
  Icon_SetInteger(visarea_window,visarea_HSCROLL, scroll->x);
  Icon_SetInteger(visarea_window,visarea_VSCROLL, scroll->y);
}

void visarea_open(browser_winentry *winentry)
{
  char buffer[40];

  visarea_winentry = winentry;

  visarea_writeicons(&winentry->window->window.screenrect, &winentry->window->window.scroll);
  MsgTrans_LookupPS(messages, "VisTitle", buffer, 40, winentry->identifier, 0, 0, 0);
  Window_SetTitle(visarea_window,buffer);
  Window_Show(visarea_window, open_UNDERPOINTER);
  Icon_SetCaret(visarea_window, visarea_LEFT);
}

BOOL visarea_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock ev;

    ev.data.mouse.button.value = button_SELECT;
    return visarea_clickok(&ev,reference);
  }
  else if (event->data.key.code == 27)
    visarea_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

BOOL visarea_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    visarea_close();
  return TRUE;
}

BOOL visarea_clickok(event_pollblock *event, void *reference)
{
  window_state wstate;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  Wimp_GetWindowState(visarea_winentry->handle, &wstate);
  wstate.openblock.screenrect.min.x = Icon_GetInteger(visarea_window,visarea_LEFT);
  wstate.openblock.screenrect.max.y = Icon_GetInteger(visarea_window,visarea_TOP);
  wstate.openblock.screenrect.max.x = wstate.openblock.screenrect.min.x + Icon_GetInteger(visarea_window,visarea_WIDTH);
  wstate.openblock.screenrect.min.y = wstate.openblock.screenrect.max.y - Icon_GetInteger(visarea_window,visarea_HEIGHT);
  wstate.openblock.scroll.x = Icon_GetInteger(visarea_window, visarea_HSCROLL);
  wstate.openblock.scroll.y = Icon_GetInteger(visarea_window, visarea_VSCROLL);

  if (choices->viewtools)
    viewtools_open(&wstate.openblock,visarea_winentry->pane);
  else
    Wimp_OpenWindow(&wstate.openblock);
  visarea_winentry->window->window.screenrect = wstate.openblock.screenrect;
  visarea_winentry->window->window.scroll = wstate.openblock.scroll;

  browser_settitle(visarea_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    visarea_close();

  return TRUE;
}

void visarea_close()
{
  return_caret(visarea_window, visarea_winentry->handle);
  Wimp_CloseWindow(visarea_window);
}
