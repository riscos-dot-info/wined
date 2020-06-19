/* windiag.c */

#include "common.h"

#include "diagutils.h"
#include "title.h"
#include "usersprt.h"
#include "viewex.h"
#include "windiag.h"
/* Icon names exported by WinEd */

/* Icons for window windiag */
typedef enum {
  windiag_UPDATE = 0,
  windiag_CANCEL = 1,
  windiag_NOSCROLL = 11,
  windiag_BACK = 12,
  windiag_CLOSE = 13,
  windiag_TITLE = 14,
  windiag_TOGGLE = 15,
  windiag_VSCROLL = 16,
  windiag_ADJUST = 17,
  windiag_HSCROLL = 18,
  windiag_SCROLLDEBOUNCED = 19,
  windiag_SCROLLAUTOREPEAT = 20,
  windiag_MOVEABLE = 21,
  windiag_AUTOREDRAW = 22,
  windiag_PANE = 23,
  windiag_BACKDROP = 24,
  windiag_HOTKEYS = 25,
  windiag_NOBOUNDS = 26,
  windiag_CONFINE = 27,
  windiag_IGNORERIGHT = 28,
  windiag_IGNOREBOTTOM = 29,
  windiag_BUTTONTYPE = 30,
  windiag_COLTITLEFORE = 32,
  windiag_COLTITLEBACK = 36,
  windiag_COLWORKFORE = 40,
  windiag_COLWORKBACK = 44,
  windiag_COLSCROLLOUTER = 48,
  windiag_COLSCROLLINNER = 52,
  windiag_COLFOCUS = 56,
  windiag_COLDEFAULT = 60,
  windiag_COLGCOL = 61,
  windiag_FURNITURE = 70,
  windiag_SCROLLEXTENDED = 71,
  windiag_COL24BIT = 72,
  windiag_BORDER3DYES = 73,
  windiag_SHADEDINFO = 74,
  windiag_BORDER3DNO = 76
} windiag_icons;

/* Handlers for window */
BOOL windiag_OpenWindow(event_pollblock *event,void *reference);
BOOL windiag_CloseWindow(event_pollblock *event,void *reference);
/* Update icon clicked */
BOOL windiag_update(event_pollblock *event,void *reference);
BOOL windiag_cancel(event_pollblock *event,void *reference);
/* Shade some options depending on state of others */
BOOL windiag_optionclick(event_pollblock *event,void *reference);
/* Make window colours default */
BOOL windiag_clickdefault(event_pollblock *event,void *reference);

/* Update icons in windiag window */
void windiag_seticons(window_block *wblock);

/* Read icons in windiag window into window block */
void windiag_readicons(window_block *wblock);

/* winentry being edited */
browser_winentry *windiag_winentry;

/* Window handles */
window_handle windiag_window, windiag_pane;

/* Height of pane */
int windiag_paneheight;

void windiag_init()
{
  window_block *templat;

  templat = templates_load("WinDiag",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&windiag_window));
  free(templat);
  templat = templates_load("WinPane",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&windiag_pane));
  windiag_paneheight = templat->screenrect.max.y - templat->screenrect.min.y;
  free(templat);

  Event_Claim(event_OPEN,   windiag_window, event_ANY,          windiag_OpenWindow,0);
  Event_Claim(event_CLOSE,  windiag_window, event_ANY,          windiag_CloseWindow,0);
  Event_Claim(event_SCROLL, windiag_window, event_ANY,          globals_scrollevent,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_UPDATE,     windiag_update,0);
  Event_Claim(event_CLICK,  windiag_pane,   windiag_CANCEL,     windiag_cancel,0);
  Event_Claim(event_CLICK,  windiag_pane,   windiag_UPDATE,     windiag_update,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_CANCEL,     windiag_cancel,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_TITLE,      windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_FURNITURE,  windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_PANE,       windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_VSCROLL,    windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_HSCROLL,    windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_COL24BIT,   windiag_optionclick,0);
  Event_Claim(event_CLICK,  windiag_window, windiag_COLDEFAULT, windiag_clickdefault,0);
  help_claim_window(windiag_window,"WEC");
  help_claim_window(windiag_pane,"WEC");

  diagutils_btype(windiag_window,windiag_BUTTONTYPE);
  diagutils_colour(windiag_window,windiag_COLTITLEFORE,TRUE);
  diagutils_colour(windiag_window,windiag_COLTITLEBACK,FALSE);
  diagutils_colour(windiag_window,windiag_COLWORKFORE,FALSE);
  diagutils_colour(windiag_window,windiag_COLWORKBACK,TRUE);
  diagutils_colour(windiag_window,windiag_COLSCROLLOUTER,FALSE);
  diagutils_colour(windiag_window,windiag_COLSCROLLINNER,FALSE);
  diagutils_colour(windiag_window,windiag_COLFOCUS,FALSE);
}

