/* multiicon.c */

#include "common.h"

#include "diagutils.h"
#include "icnedit.h"
#include "multiicon.h"
#include "title.h"

typedef enum {
  multiicon_CANCEL = 61,
  multiicon_UPDATE,

  multiicon_BORDER = 3,
  multiicon_HCENTRE = 7,
  multiicon_VCENTRE = 15,
  multiicon_RALIGN = 19,
  multiicon_FILLED = 23,
  multiicon_NEEDSHELP = 27,
  multiicon_ALLOWADJUST = 34,
  multiicon_HALFSIZE = 38,
  multiicon_SELECTED = 42,
  multiicon_SHADED = 46,
  multiicon_DELETED = 50,

  multiicon_BTNTYPEOPT = 54, multiicon_BTNTYPE, multiicon_BTNMENU,

  multiicon_ESGOPT = 57, multiicon_ESG, multiicon_DECESG, multiicon_INCESG

  /*
  multiicon_FORECOLOPT = 63, multiicon_FORECOL,
  multiicon_DECFORECOL, multiicon_INCFORECOL, multiicon_FORECOLMENU,
  multiicon_BACKCOLOPT = 68, multiicon_BACKCOL,
  multiicon_DECBACKCOL, multiicon_INCBACKCOL, multiicon_BACKCOLMENU
  */
} multiicon_icons;

/* Handlers */
BOOL multiicon_CloseWindow(event_pollblock *event,void *reference);
BOOL multiicon_keypress(event_pollblock *event,void *reference);
BOOL multiicon_cancel(event_pollblock *event,void *reference);

/* When an option button requires fading to be changed */
BOOL multiicon_affect(event_pollblock *event,void *reference);

/* Update icon clicked */
BOOL multiicon_update(event_pollblock *event,void *reference);

/* Reset icons in multiicon window */
void multiicon_reseticons(void);

/* What's being edited */
browser_winentry *multiicon_winentry;

/* Window handle */
window_handle multiicon_window;

void multiicon_init()
{
  window_block *templat;

  templat = templates_load("MultiIcon",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&multiicon_window));
  free(templat);
  Event_Claim(event_OPEN,multiicon_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,multiicon_window,event_ANY,multiicon_CloseWindow,0);
  Event_Claim(event_CLICK,multiicon_window,multiicon_UPDATE,multiicon_update,0);
  Event_Claim(event_CLICK,multiicon_window,multiicon_CANCEL,multiicon_cancel,0);
  Event_Claim(event_KEY,multiicon_window,event_ANY,multiicon_keypress,0);
  Event_Claim(event_CLICK,multiicon_window,multiicon_BTNTYPEOPT,
  	multiicon_affect,0);
  Event_Claim(event_CLICK,multiicon_window,multiicon_ESGOPT,
  	multiicon_affect,0);
  /*
  Event_Claim(event_CLICK,multiicon_window,multiicon_FORECOLOPT,
  	multiicon_affect,0);
  Event_Claim(event_CLICK,multiicon_window,multiicon_BACKCOLOPT,
  	multiicon_affect,0);
  diagutils_colour(multiicon_window,multiicon_FORECOL,FALSE);
  diagutils_colour(multiicon_window,multiicon_BACKCOL,FALSE);
  */
  diagutils_bumpers(multiicon_window,multiicon_ESG,0,31,1);
  diagutils_btype(multiicon_window,multiicon_BTNTYPE);
  help_claim_window(multiicon_window,"MIE");
}

BOOL multiicon_cancel(event_pollblock *event,void *reference)
{
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  multiicon_close();
  return TRUE;
}

BOOL multiicon_OpenWindow(event_pollblock *event, void *ref)
{
  Error_Check(Wimp_OpenWindow(&event->data.openblock));
  return TRUE;
}

BOOL multiicon_CloseWindow(event_pollblock *event, void *ref)
{
  multiicon_close();
  return TRUE;
}

