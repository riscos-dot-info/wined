/* alndiag.c */

#include "common.h"

#include <limits.h>

#include "DeskLib:Validation.h"

#include "alndiag.h"
#include "icnedit.h"
#include "title.h"

typedef enum {
  alndiag_ALIGN,
  alndiag_CANCEL,
  alndiag_LEFT = 4,
  alndiag_HCENTRE,
  alndiag_RIGHT,
  alndiag_TOP,
  alndiag_VCENTRE,
  alndiag_BOTTOM,
  alndiag_GROUPBOX,
  alndiag_MOVE,
  alndiag_RESIZE,
  alndiag_BOTTOMLEFT,
  alndiag_TOPRIGHT,
  alndiag_INWINDOW
} alndiag_icons;

#define alndiag_LABELHOFFSET 20
#define alndiag_LABELVOFFSET 24

/* Button handlers */
BOOL alndiag_clickalign(event_pollblock *event,void *reference);
BOOL alndiag_clickcancel(event_pollblock *event,void *reference);
/* Makes sure selections are sensible, params are ignored */
BOOL alndiag_clickradio(event_pollblock *event,void *reference);

window_handle alndiag_window;
browser_winentry *alndiag_winentry;

/* Whether this is a pair of icons suitable as box & label */
static BOOL alndiag_allowgroupbox;
/* Whether this is a single icon which can only be centred in window */
static BOOL alndiag_sole;

BOOL grouppair(browser_winentry *winentry,icon_handle icon1,icon_handle icon2);
/* Is the icon suitable as a border? */
BOOL suitable_border(browser_winentry *winentry,icon_handle icon);

/* malloc and fill in a table of selected icons (terminated by -1) */
BOOL alndiag_fillselectedtable(icon_handle **table);

BOOL suitable_border(browser_winentry *winentry,icon_handle icon)
{
  icon_block *block = &winentry->window->icon[icon];

  /* Is  icon bordered with no contents? */
  if (!block->flags.data.border)
    return FALSE;
  if (!block->flags.data.indirected)
  {
    if ((block->flags.data.text || block->flags.data.sprite) &&
        block->data.text[0] > 31)
    return FALSE;
  }
  else /* Indirected */
  {
    if (block->flags.data.text)
    {
      if ((block->data.indirecttext.buffer +
           (int) winentry->window)[0] > 31)
        return FALSE;
      if (block->flags.data.sprite)
      {
        if (block->flags.data.text)
        {
          if (Validation_ScanString(block->data.indirecttext.validstring +
            			    (int) winentry->window,'S'))
            return FALSE;
        }
        else
          return FALSE;
      }
    }
  }
  return TRUE;
}

BOOL grouppair(browser_winentry *winentry,icon_handle icon1,icon_handle icon2)
{
  icon_block *block = &winentry->window->icon[icon2];
  /* Is first icon suitable for border? */
  if (!suitable_border(winentry,icon1))
    return FALSE;
  /* Is second icon text only, no border,
     or "trick" indirected text and sprite? */
  if (block->flags.data.border || !block->flags.data.text ||
      (!block->flags.data.indirected && block->flags.data.sprite))
    return FALSE;

  return TRUE;
}

void alndiag_init()
{
  window_block *templat;

  templat = templates_load("Align",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&alndiag_window));
  alndiag_winentry = 0;
  Event_Claim(event_OPEN,alndiag_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,alndiag_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,alndiag_window,alndiag_ALIGN,alndiag_clickalign,0);
  Event_Claim(event_CLICK,alndiag_window,alndiag_CANCEL,alndiag_clickcancel,0);
  Event_Claim(event_CLICK,alndiag_window,event_ANY,alndiag_clickradio,0);
  help_claim_window(alndiag_window,"ALN");
}

void alndiag_open(browser_winentry *winentry)
{
  icon_handle selected[3];

  alndiag_clickradio(0,0);
  alndiag_winentry = winentry;

  alndiag_allowgroupbox = FALSE;
  switch (count_selections(winentry->handle))
  {
    case 1:
      Icon_SetRadios(alndiag_window, alndiag_BOTTOMLEFT, alndiag_INWINDOW,
      	alndiag_INWINDOW);
      Icon_Shade(alndiag_window, alndiag_BOTTOMLEFT);
      Icon_Shade(alndiag_window, alndiag_TOPRIGHT);
      alndiag_allowgroupbox = FALSE;
      alndiag_sole = TRUE;
      break;
    case 2:
      Wimp_WhichIcon(winentry->handle,selected,icon_SELECTED,icon_SELECTED);
      if (grouppair(winentry,selected[0],selected[1]))
        alndiag_allowgroupbox = TRUE;
      else
        alndiag_allowgroupbox = FALSE;
    default:
      Icon_Unshade(alndiag_window, alndiag_BOTTOMLEFT);
      Icon_Unshade(alndiag_window, alndiag_TOPRIGHT);
      //Icon_Unshade(alndiag_window, alndiag_MOVE);
      //Icon_Unshade(alndiag_window, alndiag_RESIZE);
      Icon_Unshade(alndiag_window, alndiag_LEFT);
      Icon_Unshade(alndiag_window, alndiag_RIGHT);
      Icon_Unshade(alndiag_window, alndiag_TOP);
      Icon_Unshade(alndiag_window, alndiag_BOTTOM);
      alndiag_sole = FALSE;
      break;
  }
  alndiag_clickradio(0,0);
  Window_Show(alndiag_window,open_UNDERPOINTER);
}