BOOL windiag_OpenWindow(event_pollblock *event, void *ref)
{
  wimp_rect visarea;
  window_state panestate;
  if (choices->editpanes)
  {
    visarea = event->data.openblock.screenrect;
    Error_Check(Wimp_GetWindowState(windiag_pane, &panestate));
    panestate.openblock.screenrect = event->data.openblock.screenrect;
    panestate.openblock.screenrect.max.y =
    	panestate.openblock.screenrect.min.y + windiag_paneheight;
    if (event->data.openblock.behind != windiag_pane)
    {
      panestate.openblock.behind = event->data.openblock.behind;
      event->data.openblock.behind = windiag_pane;
    }
    Error_Check(Wimp_OpenWindow(&panestate.openblock));
  }
  Error_Check(Wimp_OpenWindow(&event->data.openblock));
  if (choices->editpanes &&
  	(event->data.openblock.screenrect.min.x != visarea.min.x ||
  	 event->data.openblock.screenrect.min.y != visarea.min.y ||
  	 event->data.openblock.screenrect.max.x != visarea.max.x ||
  	 event->data.openblock.screenrect.max.y != visarea.max.y))
  {
    panestate.openblock.screenrect = event->data.openblock.screenrect;
    panestate.openblock.screenrect.max.y =
    	panestate.openblock.screenrect.min.y + windiag_paneheight;
    panestate.openblock.behind = event->data.openblock.behind;
    Error_Check(Wimp_OpenWindow(&panestate.openblock));
  }
  return TRUE;
}

BOOL windiag_CloseWindow(event_pollblock *event, void *ref)
{
  windiag_close();
  return TRUE;
}

void windiag_open(browser_winentry *winentry)
{
  window_state wstate;
  event_pollblock dumev;
/*
  mouse_block ptrinfo;
  wimp_point moveby;
*/
  char buffer[28];

/*
  Wimp_GetPointerInfo(&ptrinfo);
*/

  windiag_winentry = winentry;
  windiag_seticons(&winentry->window->window);

  Str_MakeASCIIZ(winentry->identifier,wimp_MAXNAME);
  MsgTrans_LookupPS(messages,"WinDgT",buffer,28,winentry->identifier,0,0,0);
  Window_SetTitle(windiag_window,buffer);

  windiag_optionclick(NULL, NULL);

  Wimp_GetWindowState(windiag_window,&wstate);
/*
  moveby.x = ptrinfo.pos.x - 64 - wstate.openblock.screenrect.min.x;
  moveby.y = ptrinfo.pos.y + 64 - wstate.openblock.screenrect.max.y;
  wstate.openblock.screenrect.min.x += moveby.x;
  wstate.openblock.screenrect.min.y += moveby.y;
  wstate.openblock.screenrect.max.x += moveby.x;
  wstate.openblock.screenrect.max.y += moveby.y;
*/
  dumev.data.openblock = wstate.openblock;
  windiag_OpenWindow(&dumev, 0);
}

BOOL windiag_cancel(event_pollblock *event,void *reference)
{
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  windiag_close();
  return TRUE;
}