void multiicon_open(browser_winentry *winentry)
{
  window_state wstate;

  multiicon_winentry = winentry;

  multiicon_reseticons();

  Wimp_GetWindowState(multiicon_window,&wstate);
/*
  moveby.x = ptrinfo.pos.x - 64 - wstate.openblock.screenrect.min.x;
  moveby.y = ptrinfo.pos.y + 64 - wstate.openblock.screenrect.max.y;
  wstate.openblock.screenrect.min.x += moveby.x;
  wstate.openblock.screenrect.min.y += moveby.y;
  wstate.openblock.screenrect.max.x += moveby.x;
  wstate.openblock.screenrect.max.y += moveby.y;
*/
  Wimp_OpenWindow(&wstate.openblock);
  /*Icon_SetCaret(multiicon_window,multiicon_ESG);*/
}

#define multiicon_SETRADIO(i) Icon_SetRadios(multiicon_window,(i),(i)+2,(i)+2)

void multiicon_reseticons()
{
  multiicon_SETRADIO(multiicon_BORDER);
  multiicon_SETRADIO(multiicon_HCENTRE);
  multiicon_SETRADIO(multiicon_VCENTRE);
  multiicon_SETRADIO(multiicon_RALIGN);
  multiicon_SETRADIO(multiicon_FILLED);
  multiicon_SETRADIO(multiicon_NEEDSHELP);
  multiicon_SETRADIO(multiicon_ALLOWADJUST);
  multiicon_SETRADIO(multiicon_HALFSIZE);
  multiicon_SETRADIO(multiicon_SELECTED);
  multiicon_SETRADIO(multiicon_SHADED);
  multiicon_SETRADIO(multiicon_DELETED);

  Icon_Deselect(multiicon_window, multiicon_BTNTYPEOPT);
  diagutils_writebtype(multiicon_window, multiicon_BTNTYPE, 0);

  Icon_Deselect(multiicon_window, multiicon_ESGOPT);
  Icon_SetInteger(multiicon_window, multiicon_ESG, 0);

/*
  Icon_Deselect(multiicon_window, multiicon_FORECOLOPT);
  diagutils_writecolour(multiicon_window, multiicon_FORECOL, colour_BLACK);
  Icon_Deselect(multiicon_window, multiicon_BACKCOLOPT);
  diagutils_writecolour(multiicon_window, multiicon_BACKCOL, colour_GREY1);
*/

  multiicon_affect(0, 0);
}

BOOL multiicon_affect(event_pollblock *event,void *reference)
{
  BOOL fade;

  fade = !Icon_GetSelect(multiicon_window, multiicon_BTNTYPEOPT);
  Icon_SetShade(multiicon_window, multiicon_BTNTYPE, fade);
  Icon_SetShade(multiicon_window, multiicon_BTNMENU, fade);

  fade = !Icon_GetSelect(multiicon_window, multiicon_ESGOPT);
  Icon_SetShade(multiicon_window, multiicon_ESG, fade);
  Icon_SetShade(multiicon_window, multiicon_DECESG, fade);
  Icon_SetShade(multiicon_window, multiicon_INCESG, fade);

/*
  fade = !Icon_GetSelect(multiicon_window, multiicon_FORECOLOPT);
  Icon_SetShade(multiicon_window, multiicon_FORECOL, fade);
  Icon_SetShade(multiicon_window, multiicon_DECFORECOL, fade);
  Icon_SetShade(multiicon_window, multiicon_INCFORECOL, fade);
  Icon_SetShade(multiicon_window, multiicon_FORECOLMENU, fade);

  fade = !Icon_GetSelect(multiicon_window, multiicon_BACKCOLOPT);
  Icon_SetShade(multiicon_window, multiicon_BACKCOL, fade);
  Icon_SetShade(multiicon_window, multiicon_DECBACKCOL, fade);
  Icon_SetShade(multiicon_window, multiicon_INCBACKCOL, fade);
  Icon_SetShade(multiicon_window, multiicon_BACKCOLMENU, fade);
*/

  return TRUE;
}