BOOL alndiag_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.select ||
      event->data.mouse.button.data.adjust)
    alndiag_close();
  return TRUE;
}

BOOL alndiag_clickradio(event_pollblock *event,void *reference)
{
  icon_handle selection;

  Icon_Unshade(alndiag_window,alndiag_INWINDOW);
  if (Icon_WhichRadioInEsg(alndiag_window,3) == -1)
  {
    if (alndiag_sole)
      Icon_Select(alndiag_window,alndiag_INWINDOW);
    else
      Icon_Select(alndiag_window,alndiag_BOTTOMLEFT);
  }
  if (Icon_GetSelect(alndiag_window, alndiag_INWINDOW))
  {
    Icon_Select(alndiag_window, alndiag_MOVE);
    Icon_Deselect(alndiag_window, alndiag_RESIZE);
    Icon_Shade(alndiag_window, alndiag_MOVE);
    Icon_Shade(alndiag_window, alndiag_RESIZE);
    if (!Icon_GetSelect(alndiag_window, alndiag_HCENTRE) &&
    	!Icon_GetSelect(alndiag_window, alndiag_VCENTRE))
      Icon_SetRadios(alndiag_window, alndiag_LEFT, alndiag_GROUPBOX,
      	alndiag_HCENTRE);
    Icon_Shade(alndiag_window, alndiag_LEFT);
    Icon_Shade(alndiag_window, alndiag_RIGHT);
    Icon_Shade(alndiag_window, alndiag_TOP);
    Icon_Shade(alndiag_window, alndiag_BOTTOM);
    Icon_Deselect(alndiag_window, alndiag_GROUPBOX);
    Icon_Shade(alndiag_window, alndiag_GROUPBOX);
  }
  else
  {
    Icon_Unshade(alndiag_window, alndiag_MOVE);
    Icon_Unshade(alndiag_window, alndiag_RESIZE);
    Icon_Unshade(alndiag_window, alndiag_LEFT);
    Icon_Unshade(alndiag_window, alndiag_RIGHT);
    Icon_Unshade(alndiag_window, alndiag_TOP);
    Icon_Unshade(alndiag_window, alndiag_BOTTOM);
    Icon_Unshade(alndiag_window, alndiag_GROUPBOX);
    Icon_Unshade(alndiag_window, alndiag_BOTTOMLEFT);
    Icon_Unshade(alndiag_window, alndiag_TOPRIGHT);
    Icon_Unshade(alndiag_window, alndiag_RESIZE);
  }

  if (alndiag_allowgroupbox)
    Icon_Unshade(alndiag_window,alndiag_GROUPBOX);
  else
  {
    Icon_Deselect(alndiag_window,alndiag_GROUPBOX);
    Icon_Shade(alndiag_window,alndiag_GROUPBOX);
  }
  selection = Icon_WhichRadioInEsg(alndiag_window,1);
  if (selection == -1)
    Icon_Select(alndiag_window,alndiag_HCENTRE);
  switch (selection)
  {
    case alndiag_GROUPBOX:
      if (Icon_WhichRadioInEsg(alndiag_window,3) == alndiag_TOPRIGHT)
      {
        Icon_Unshade(alndiag_window,alndiag_RESIZE);
        break;
      }
      Icon_Shade(alndiag_window,alndiag_INWINDOW);
    case alndiag_HCENTRE:
    case alndiag_VCENTRE:
      Icon_Shade(alndiag_window,alndiag_RESIZE);
      Icon_Deselect(alndiag_window,alndiag_RESIZE);
      break;
    default:
      if (!alndiag_sole)
        Icon_Unshade(alndiag_window,alndiag_RESIZE);
      break;
  }

  if (Icon_WhichRadioInEsg(alndiag_window,2) == -1)
    Icon_Select(alndiag_window,alndiag_MOVE);

  return TRUE;
}

