/* rszdiag.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Hourglass.h"

#include "diagutils.h"
#include "icnedit.h"
#include "rszdiag.h"
#include "title.h"

typedef enum {
  rszdiag_MINWIDTH = 4,
  rszdiag_MINHEIGHT,
  rszdiag_MINBOTH,
  rszdiag_WIDEST,
  rszdiag_NARROWEST,
  rszdiag_TALLEST,
  rszdiag_SHORTEST,
  rszdiag_EXCBORDER
} rszdiag_icons;

/* Button handlers */
BOOL rszdiag_clickminimise(event_pollblock *event,void *reference);
BOOL rszdiag_clickmakesame(event_pollblock *event,void *reference);


window_handle rszdiag_window;
browser_winentry *rszdiag_winentry;


void rszdiag_init()
{
  window_block *templat;

  templat = templates_load("Size",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&rszdiag_window));
  rszdiag_winentry = 0;
  Event_Claim(event_OPEN,rszdiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,rszdiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_MINWIDTH,
  	      rszdiag_clickminimise,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_MINHEIGHT,
  	      rszdiag_clickminimise,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_MINBOTH,
  	      rszdiag_clickminimise,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_WIDEST,
  	      rszdiag_clickmakesame,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_NARROWEST,
  	      rszdiag_clickmakesame,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_TALLEST,
  	      rszdiag_clickmakesame,0);
  Event_Claim(event_CLICK,rszdiag_window,rszdiag_SHORTEST,
  	      rszdiag_clickmakesame,0);
  help_claim_window(rszdiag_window,"RSZ");
}

void rszdiag_open(browser_winentry *winentry)
{
  int selections = count_selections(winentry->handle);

  rszdiag_winentry = winentry;
  if (selections == 1)
  {
    Icon_Shade(rszdiag_window,rszdiag_WIDEST);
    Icon_Shade(rszdiag_window,rszdiag_NARROWEST);
    Icon_Shade(rszdiag_window,rszdiag_TALLEST);
    Icon_Shade(rszdiag_window,rszdiag_SHORTEST);
  }
  else
  {
    Icon_Unshade(rszdiag_window,rszdiag_WIDEST);
    Icon_Unshade(rszdiag_window,rszdiag_NARROWEST);
    Icon_Unshade(rszdiag_window,rszdiag_TALLEST);
    Icon_Unshade(rszdiag_window,rszdiag_SHORTEST);
  }
  Window_Show(rszdiag_window,open_UNDERPOINTER);
}

BOOL rszdiag_clickminimise(event_pollblock *event,void *reference)
{
  wimp_point minsize;
  int icon;
  wimp_rect newrect;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  for (icon = 0;icon < rszdiag_winentry->window->window.numicons;icon++)
  {
    if (Icon_GetSelect(rszdiag_winentry->handle,icon))
    {
      newrect = rszdiag_winentry->window->icon[icon].workarearect;
      icnedit_minsize(rszdiag_winentry,icon,&minsize);
      if (event->data.mouse.icon == rszdiag_MINWIDTH ||
      	  event->data.mouse.icon == rszdiag_MINBOTH)
      {
        newrect.max.x = newrect.min.x + minsize.x;
      }
      if (event->data.mouse.icon == rszdiag_MINHEIGHT ||
      	  event->data.mouse.icon == rszdiag_MINBOTH)
      {
        newrect.max.y = newrect.min.y + minsize.y;
      }
      icnedit_moveicon(rszdiag_winentry,icon,&newrect);
    }
  }

  browser_settitle(rszdiag_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    rszdiag_close();

  return TRUE;
}

BOOL rszdiag_clickmakesame(event_pollblock *event,void *reference)
{
  int size = 0;
  int icon;
  wimp_rect newrect;
  BOOL xborder = Icon_GetSelect(event->data.mouse.window, rszdiag_EXCBORDER);

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  /* Find target size */
  switch (event->data.mouse.icon)
  {
    case rszdiag_WIDEST:
    case rszdiag_TALLEST:
      size = INT_MIN;
      break;
    case rszdiag_NARROWEST:
    case rszdiag_SHORTEST:
      size = INT_MAX;
      break;
  }
  for (icon = 0;icon < rszdiag_winentry->window->window.numicons;icon++)
  {
    if (Icon_GetSelect(rszdiag_winentry->handle,icon))
    {
      wimp_point oldsize;

      oldsize.x = rszdiag_winentry->window->icon[icon].workarearect.max.x -
                  rszdiag_winentry->window->icon[icon].workarearect.min.x;
      oldsize.y = rszdiag_winentry->window->icon[icon].workarearect.max.y -
                  rszdiag_winentry->window->icon[icon].workarearect.min.y;
      if (xborder)
      {
        int w = icnedit_borderwidth(rszdiag_winentry, icon);

        oldsize.x -= w;
        oldsize.y -= w;
      }
      switch (event->data.mouse.icon)
      {
        case rszdiag_WIDEST:
          if (oldsize.x > size)
            size = oldsize.x;
          break;
        case rszdiag_NARROWEST:
          if (oldsize.x < size)
            size = oldsize.x;
          break;
        case rszdiag_TALLEST:
          if (oldsize.y > size)
            size = oldsize.y;
          break;
        case rszdiag_SHORTEST:
          if (oldsize.y < size)
            size = oldsize.y;
          break;
      }
    }
  }

  /* Resize icons */
  for (icon = 0;icon < rszdiag_winentry->window->window.numicons;icon++)
  {
    if (Icon_GetSelect(rszdiag_winentry->handle,icon))
    {
      int w;

      newrect = rszdiag_winentry->window->icon[icon].workarearect;
      if (xborder)
        w = icnedit_borderwidth(rszdiag_winentry, icon);
      else
        w = 0;
      switch (event->data.mouse.icon)
      {
        case rszdiag_WIDEST:
        case rszdiag_NARROWEST:
          newrect.max.x = newrect.min.x + size + w;
          break;
        case rszdiag_TALLEST:
        case rszdiag_SHORTEST:
          newrect.max.y = newrect.min.y + size + w;
          break;
      }
      icnedit_moveicon(rszdiag_winentry,icon,&newrect);
    }
  }

  browser_settitle(rszdiag_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    rszdiag_close();

  return TRUE;
}

void rszdiag_close()
{
  Wimp_CloseWindow(rszdiag_window);
}