BOOL multiicon_keypress(event_pollblock *event,void *reference)
{
  if (event->data.key.code == 13)
  {
    event_pollblock dummyev;

    dummyev.type = event_CLICK;
    dummyev.data.mouse.window = multiicon_window;
    dummyev.data.mouse.icon = multiicon_UPDATE;
    dummyev.data.mouse.button.value = button_SELECT;
    return multiicon_update(&dummyev,reference);
  }
  else if (event->data.key.code == 27)
  {
    multiicon_close();
    return TRUE;
  }
  Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

static void multiicon_setflag(icon_flags *flags, int icon, int flag)
{
  switch (Icon_WhichRadio(multiicon_window, icon, icon + 2) - icon)
  {
    case 0:
      flags->value &= ~flag;
      flags->value |= flag;
      break;
    case 1:
      flags->value &= ~flag;
      break;
    case 2:
      break;
  }
}

BOOL multiicon_update(event_pollblock *event,void *reference)
{
  int icon;

  if (!event->data.mouse.button.data.adjust &&
      !event->data.mouse.button.data.select)
    return FALSE;

  for (icon = 0; icon < multiicon_winentry->window->window.numicons; ++icon)
  {
    if (Icon_GetSelect(multiicon_winentry->handle, icon))
    {
      icon_flags flags = multiicon_winentry->window->icon[icon].flags;

      multiicon_setflag(&flags, multiicon_BORDER, icon_BORDER);
      multiicon_setflag(&flags, multiicon_HCENTRE, icon_HCENTRE);
      multiicon_setflag(&flags, multiicon_VCENTRE, icon_VCENTRE);
      multiicon_setflag(&flags, multiicon_RALIGN, icon_RJUSTIFY);
      multiicon_setflag(&flags, multiicon_FILLED, icon_FILLED);
      multiicon_setflag(&flags, multiicon_NEEDSHELP, icon_NEEDSHELP);
      multiicon_setflag(&flags, multiicon_ALLOWADJUST, icon_ALLOWADJUST);
      multiicon_setflag(&flags, multiicon_HALFSIZE, icon_HALVESPRITE);
      multiicon_setflag(&flags, multiicon_SELECTED, icon_SELECTED);
      multiicon_setflag(&flags, multiicon_SHADED, icon_SHADED);
      multiicon_setflag(&flags, multiicon_DELETED, icon_DELETED);

      if (Icon_GetSelect(multiicon_window, multiicon_BTNTYPEOPT))
      {
        flags.data.buttontype =
        	diagutils_readbtype(multiicon_window, multiicon_BTNTYPE);
      }

      if (Icon_GetSelect(multiicon_window, multiicon_ESGOPT))
      {
        int esg = Icon_GetInteger(multiicon_window, multiicon_ESG);

        if (esg < 0)
          Icon_SetInteger(multiicon_window, multiicon_ESG, esg = 0);
        else if (esg > 31)
          Icon_SetInteger(multiicon_window, multiicon_ESG, esg = 31);
        flags.data.esg = esg;

      }
      /*
      if (Icon_GetSelect(multiicon_window, multiicon_FORECOLOPT))
      {
        flags.data.foreground =
        	diagutils_readcolour(multiicon_window, multiicon_FORECOL);
      }

      if (Icon_GetSelect(multiicon_window, multiicon_BACKCOLOPT))
      {
        flags.data.foreground =
        	diagutils_readcolour(multiicon_window, multiicon_BACKCOL);
      }
      */
      multiicon_winentry->window->icon[icon].flags = flags;
      icnedit_makeeditableflags(multiicon_winentry, &flags);
      Error_Check(Wimp_SetIconState(multiicon_winentry->handle, icon,
      	flags.value, (int) 0xffffffff));
    }
  }

  browser_settitle(multiicon_winentry->browser,0,TRUE);

  if (event->data.mouse.button.data.select)
    multiicon_close();

  return TRUE;
}

void multiicon_close()
{
  return_caret(multiicon_window, multiicon_winentry->handle);
  Wimp_CloseWindow(multiicon_window);
}