BOOL alndiag_fillselectedtable(icon_handle **table)
{
  *table = malloc(sizeof(icon_handle) *
           	  (count_selections(alndiag_winentry->handle) + 1));
  if (!*table)
  {
    WinEd_MsgTrans_Report(0,"NoStore",FALSE);
    return FALSE;
  }

  Wimp_WhichIcon(alndiag_winentry->handle,*table,icon_SELECTED,icon_SELECTED);

  return TRUE;
}

BOOL alndiag_clickalign(event_pollblock *event,void *reference)
{
  int alignto;
  int i,j;
  icon_handle *selected;
  wimp_rect newbounds;
  BOOL resize;

  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  if (!alndiag_fillselectedtable(&selected))
    return TRUE;

  if (Icon_GetSelect(alndiag_window, alndiag_INWINDOW))
  {
    int wcentre = 0;	/* Initialiser suppresses warning */

    switch (Icon_WhichRadioInEsg(alndiag_window,1))
    {
      case alndiag_HCENTRE:
        wcentre = (alndiag_winentry->window->window.screenrect.max.x +
        	alndiag_winentry->window->window.screenrect.min.x) / 2 -
        	alndiag_winentry->window->window.screenrect.min.x +
        	alndiag_winentry->window->window.scroll.x;
        break;
      case alndiag_VCENTRE:
        wcentre = (alndiag_winentry->window->window.screenrect.max.y +
        	alndiag_winentry->window->window.screenrect.min.y) / 2 -
        	alndiag_winentry->window->window.screenrect.max.y +
        	alndiag_winentry->window->window.scroll.y;
        break;
    }

    for (i = 0; selected[i] != -1; ++i)
    {
      int icentre;

      newbounds = alndiag_winentry->window->icon[selected[i]].workarearect;
      switch (Icon_WhichRadioInEsg(alndiag_window,1))
      {
        case alndiag_HCENTRE:
          icentre =
          	(alndiag_winentry->window->icon[selected[i]].workarearect.max.x+
          	alndiag_winentry->window->icon[selected[i]].workarearect.min.x)
          	/ 2;
          newbounds.min.x -= icentre - wcentre;
          newbounds.max.x -= icentre - wcentre;
          break;
        case alndiag_VCENTRE:
          icentre =
          	(alndiag_winentry->window->icon[selected[i]].workarearect.max.y+
          	alndiag_winentry->window->icon[selected[i]].workarearect.min.y)
          	/ 2;
          newbounds.min.y -= icentre - wcentre;
          newbounds.max.y -= icentre - wcentre;
          break;
      }
      icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
    }
  }
  else
  {
    resize = Icon_GetSelect(alndiag_window,alndiag_RESIZE);

    switch (Icon_WhichRadioInEsg(alndiag_window,1))
    {
      case alndiag_LEFT:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find leftmost left edge */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.min.x
              	< alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
            break;
          case alndiag_TOPRIGHT: /* Find rightmost left edge */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.min.x
              	> alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          newbounds = alndiag_winentry->window->icon[selected[i]].workarearect;
          newbounds.min.x = alignto;
          if (!resize)
          {
            newbounds.max.x = newbounds.min.x +
              alndiag_winentry->window->icon[selected[i]].workarearect.max.x -
              alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
          }
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_HCENTRE:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find leftmost h middle */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
            {
              j = alndiag_winentry->window->icon[selected[i]].workarearect.max.x
               + alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
              if (j < alignto)
                alignto = j;
            }
            alignto /= 2;
            break;
          case alndiag_TOPRIGHT: /* Find rightmost h middle */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
            {
              j = alndiag_winentry->window->icon[selected[i]].workarearect.max.x
               + alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
              if (j > alignto)
                alignto = j;
            }
            alignto /= 2;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          int size_x =
            alndiag_winentry->window->icon[selected[i]].workarearect.max.x -
            alndiag_winentry->window->icon[selected[i]].workarearect.min.x;

          newbounds.min.y =
            alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
          newbounds.max.y =
            alndiag_winentry->window->icon[selected[i]].workarearect.max.y;
          newbounds.min.x = alignto - size_x / 2;
          newbounds.max.x = alignto + size_x / 2;
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_RIGHT:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find leftmost right edge */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.max.x
              	< alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.max.x;
            break;
          case alndiag_TOPRIGHT: /* Find rightmost right edge */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.max.x
              	> alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.max.x;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          newbounds = alndiag_winentry->window->icon[selected[i]].workarearect;
          newbounds.max.x = alignto;
          if (!resize)
          {
            newbounds.min.x = newbounds.max.x -
              alndiag_winentry->window->icon[selected[i]].workarearect.max.x +
              alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
          }
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_TOP:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find bottommost top edge */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.max.y
              	< alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.max.y;
            break;
          case alndiag_TOPRIGHT: /* Find topmost top edge */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.max.y
              	> alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.max.y;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          newbounds = alndiag_winentry->window->icon[selected[i]].workarearect;
          newbounds.max.y = alignto;
          if (!resize)
          {
            newbounds.min.y = newbounds.max.y -
              alndiag_winentry->window->icon[selected[i]].workarearect.max.y +
              alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
          }
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_VCENTRE:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find bottommost v middle */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
            {
              j = alndiag_winentry->window->icon[selected[i]].workarearect.max.y
               + alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
              if (j < alignto)
                alignto = j;
            }
            alignto /= 2;
            break;
          case alndiag_TOPRIGHT: /* Find topmost v middle */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
            {
              j = alndiag_winentry->window->icon[selected[i]].workarearect.max.y
               + alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
              if (j > alignto)
                alignto = j;
            }
            alignto /= 2;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          int size_y =
            alndiag_winentry->window->icon[selected[i]].workarearect.max.y -
            alndiag_winentry->window->icon[selected[i]].workarearect.min.y;

          newbounds.min.x =
            alndiag_winentry->window->icon[selected[i]].workarearect.min.x;
          newbounds.max.x =
            alndiag_winentry->window->icon[selected[i]].workarearect.max.x;
          newbounds.min.y = alignto - size_y / 2;
          newbounds.max.y = alignto + size_y / 2;
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_BOTTOM:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Find bottommost bottom edge */
            alignto = INT_MAX;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.min.y
              < alignto)
                alignto =
                 alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
            break;
          case alndiag_TOPRIGHT: /* Find topmost bottom edge */
            alignto = INT_MIN;
            for (i = 0; selected[i] != -1; i++)
              if (alndiag_winentry->window->icon[selected[i]].workarearect.min.y
              	> alignto)
                alignto =
                  alndiag_winentry->window->icon[selected[i]].
                  	workarearect.min.y;
            break;
          default:
            alignto = 0;
        }
        for (i = 0; selected[i] != -1; i++)
        {
          newbounds = alndiag_winentry->window->icon[selected[i]].workarearect;
          newbounds.min.y = alignto;
          if (!resize)
          {
            newbounds.max.y = newbounds.min.y +
              alndiag_winentry->window->icon[selected[i]].workarearect.max.y -
              alndiag_winentry->window->icon[selected[i]].workarearect.min.y;
          }
          icnedit_moveicon(alndiag_winentry,selected[i],&newbounds);
        }
        break;
      case alndiag_GROUPBOX:
        switch (Icon_WhichRadioInEsg(alndiag_window,3))
        {
          case alndiag_BOTTOMLEFT: /* Move label */
            newbounds.min.x =
              alndiag_winentry->window->icon[selected[0]].workarearect.min.x +
              alndiag_LABELHOFFSET;
            newbounds.max.x = newbounds.min.x +
              alndiag_winentry->window->icon[selected[1]].workarearect.max.x -
              alndiag_winentry->window->icon[selected[1]].workarearect.min.x;
            newbounds.min.y =
              alndiag_winentry->window->icon[selected[0]].workarearect.max.y -
              alndiag_LABELVOFFSET;
            newbounds.max.y = newbounds.min.y +
              alndiag_winentry->window->icon[selected[1]].workarearect.max.y -
              alndiag_winentry->window->icon[selected[1]].workarearect.min.y;
            icnedit_moveicon(alndiag_winentry,selected[1],&newbounds);
            break;
          case alndiag_TOPRIGHT: /* Move box */
            newbounds.min.x =
              alndiag_winentry->window->icon[selected[1]].workarearect.min.x -
              alndiag_LABELHOFFSET;
            newbounds.max.y =
              alndiag_winentry->window->icon[selected[1]].workarearect.min.y +
              alndiag_LABELVOFFSET;
            if (resize)
            {
              newbounds.max.x =
                alndiag_winentry->window->icon[selected[0]].workarearect.max.x;
              newbounds.min.y =
                alndiag_winentry->window->icon[selected[0]].workarearect.min.y;
            }
            else
            {
              newbounds.max.x = newbounds.min.x +
                alndiag_winentry->window->icon[selected[0]].workarearect.max.x -
                alndiag_winentry->window->icon[selected[0]].workarearect.min.x;
              newbounds.min.y = newbounds.max.y -
                alndiag_winentry->window->icon[selected[0]].workarearect.max.y +
                alndiag_winentry->window->icon[selected[0]].workarearect.min.y;
            }
            icnedit_moveicon(alndiag_winentry,selected[0],&newbounds);
            break;
        }
        break;
    }
  }
  browser_settitle(alndiag_winentry->browser,NULL,TRUE);
  if (event->data.mouse.button.data.select)
    alndiag_close();
  free(selected);

  return TRUE;
}

void alndiag_close()
{
  return_caret(alndiag_window, alndiag_winentry->handle);
  Wimp_CloseWindow(alndiag_window);
}
