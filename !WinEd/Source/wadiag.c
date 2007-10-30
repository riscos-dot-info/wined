/* wadiag.c */

#include "common.h"

#include "diagutils.h"
#include "title.h"
#include "viewex.h"
#include "wadiag.h"

typedef enum {
  wadiag_UPDATE,
  wadiag_CANCEL,
  wadiag_YMAX = 6,
  wadiag_DECYMAX,
  wadiag_INCYMAX,
  wadiag_XMIN,
  wadiag_DECXMIN,
  wadiag_INCXMIN,
  wadiag_XMAX,
  wadiag_DECXMAX,
  wadiag_INCXMAX,
  wadiag_YMIN,
  wadiag_DECYMIN,
  wadiag_INCYMIN,
  wadiag_NORMALISE,
  wadiag_MINIMISE,
  wadiag_SIZEX,
  wadiag_DECSIZEX,
  wadiag_INCSIZEX,
  wadiag_SIZEY,
  wadiag_DECSIZEY,
  wadiag_INCSIZEY,
  wadiag_MAXIMISE
} wadiag_icons;

/* Set up writable icons in wadiag */
void wadiag_setworkarea(wimp_rect *workarea);
void wadiag_setminsize(window_minsize *minsize);

/* Button handlers */
BOOL wadiag_clickupdate(event_pollblock *event,void *reference);
BOOL wadiag_clickcancel(event_pollblock *event,void *reference);
BOOL wadiag_clicknormalise(event_pollblock *event,void *reference);
BOOL wadiag_clickminimise(event_pollblock *event,void *reference);
BOOL wadiag_clickmaximise(event_pollblock *event,void *reference);
/* Handle key presses */
BOOL wadiag_keypress(event_pollblock *event,void *reference);

window_handle wadiag_window;
browser_winentry *wadiag_winentry;

void wadiag_init()
{
  window_block *templat;

  templat = templates_load("WorkArea",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&wadiag_window));
  wadiag_winentry = 0;
  Event_Claim(event_OPEN,wadiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,wadiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,wadiag_window,wadiag_UPDATE,wadiag_clickupdate,0);
  Event_Claim(event_CLICK,wadiag_window,wadiag_CANCEL,wadiag_clickcancel,0);
  Event_Claim(event_CLICK,wadiag_window,wadiag_MINIMISE,
  	      wadiag_clickminimise,0);
  Event_Claim(event_CLICK,wadiag_window,wadiag_NORMALISE,
  	      wadiag_clicknormalise,0);
  Event_Claim(event_CLICK,wadiag_window,wadiag_MAXIMISE,
  	      wadiag_clickmaximise,0);
  Event_Claim(event_KEY,wadiag_window,event_ANY,wadiag_keypress,0);
  /* Handlers for 'bumper' icons */
  diagutils_bumpers(wadiag_window,wadiag_XMIN,0,99999996,-1);
  diagutils_bumpers(wadiag_window,wadiag_XMAX,0,99999996,-1);
  diagutils_bumpers(wadiag_window,wadiag_YMIN,-99999996,0,-1);
  diagutils_bumpers(wadiag_window,wadiag_YMAX,-99999996,0,-1);
  diagutils_bumpers(wadiag_window,wadiag_SIZEX,0,16380,-1);
  diagutils_bumpers(wadiag_window,wadiag_SIZEY,0,16380,-1);
  help_claim_window(wadiag_window,"WAD");
}

void wadiag_setworkarea(wimp_rect *workarea)
{
  Icon_SetInteger(wadiag_window,wadiag_XMIN,workarea->min.x);
  Icon_SetInteger(wadiag_window,wadiag_YMIN,workarea->min.y);
  Icon_SetInteger(wadiag_window,wadiag_XMAX,workarea->max.x);
  Icon_SetInteger(wadiag_window,wadiag_YMAX,workarea->max.y);
}

void wadiag_setminsize(window_minsize *minsize)
{
  Icon_SetInteger(wadiag_window,wadiag_SIZEX,minsize->x);
  Icon_SetInteger(wadiag_window,wadiag_SIZEY,minsize->y);
}