BOOL windiag_optionclick(event_pollblock *event,void *reference)
{
  BOOL back = TRUE, close = TRUE, toggle = TRUE,
  	moveable = TRUE, adjust = TRUE, deepcolour = FALSE;

  if (Icon_GetSelect(windiag_window, windiag_COL24BIT))
    deepcolour=TRUE;

  if (!Icon_GetSelect(windiag_window,windiag_TITLE))
  {
    back = close = FALSE;
    if (!Icon_GetSelect(windiag_window,windiag_VSCROLL))
      toggle = FALSE;
  }
  if (Icon_GetSelect(windiag_window,windiag_FURNITURE) ||
  	(choices->strict_panes && Icon_GetSelect(windiag_window,windiag_PANE)))
  {
    moveable = adjust = back = toggle = FALSE;
  }
  if (!Icon_GetSelect(windiag_window,windiag_VSCROLL))
  {
    if (!Icon_GetSelect(windiag_window,windiag_HSCROLL))
      adjust = FALSE;
    if (!Icon_GetSelect(windiag_window,windiag_TITLE))
      toggle = FALSE;
  }

  if (back)
    Icon_Unshade(windiag_window,windiag_BACK);
  else
  {
    Icon_Deselect(windiag_window,windiag_BACK);
    Icon_Shade(windiag_window,windiag_BACK);
  }

  if (close)
    Icon_Unshade(windiag_window,windiag_CLOSE);
  else
  {
    Icon_Deselect(windiag_window,windiag_CLOSE);
    Icon_Shade(windiag_window,windiag_CLOSE);
  }

  if (toggle)
    Icon_Unshade(windiag_window,windiag_TOGGLE);
  else
  {
    Icon_Deselect(windiag_window,windiag_TOGGLE);
    Icon_Shade(windiag_window,windiag_TOGGLE);
  }

  if (moveable)
    Icon_Unshade(windiag_window,windiag_MOVEABLE);
  else
  {
    Icon_Deselect(windiag_window,windiag_MOVEABLE);
    Icon_Shade(windiag_window,windiag_MOVEABLE);
  }

  if (adjust)
    Icon_Unshade(windiag_window,windiag_ADJUST);
  else
  {
    Icon_Deselect(windiag_window,windiag_ADJUST);
    Icon_Shade(windiag_window,windiag_ADJUST);
  }

  if (deepcolour)
    Icon_Shade(windiag_window, windiag_COLGCOL);
  else
    Icon_Unshade(windiag_window, windiag_COLGCOL);

  return TRUE;
}

BOOL windiag_clickdefault(event_pollblock *event,void *reference)
{
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  diagutils_writecolour(windiag_window,windiag_COLTITLEFORE,7);
  diagutils_writecolour(windiag_window,windiag_COLTITLEBACK,2);
  diagutils_writecolour(windiag_window,windiag_COLWORKFORE,7);
  diagutils_writecolour(windiag_window,windiag_COLWORKBACK,1);
  diagutils_writecolour(windiag_window,windiag_COLSCROLLOUTER,3);
  diagutils_writecolour(windiag_window,windiag_COLSCROLLINNER,1);
  diagutils_writecolour(windiag_window,windiag_COLFOCUS,12);
  return TRUE;
}

