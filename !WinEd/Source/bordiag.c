/* bordiag.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Hourglass.h"

#include "diagutils.h"
#include "icnedit.h"
#include "bordiag.h"
#include "title.h"

typedef enum {
  bordiag_BORDER,
  bordiag_CANCEL,
  bordiag_GAP, bordiag_DECGAP, bordiag_INCGAP,
  bordiag_VISAREA,
  bordiag_WITHICON,
  bordiag_ICON, bordiag_DECICON, bordiag_INCICON,
  bordiag_LABEL, bordiag_ALLOW, bordiag_DECALLOW, bordiag_INCALLOW,
  bordiag_LEFT, bordiag_BOTTOM, bordiag_RIGHT, bordiag_TOP
} bordiag_icons;

/* This function is defined in alndiag.c */
extern BOOL suitable_border(browser_winentry *winentry,icon_handle icon);

/* Button handlers */
BOOL bordiag_clickborder(event_pollblock *event,void *reference);
BOOL bordiag_clickcancel(event_pollblock *event,void *reference);
BOOL bordiag_clickradio(event_pollblock *event,void *reference);
BOOL bordiag_keypress(event_pollblock *event,void *reference);

window_handle bordiag_window;
browser_winentry *bordiag_winentry;


void bordiag_init()
{
  window_block *templat;

  templat = templates_load("Frame",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&bordiag_window));
  bordiag_winentry = 0;
  Event_Claim(event_OPEN,bordiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,bordiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,bordiag_window,bordiag_BORDER,bordiag_clickborder,0);
  Event_Claim(event_CLICK,bordiag_window,bordiag_CANCEL,bordiag_clickcancel,0);
  Event_Claim(event_CLICK,bordiag_window,bordiag_VISAREA,bordiag_clickradio,0);
  Event_Claim(event_CLICK,bordiag_window,bordiag_WITHICON,bordiag_clickradio,0);
  Event_Claim(event_CLICK,bordiag_window,bordiag_TOP,bordiag_clickradio,0);
  Event_Claim(event_KEY,bordiag_window,event_ANY,bordiag_keypress,0);
  diagutils_bumpers(bordiag_window,bordiag_GAP,0,256,-1);
  diagutils_bumpers(bordiag_window,bordiag_ICON,0,999,1);
  diagutils_bumpers(bordiag_window,bordiag_ALLOW,0,996,-1);
  help_claim_window(bordiag_window,"BOR");
  Icon_Select(bordiag_window,bordiag_LEFT);
  Icon_Select(bordiag_window,bordiag_BOTTOM);
  Icon_Select(bordiag_window,bordiag_RIGHT);
  Icon_Select(bordiag_window,bordiag_TOP);
}

void bordiag_open(browser_winentry *winentry)
{
  bordiag_winentry = winentry;
  bordiag_clickradio(0,0);
  Window_Show(bordiag_window,open_UNDERPOINTER);
  Icon_SetCaret(bordiag_window,bordiag_GAP);
}

BOOL bordiag_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock ev;

    ev.data.mouse.button.value = button_SELECT;
    return bordiag_clickborder(&ev,reference);
  }
  else if (event->data.key.code == 27)
    bordiag_close();
  else
    Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

BOOL bordiag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    bordiag_close();
  return TRUE;
}