void wadiag_open(browser_winentry *winentry)
{
  char buffer[28];

  wadiag_winentry = winentry;
  wadiag_setworkarea(&winentry->window->window.workarearect);
  wadiag_setminsize(&winentry->window->window.minsize);
  Str_MakeASCIIZ(winentry->identifier,wimp_MAXNAME);
  MsgTrans_LookupPS(messages,"WADgT",buffer,28,winentry->identifier,0,0,0);
  Window_SetTitle(wadiag_window,buffer);
  Window_Show(wadiag_window,open_UNDERPOINTER);
  Icon_SetCaret(wadiag_window,wadiag_YMAX);
}

BOOL wadiag_clickupdate(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
  {
    wadiag_winentry->window->window.workarearect.min.x =
      Icon_GetInteger(wadiag_window,wadiag_XMIN);
    wadiag_winentry->window->window.workarearect.min.y =
      Icon_GetInteger(wadiag_window,wadiag_YMIN);
    wadiag_winentry->window->window.workarearect.max.x =
      Icon_GetInteger(wadiag_window,wadiag_XMAX);
    wadiag_winentry->window->window.workarearect.max.y =
      Icon_GetInteger(wadiag_window,wadiag_YMAX);
    wadiag_winentry->window->window.minsize.x =
      Icon_GetInteger(wadiag_window,wadiag_SIZEX);
    wadiag_winentry->window->window.minsize.y =
      Icon_GetInteger(wadiag_window,wadiag_SIZEY);
    viewer_reopen(wadiag_winentry,FALSE);
    browser_settitle(wadiag_winentry->browser,0,TRUE);
  }

  if (event->data.mouse.button.data.select)
    wadiag_close();

  return TRUE;
}

BOOL wadiag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    wadiag_close();
  return TRUE;
}

BOOL wadiag_clicknormalise(event_pollblock *event,void *reference)
{
  wimp_rect newwa;

  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
  {
    newwa.min.x = wadiag_winentry->window->window.scroll.x +
    		  wadiag_winentry->window->window.workarearect.min.x;
    newwa.max.y = wadiag_winentry->window->window.scroll.y +
    		  wadiag_winentry->window->window.workarearect.max.y;
    newwa.max.x = newwa.min.x +
    		  wadiag_winentry->window->window.screenrect.max.x -
    	      	  wadiag_winentry->window->window.screenrect.min.x;
    newwa.min.y = newwa.max.y -
    		  wadiag_winentry->window->window.screenrect.max.y +
    	      	  wadiag_winentry->window->window.screenrect.min.y;
    newwa.min.x = 0;
    newwa.max.y = 0;
    wadiag_setworkarea(&newwa);
  }

  return TRUE;
}

BOOL wadiag_clickminimise(event_pollblock *event,void *reference)
{
  wimp_rect newwa;

  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
  {
    newwa.min.x = wadiag_winentry->window->window.scroll.x +
    		  wadiag_winentry->window->window.workarearect.min.x;
    newwa.max.y = wadiag_winentry->window->window.scroll.y +
    		  wadiag_winentry->window->window.workarearect.max.y;
    newwa.max.x = newwa.min.x +
    		  wadiag_winentry->window->window.screenrect.max.x -
    	      	  wadiag_winentry->window->window.screenrect.min.x;
    newwa.min.y = newwa.max.y -
    		  wadiag_winentry->window->window.screenrect.max.y +
    	      	  wadiag_winentry->window->window.screenrect.min.y;
    wadiag_setworkarea(&newwa);
  }

  return TRUE;
}

BOOL wadiag_clickmaximise(event_pollblock *event,void *reference)
{
  window_minsize minsize;

  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
  {
    minsize.x = wadiag_winentry->window->window.screenrect.max.x -
    	      	wadiag_winentry->window->window.screenrect.min.x;
    minsize.y = wadiag_winentry->window->window.screenrect.max.y -
    	      	wadiag_winentry->window->window.screenrect.min.y;
    wadiag_setminsize(&minsize);
  }

  return TRUE;
}

BOOL wadiag_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock event;

    event.type = event_CLICK;
    event.data.mouse.window = wadiag_window;
    event.data.mouse.button.value = button_SELECT;

    return wadiag_clickupdate(&event,reference);
  }
  else if (event->data.key.code == 27)
    wadiag_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

void wadiag_close()
{
  return_caret(wadiag_window, wadiag_winentry->handle);
  Wimp_CloseWindow(wadiag_window);
}