void windiag_seticons(window_block *wblock)
{
  Icon_SetSelect(windiag_window,windiag_BACK,wblock->flags.data.backicon);
  Icon_SetSelect(windiag_window,windiag_CLOSE,wblock->flags.data.closeicon);
  Icon_SetSelect(windiag_window,windiag_TITLE,wblock->flags.data.titlebar);
  Icon_SetSelect(windiag_window,windiag_TOGGLE,wblock->flags.data.toggleicon);
  Icon_SetSelect(windiag_window,windiag_VSCROLL,wblock->flags.data.vscroll);
  Icon_SetSelect(windiag_window,windiag_ADJUST,wblock->flags.data.adjusticon);
  Icon_SetSelect(windiag_window,windiag_HSCROLL,wblock->flags.data.hscroll);
  Icon_SetSelect(windiag_window,windiag_MOVEABLE,wblock->flags.data.moveable);
  Icon_SetSelect(windiag_window,windiag_AUTOREDRAW, wblock->flags.data.autoredraw);
  Icon_SetSelect(windiag_window,windiag_PANE,wblock->flags.data.pane);
  Icon_SetSelect(windiag_window,windiag_BACKDROP, wblock->flags.data.backwindow);
  Icon_SetSelect(windiag_window,windiag_HOTKEYS,wblock->flags.data.hotkeys);
  Icon_SetSelect(windiag_window,windiag_NOBOUNDS,wblock->flags.data.nobounds);
  Icon_SetSelect(windiag_window,windiag_CONFINE, wblock->flags.data.keeponscreen);
  Icon_SetSelect(windiag_window,windiag_IGNORERIGHT, wblock->flags.data.ignoreright);
  Icon_SetSelect(windiag_window,windiag_IGNOREBOTTOM, wblock->flags.data.ignorebottom);
  Icon_SetSelect(windiag_window,windiag_SCROLLDEBOUNCED, wblock->flags.data.scrollrqdebounced);
  Icon_SetSelect(windiag_window,windiag_SCROLLAUTOREPEAT, wblock->flags.data.scrollrq);
  Icon_SetSelect(windiag_window,windiag_NOSCROLL,
  		 !(wblock->flags.data.scrollrq ||
  		 	wblock->flags.data.scrollrqdebounced));
  Icon_SetSelect(windiag_window,windiag_FURNITURE, wblock->flags.data.childfurniture);

  /* New window flags */
  Icon_SetSelect(windiag_window,windiag_BORDER3DYES,wblock->colours.cols.always3d);
  Icon_SetSelect(windiag_window,windiag_BORDER3DNO,wblock->colours.cols.never3d);
  if (wblock->colours.cols.always3d && wblock->colours.cols.never3d)
  {
    /* If /both/ are set, act as if neither are set */
    Icon_SetSelect(windiag_window,windiag_BORDER3DYES,FALSE);
    Icon_SetSelect(windiag_window,windiag_BORDER3DNO,FALSE);
  }
  Icon_SetSelect(windiag_window,windiag_SHADEDINFO,wblock->colours.cols.returnshaded);
  Icon_SetSelect(windiag_window,windiag_SCROLLEXTENDED,wblock->colours.cols.extendedscroll);
  Icon_SetSelect(windiag_window,windiag_COL24BIT,wblock->colours.cols.fullcolour);

  diagutils_writebtype(windiag_window,windiag_BUTTONTYPE, wblock->workflags.data.buttontype);
  /*
  if ((int) wblock->spritearea == 1)
  {
    Icon_Select(windiag_window,windiag_WIMPSPRITES);
    Icon_Deselect(windiag_window,windiag_USERSPRITES);
  }
  else
  {
    Icon_Deselect(windiag_window,windiag_WIMPSPRITES);
    Icon_Select(windiag_window,windiag_USERSPRITES);
  }
  */
  diagutils_writecolour(windiag_window,windiag_COLTITLEFORE,wblock->colours.vals.colours[0]);
  diagutils_writecolour(windiag_window,windiag_COLTITLEBACK,wblock->colours.vals.colours[1]);
  diagutils_writecolour(windiag_window,windiag_COLWORKFORE,wblock->colours.vals.colours[2]);
  diagutils_writecolour(windiag_window,windiag_COLWORKBACK,wblock->colours.vals.colours[3]);
  diagutils_writecolour(windiag_window,windiag_COLSCROLLOUTER,wblock->colours.vals.colours[4]);
  diagutils_writecolour(windiag_window,windiag_COLSCROLLINNER,wblock->colours.vals.colours[5]);
  diagutils_writecolour(windiag_window,windiag_COLFOCUS,wblock->colours.vals.colours[6]);
  Icon_SetSelect(windiag_window,windiag_COLGCOL,wblock->flags.data.realcolours);
}