BOOL bordiag_clickborder(event_pollblock *event,void *reference)
{
  int gap;
  int first; /* ... icon in selection */
  int icon;
  wimp_rect rect;
  window_state wstate;
  BOOL left,right,bottom,top;
  BOOL ext;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  /* Find bounds of icons with gap */
  first = INT_MAX;
  rect.min.x = rect.min.y = INT_MAX;
  rect.max.x = rect.max.y = INT_MIN;
  for (icon = 0;icon < bordiag_winentry->window->window.numicons;icon++)
  {
    if (Icon_GetSelect(bordiag_winentry->handle,icon))
    {
      wimp_rect *refrect = &bordiag_winentry->window->icon[icon].workarearect;

      if (icon < first)
        first = icon;
      if (refrect->min.x < rect.min.x)
        rect.min.x = refrect->min.x;
      if (refrect->min.y < rect.min.y)
        rect.min.y = refrect->min.y;
      if (refrect->max.x > rect.max.x)
        rect.max.x = refrect->max.x;
      if (refrect->max.y > rect.max.y)
        rect.max.y = refrect->max.y;
    }
  }
  gap = Icon_GetInteger(bordiag_window,bordiag_GAP);
  rect.min.x -= gap;
  rect.min.y -= gap;
  rect.max.x += gap;
  rect.max.y += gap;

  left = Icon_GetSelect(bordiag_window,bordiag_LEFT);
  right = Icon_GetSelect(bordiag_window,bordiag_RIGHT);
  bottom = Icon_GetSelect(bordiag_window,bordiag_BOTTOM);
  top = Icon_GetSelect(bordiag_window,bordiag_TOP);
  if (Icon_GetSelect(bordiag_window,bordiag_LABEL))
    rect.max.y += Icon_GetInteger(bordiag_window,bordiag_ALLOW);
  switch (Icon_WhichRadioInEsg(bordiag_window,1))
  {
    case bordiag_VISAREA:
      ext = FALSE;
      if (left &&
      	bordiag_winentry->window->window.workarearect.min.x > rect.min.x)
      {
        ext = TRUE;
        bordiag_winentry->window->window.workarearect.min.x = rect.min.x;
      }
      if (right &&
      	bordiag_winentry->window->window.workarearect.max.x < rect.max.x)
      {
        ext = TRUE;
        bordiag_winentry->window->window.workarearect.max.x = rect.max.x;
      }
      if (bottom &&
      	bordiag_winentry->window->window.workarearect.min.y > rect.min.y)
      {
        ext = TRUE;
        bordiag_winentry->window->window.workarearect.min.y = rect.min.y;
      }
      if (right &&
      	bordiag_winentry->window->window.workarearect.max.y < rect.max.y)
      {
        ext = TRUE;
        bordiag_winentry->window->window.workarearect.max.y = rect.max.y;
      }
      if (ext)
        Wimp_SetExtent(bordiag_winentry->handle,
        	&bordiag_winentry->window->window.workarearect);
      Wimp_GetWindowState(bordiag_winentry->handle,&wstate);
      if (left)
        wstate.openblock.scroll.x = rect.min.x;
      if (top)
        wstate.openblock.scroll.y = rect.max.y;
      if (bottom)
        wstate.openblock.screenrect.min.y = wstate.openblock.screenrect.max.y -
       					    wstate.openblock.scroll.y +
      					    rect.min.y;
      if (right)
        wstate.openblock.screenrect.max.x = wstate.openblock.screenrect.min.x -
      					    wstate.openblock.scroll.x +
      					    rect.max.x;
      Wimp_OpenWindow(&wstate.openblock);
      bordiag_winentry->window->window.screenrect = wstate.openblock.screenrect;
      bordiag_winentry->window->window.scroll = wstate.openblock.scroll;
      break;
    case bordiag_WITHICON:
      icon = Icon_GetInteger(bordiag_window,bordiag_ICON);
      if ((unsigned int) icon > bordiag_winentry->window->window.numicons)
      {
        MsgTrans_Report(messages,"IconRange",FALSE);
        return TRUE;
      }
      if (first < icon)
      {
        os_error message;

        message.errnum = 0;
        MsgTrans_Lookup(messages,"BehindBorder",message.errmess,252);
        if (!ok_report(&message))
          return TRUE;
      }
      else if (!suitable_border(bordiag_winentry,icon))
      {
        os_error message;

        message.errnum = 0;
        MsgTrans_Lookup(messages,"BadBorder",message.errmess,252);
        if (!ok_report(&message))
          return TRUE;
      }
      else if (Icon_GetSelect(bordiag_winentry->handle,icon))
      {
        os_error message;

        message.errnum = 0;
        MsgTrans_Lookup(messages,"SelBorder",message.errmess,252);
        if (!ok_report(&message))
          return TRUE;
      }
      if (!left)
        rect.min.x = bordiag_winentry->window->icon[icon].workarearect.min.x;
      if (!right)
        rect.max.x = bordiag_winentry->window->icon[icon].workarearect.max.x;
      if (!bottom)
        rect.min.y = bordiag_winentry->window->icon[icon].workarearect.min.y;
      if (!top)
        rect.max.y = bordiag_winentry->window->icon[icon].workarearect.max.y;
      icnedit_moveicon(bordiag_winentry,icon,&rect);
  }

  browser_settitle(bordiag_winentry->browser,NULL,TRUE);
  if (event->data.mouse.button.data.select)
    bordiag_close();

  return TRUE;
}

BOOL bordiag_clickradio(event_pollblock *event,void *reference)
{
  icon_handle shadegroup[] = {bordiag_ICON,bordiag_DECICON,bordiag_INCICON,
  	      		      bordiag_LABEL,bordiag_ALLOW,
  	      		      bordiag_DECALLOW,bordiag_INCALLOW,-1};

  switch (Icon_WhichRadioInEsg(bordiag_window,1))
  {
    case -1:
      Icon_Select(bordiag_window,bordiag_VISAREA);
    case bordiag_VISAREA:
      Icon_Deselect(bordiag_window,bordiag_LABEL);
      Icon_ShadeGroup(bordiag_window,shadegroup,TRUE);
      break;
    case bordiag_WITHICON:
      Icon_ShadeGroup(bordiag_window,shadegroup,FALSE);
      break;
  }
  if (Icon_GetSelect(bordiag_window,bordiag_TOP) &&
      Icon_GetSelect(bordiag_window,bordiag_WITHICON))
  {
    Icon_Unshade(bordiag_window,bordiag_LABEL);
    Icon_Unshade(bordiag_window,bordiag_ALLOW);
    Icon_Unshade(bordiag_window,bordiag_DECALLOW);
    Icon_Unshade(bordiag_window,bordiag_INCALLOW);
  }
  else
  {
    Icon_Deselect(bordiag_window,bordiag_LABEL);
    Icon_Shade(bordiag_window,bordiag_LABEL);
    Icon_Shade(bordiag_window,bordiag_ALLOW);
    Icon_Shade(bordiag_window,bordiag_DECALLOW);
    Icon_Shade(bordiag_window,bordiag_INCALLOW);
  }

  return TRUE;
}

void bordiag_close()
{
  return_caret(bordiag_window, bordiag_winentry->handle);
  Wimp_CloseWindow(bordiag_window);
}