void windiag_readicons(window_block *wblock)
{
  wblock->flags.data.backicon          = Icon_GetSelect(windiag_window, windiag_BACK);
  wblock->flags.data.closeicon         = Icon_GetSelect(windiag_window, windiag_CLOSE);
  wblock->flags.data.titlebar          = Icon_GetSelect(windiag_window, windiag_TITLE);
  wblock->flags.data.toggleicon        = Icon_GetSelect(windiag_window, windiag_TOGGLE);
  wblock->flags.data.vscroll           = Icon_GetSelect(windiag_window, windiag_VSCROLL);
  wblock->flags.data.adjusticon        = Icon_GetSelect(windiag_window, windiag_ADJUST);
  wblock->flags.data.hscroll           = Icon_GetSelect(windiag_window, windiag_HSCROLL);
  wblock->flags.data.moveable          = Icon_GetSelect(windiag_window, windiag_MOVEABLE);
  wblock->flags.data.autoredraw        = Icon_GetSelect(windiag_window, windiag_AUTOREDRAW);
  wblock->flags.data.pane              = Icon_GetSelect(windiag_window, windiag_PANE);
  wblock->flags.data.backwindow        = Icon_GetSelect(windiag_window, windiag_BACKDROP);
  wblock->flags.data.hotkeys           = Icon_GetSelect(windiag_window, windiag_HOTKEYS);
  wblock->flags.data.nobounds          = Icon_GetSelect(windiag_window, windiag_NOBOUNDS);
  wblock->flags.data.keeponscreen      = Icon_GetSelect(windiag_window, windiag_CONFINE);
  wblock->flags.data.ignoreright       = Icon_GetSelect(windiag_window, windiag_IGNORERIGHT);
  wblock->flags.data.ignorebottom      = Icon_GetSelect(windiag_window, windiag_IGNOREBOTTOM);
  wblock->flags.data.scrollrqdebounced = Icon_GetSelect(windiag_window, windiag_SCROLLDEBOUNCED);
  wblock->flags.data.scrollrq          = Icon_GetSelect(windiag_window, windiag_SCROLLAUTOREPEAT);
  wblock->flags.data.childfurniture    = Icon_GetSelect(windiag_window, windiag_FURNITURE);
  wblock->workflags.data.buttontype    = diagutils_readbtype(windiag_window,windiag_BUTTONTYPE);

  /* New window flags */
  wblock->colours.cols.always3d        = Icon_GetSelect(windiag_window, windiag_BORDER3DYES);
  wblock->colours.cols.never3d         = Icon_GetSelect(windiag_window, windiag_BORDER3DNO);
  wblock->colours.cols.returnshaded    = Icon_GetSelect(windiag_window, windiag_SHADEDINFO);
  wblock->colours.cols.extendedscroll  = Icon_GetSelect(windiag_window, windiag_SCROLLEXTENDED);
  wblock->colours.cols.fullcolour      = Icon_GetSelect(windiag_window, windiag_COL24BIT);

  /*if (Icon_GetSelect(windiag_window,windiag_USERSPRITES))*/
  wblock->spritearea = user_sprites;
  /*else
    wblock->spritearea = (void *) 1;*/
  wblock->colours.vals.colours[0] =
    diagutils_readcolour(windiag_window,windiag_COLTITLEFORE);
  wblock->colours.vals.colours[1] =
    diagutils_readcolour(windiag_window,windiag_COLTITLEBACK);
  wblock->colours.vals.colours[2] =
    diagutils_readcolour(windiag_window,windiag_COLWORKFORE);
  wblock->colours.vals.colours[3] =
    diagutils_readcolour(windiag_window,windiag_COLWORKBACK);
  wblock->colours.vals.colours[4] =
    diagutils_readcolour(windiag_window,windiag_COLSCROLLOUTER);
  wblock->colours.vals.colours[5] =
    diagutils_readcolour(windiag_window,windiag_COLSCROLLINNER);
  wblock->colours.vals.colours[6] =
    diagutils_readcolour(windiag_window,windiag_COLFOCUS);
  wblock->flags.data.realcolours =
    Icon_GetSelect(windiag_window,windiag_COLGCOL);
}

BOOL windiag_update(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;

  windiag_readicons(&windiag_winentry->window->window);
  viewer_reopen(windiag_winentry,FALSE);
  browser_settitle(windiag_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    windiag_close();

  Window_GainCaret(windiag_winentry->handle);

  return TRUE;
}

void windiag_close()
{
  return_caret(windiag_window, windiag_winentry->handle);
  Wimp_CloseWindow(windiag_window);
  if (choices->editpanes)
    Wimp_CloseWindow(windiag_pane);
}

void windiag_responder(choices_str *old, choices_str *new_ch)
{
  window_state wstate;

  Wimp_GetWindowState(windiag_window, &wstate);
  if (!wstate.flags.data.open) return;

  if (old->editpanes && !new_ch->editpanes)
    Wimp_CloseWindow(windiag_pane);
  else if (!old->editpanes && new_ch->editpanes)
  {
    event_pollblock dummyev;

    dummyev.data.openblock = wstate.openblock;
    windiag_OpenWindow(&dummyev, 0);
  }
  if (old->strict_panes != new_ch->strict_panes)
    windiag_optionclick(NULL,NULL);
}
