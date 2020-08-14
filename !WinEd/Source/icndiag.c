/* icndiag.c */

#include "common.h"

#include "DeskLib:Validation.h"

#include "diagutils.h"
#include "fontpick.h"
#include "icndiag.h"
#include "icnedit.h"
#include "tempfont.h"
#include "title.h"
#include "usersprt.h"
#include "viewex.h"

typedef enum {
  icndiag_UPDATE,
  icndiag_ALTUPDATE,
  icndiag_CANCEL,
  icndiag_STRING = 6,
  icndiag_TEXT,
  icndiag_SPRITE,
  icndiag_INDIRECTED,
  icndiag_VALID = 11,
  icndiag_MAXLEN = 13,
  icndiag_MINMAX,
  icndiag_BTNTYPE = 16,
  icndiag_BTNMENU,
  icndiag_BORDER = 20,
  icndiag_HCENTRE,
  icndiag_VCENTRE,
  icndiag_RALIGN,
  icndiag_FILLED,
  icndiag_NEEDSHELP,
  icndiag_ALLOWADJUST,
  icndiag_HALFSIZE,
  icndiag_SELECTED,
  icndiag_SHADED,
  icndiag_DELETED,
  icndiag_NUMERIC,
  icndiag_ESG = 33,
  icndiag_DECESG,
  icndiag_INCESG,
  icndiag_FORECOL = 37,
  icndiag_DECFORECOL,
  icndiag_INCFORECOL,
  icndiag_FORECOLMENU,
  icndiag_BACKCOL = 42,
  icndiag_DECBACKCOL,
  icndiag_INCBACKCOL,
  icndiag_BACKCOLMENU,
  icndiag_ANTIALIASED = 47,
  icndiag_FONTNAME,
  icndiag_FONTMENU,
  icndiag_FONTHEIGHT = 51,
  icndiag_DECFONTHEIGHT,
  icndiag_INCFONTHEIGHT,
  icndiag_FONTASPECT = 56,
  icndiag_FONTVALID = 58,
} icndiag_icons;

/* Handlers */
BOOL icndiag_OpenWindow(event_pollblock *event,void *reference);
BOOL icndiag_CloseWindow(event_pollblock *event,void *reference);
BOOL icndiag_keypress(event_pollblock *event,void *reference);
BOOL icndiag_fontmenu(event_pollblock *event,void *reference);
BOOL icndiag_choosefont(event_pollblock *event,void *reference);
BOOL icndiag_releasefontmenu(event_pollblock *event,void *reference);
BOOL icndiag_fontvalid(event_pollblock *event,void *reference);
BOOL icndiag_minmax(event_pollblock *event,void *reference);
BOOL icndiag_clickcancel(event_pollblock *event,void *reference);

/* Update or 'Don't resize' icon clicked */
BOOL icndiag_update(event_pollblock *event,void *reference);

/* Update icons in icndiag window */
void icndiag_seticons(browser_winentry *winentry,int icon);

/* Affects any flags which depend on others; suitable as handler for
   affecting icons */
BOOL icndiag_affect(event_pollblock *event,void *reference);

/* Read icons in icndiag window into icon block. If indirected, the pointers
   point to the icndiag buffers */
void icndiag_readicons(icon_block *block);

/* Provide our own interactive help support. */
BOOL icndiag_help(event_pollblock *event,void *reference);

/* What's being edited */
browser_winentry *icndiag_winentry;
icon_handle icndiag_icon;

/* Window handles */
window_handle icndiag_window, icndiag_pane;
int icndiag_paneheight;

/* Font menu */
static menu_ptr icndiag_menu;

static char *strterm(char *s)
{
  int n;
  if (!s) return 0;
  for (n=0; s[n]>31; ++n);
    s[n] = 0;
  return s;
}

void icndiag_init()
{
  window_block *templat;

  templat = templates_load("IcnEdit",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&icndiag_window));
  free(templat);
  templat = templates_load("IcnPane",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&icndiag_pane));
  icndiag_paneheight = templat->screenrect.max.y - templat->screenrect.min.y;
  free(templat);
  Event_Claim(event_OPEN,icndiag_window,event_ANY,icndiag_OpenWindow,0);
  Event_Claim(event_CLOSE,icndiag_window,event_ANY,icndiag_CloseWindow,0);
  Event_Claim(event_SCROLL,icndiag_window,event_ANY,globals_scrollevent,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_UPDATE,icndiag_update,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_ALTUPDATE,icndiag_update,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_CANCEL,icndiag_clickcancel,0);
  Event_Claim(event_CLICK,icndiag_pane,icndiag_UPDATE,icndiag_update,0);
  Event_Claim(event_CLICK,icndiag_pane,icndiag_ALTUPDATE,icndiag_update,0);
  Event_Claim(event_CLICK,icndiag_pane,icndiag_CANCEL,icndiag_clickcancel,0);
  Event_Claim(event_KEY,icndiag_window,event_ANY,icndiag_keypress,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_TEXT,icndiag_affect,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_SPRITE,icndiag_affect,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_INDIRECTED,icndiag_affect,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_TEXT,icndiag_affect,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_ANTIALIASED,icndiag_affect,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_FONTVALID,icndiag_fontvalid,0);
  Event_Claim(event_CLICK,icndiag_window,icndiag_MINMAX,icndiag_minmax,0);
  diagutils_bumpers(icndiag_window,icndiag_ESG,0,15,1);
  diagutils_bumpers(icndiag_window,icndiag_FONTHEIGHT,2,200,2);
  diagutils_colour(icndiag_window,icndiag_FORECOL,FALSE);
  diagutils_colour(icndiag_window,icndiag_BACKCOL,FALSE);
  diagutils_btype(icndiag_window,icndiag_BTNTYPE);
  Event_Claim(event_CLICK,icndiag_window,icndiag_FONTMENU,icndiag_fontmenu,0);
  EventMsg_Claim(message_HELPREQUEST, icndiag_window, icndiag_help,(void *) "ICC");
  EventMsg_Claim(message_HELPREQUEST, icndiag_pane, icndiag_help,(void *) "ICC");
}

BOOL icndiag_clickcancel(event_pollblock *event,void *reference)
{
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  icndiag_close();
  return TRUE;
}

BOOL icndiag_OpenWindow(event_pollblock *event, void *ref)
{
  wimp_rect visarea;
  window_state panestate;

  Log(log_DEBUG, "icndiag_OpenWindow");

  if (choices->editpanes)
  {
    visarea = event->data.openblock.screenrect;
    Error_Check(Wimp_GetWindowState(icndiag_pane, &panestate));
    panestate.openblock.screenrect = event->data.openblock.screenrect;
    panestate.openblock.screenrect.max.y =
    	panestate.openblock.screenrect.min.y + icndiag_paneheight;
    if (event->data.openblock.behind != icndiag_pane)
    {
      panestate.openblock.behind = event->data.openblock.behind;
      event->data.openblock.behind = icndiag_pane;
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
    	panestate.openblock.screenrect.min.y + icndiag_paneheight;
    panestate.openblock.behind = event->data.openblock.behind;
    Error_Check(Wimp_OpenWindow(&panestate.openblock));
  }
  return TRUE;
}

BOOL icndiag_CloseWindow(event_pollblock *event, void *ref)
{
  icndiag_close();
  return TRUE;
}

void icndiag_open(browser_winentry *winentry,icon_handle icon)
{
  window_state wstate;
  event_pollblock dumev;
/*
  mouse_block ptrinfo;
  wimp_point moveby;
*/
  char numstring[8],buffer[16];

/*
  Wimp_GetPointerInfo(&ptrinfo);
*/

  icndiag_winentry = winentry;
  icndiag_icon = icon;

  icndiag_seticons(winentry,icon);

  if (icndiag_icon == -1)
  {
    MsgTrans_Lookup(messages,"IcnDgTT",buffer,16);
    Icon_Shade(icndiag_window,icndiag_ALTUPDATE);
    Icon_Shade(icndiag_pane,icndiag_ALTUPDATE);
  }
  else
  {
    sprintf(numstring,"%d",icon);
    MsgTrans_LookupPS(messages,"IcnDgTI",buffer,16,numstring,0,0,0);
    Icon_Unshade(icndiag_window,icndiag_ALTUPDATE);
    Icon_Unshade(icndiag_pane,icndiag_ALTUPDATE);
  }
  Window_SetTitle(icndiag_window,buffer);

  if (choices->safe_icons == TRUE)
    MsgTrans_Lookup(messages,"IcnDgAdjust",buffer,16);
  else
    MsgTrans_Lookup(messages,"IcnDgDont",buffer,16);
  Icon_SetText(icndiag_window, icndiag_ALTUPDATE, buffer);
  Icon_SetText(icndiag_pane, icndiag_ALTUPDATE, buffer);

  Wimp_GetWindowState(icndiag_window,&wstate);
/*
  moveby.x = ptrinfo.pos.x - 64 - wstate.openblock.screenrect.min.x;
  moveby.y = ptrinfo.pos.y + 64 - wstate.openblock.screenrect.max.y;
  wstate.openblock.screenrect.min.x += moveby.x;
  wstate.openblock.screenrect.min.y += moveby.y;
  wstate.openblock.screenrect.max.x += moveby.x;
  wstate.openblock.screenrect.max.y += moveby.y;
*/
  dumev.data.openblock = wstate.openblock;
  icndiag_OpenWindow(&dumev, 0);
  Icon_SetCaret(icndiag_window,icndiag_STRING);
}

/**
 * Set the icons in the Icon Edit dialogue for a given window and icon.
 * 
 * \param *winentry   The window containing the icon of interest.
 * \param icon        The index of the required icon, or -1 for the title.
 */
void icndiag_seticons(browser_winentry *winentry,int icon)
{
  icon_data *data;
  icon_flags *flags;

  /* Make shortcut pointers to the icon or title data/flags */
  if (icndiag_icon == -1)
  {
    data = &winentry->window->window.title;
    flags = &winentry->window->window.titleflags;
  }
  else
  {
    data = &winentry->window->icon[icon].data;
    flags = &winentry->window->icon[icon].flags;
  }

  /* Set the icon data fields. */
  if (flags->data.indirected)
  {
    /* Indirected icon. */
    Icon_Select(icndiag_window,icndiag_INDIRECTED);

    /* Set the icon text or sprite name. */
    if (flags->data.text ||
        (flags->data.sprite && data->indirectsprite.nameisname))
    {
      Icon_SetText(icndiag_window,icndiag_STRING,
      		   strterm(data->indirecttext.buffer + (int) winentry->window));
    }
    else
      Icon_SetText(icndiag_window,icndiag_STRING,"");

    /* Set the validation string and maximum text length */
    if (flags->data.text)
    {
      Icon_SetInteger(icndiag_window,icndiag_MAXLEN,
              data->indirecttext.bufflen);
      Icon_SetText(icndiag_window,icndiag_VALID,
              strterm(data->indirecttext.validstring +
              (int) winentry->window));
    }
    else if (flags->data.sprite && data->indirectsprite.nameisname)
    {
      Icon_SetInteger(icndiag_window,icndiag_MAXLEN,
              data->indirectsprite.nameisname);
      Icon_SetText(icndiag_window,icndiag_VALID,"");
    }
    else
    {
      Icon_SetText(icndiag_window,icndiag_MAXLEN,"");
      Icon_SetText(icndiag_window,icndiag_VALID,"");
    }
  }
  else
  {
    /* Non-indirected icon. */
    Icon_Deselect(icndiag_window,icndiag_INDIRECTED);

    /* Set the icon text or sprite name. */
    if (flags->data.text || flags->data.sprite)
    {
      char buffer[13];
      strncpycr(buffer, data->text, 12);
      buffer[12] = 0;
      Icon_SetText(icndiag_window,icndiag_STRING,buffer);
    }
    else
      Icon_SetText(icndiag_window,icndiag_STRING,"");
  }

  /* Set the text and sprite flags. */
  Icon_SetSelect(icndiag_window,icndiag_TEXT,flags->data.text);
  Icon_SetSelect(icndiag_window,icndiag_SPRITE,flags->data.sprite);

  /* Button type */
  diagutils_writebtype(icndiag_window,icndiag_BTNTYPE,flags->data.buttontype);

  /* Flags */
  Icon_SetSelect(icndiag_window, icndiag_BORDER,      flags->data.border);
  Icon_SetSelect(icndiag_window, icndiag_HCENTRE,     flags->data.hcentre);
  Icon_SetSelect(icndiag_window, icndiag_VCENTRE,     flags->data.vcentre);
  Icon_SetSelect(icndiag_window, icndiag_RALIGN,      flags->data.rightjustify);
  Icon_SetSelect(icndiag_window, icndiag_FILLED,      flags->data.filled);
  Icon_SetSelect(icndiag_window, icndiag_NEEDSHELP,   flags->data.needshelp);
  Icon_SetSelect(icndiag_window, icndiag_ALLOWADJUST, flags->data.allowadjust);
  Icon_SetSelect(icndiag_window, icndiag_HALFSIZE,    flags->data.halfsize);
  Icon_SetSelect(icndiag_window, icndiag_SELECTED,    flags->data.selected);
  Icon_SetSelect(icndiag_window, icndiag_SHADED,      flags->data.shaded);
  Icon_SetSelect(icndiag_window, icndiag_DELETED,     flags->data.deleted);
  Icon_SetSelect(icndiag_window, icndiag_NUMERIC,     flags->data.numeric);

  /* ESG */
  Icon_SetInteger(icndiag_window,icndiag_ESG,flags->data.esg);

  /* Colours */
  if (flags->data.font)
  {
    char *valid;
    int validF;
    int foreground = 7;
    int background = 0;

    if (flags->data.indirected && flags->data.text)
    {
      valid = data->indirecttext.validstring + (int) winentry->window;
      validF = Validation_ScanString(valid,'F');
      if (validF)
      {
        char string[2] = " ";

        string[0] = valid[validF + 2];
        sscanf(string,"%x",&foreground);
        string[0] = valid[validF + 1];
        sscanf(string,"%x",&background);
      }
    }
    diagutils_writecolour(icndiag_window,icndiag_FORECOL,foreground);
    diagutils_writecolour(icndiag_window,icndiag_FORECOL,background);
  }
  else
  {
    diagutils_writecolour(icndiag_window,icndiag_FORECOL,
    			  flags->data.foreground);
    diagutils_writecolour(icndiag_window,icndiag_BACKCOL,
    			  flags->data.background);
  }

  /* Font */
  Icon_SetSelect(icndiag_window,icndiag_ANTIALIASED,flags->data.font);
  if (flags->data.font)
  {
    template_fontinfo *fontinfo =
      &winentry->browser->fontinfo[flags->font.handle - 1];

    Icon_SetText(icndiag_window,icndiag_FONTNAME,strterm(fontinfo->name));
    Icon_SetDouble(icndiag_window,icndiag_FONTHEIGHT,
    		   fontinfo->size.y / 16.0,
    		   4);
    Icon_SetDouble(icndiag_window,icndiag_FONTASPECT,
    		   ((double) fontinfo->size.x/fontinfo->size.y) * 100,
    		   2);
  }
  else /* Fill in suitable default font */
  {
    Icon_SetText(icndiag_window,icndiag_FONTNAME,"Homerton.Medium");
    Icon_SetText(icndiag_window,icndiag_FONTHEIGHT,"12");
    Icon_SetText(icndiag_window,icndiag_FONTASPECT,"100");
  }

  /* Do appropriate shadings etc */
  icndiag_affect(0,0);
}

/**
 * Dynamically update the contents of the dialogue box as the
 * user interacts with it or the data changes.
 * 
 * \param *event  The
 * \param *reference  
 * \return            TRUE if the event was handled.
 */
BOOL icndiag_affect(event_pollblock *event,void *reference)
{
  int len;

  /* Force indirected if text >= 12 chars */
  len = strlencr(Icon_GetTextPtr(icndiag_window,icndiag_STRING));
  if (len >= 12 && Icon_GetSelect(icndiag_window,icndiag_TEXT))
    Icon_Select(icndiag_window,icndiag_INDIRECTED);
  if (!Icon_GetSelect(icndiag_window,icndiag_TEXT) &&
      !Icon_GetSelect(icndiag_window,icndiag_SPRITE))
  {
    Icon_SetText(icndiag_window,icndiag_STRING,"");
    Icon_Shade(icndiag_window,icndiag_STRING);
  }
  else
    Icon_Unshade(icndiag_window,icndiag_STRING);
  if (Icon_GetSelect(icndiag_window,icndiag_INDIRECTED))
  {
    if (Icon_GetSelect(icndiag_window,icndiag_TEXT))
      Icon_Unshade(icndiag_window,icndiag_VALID);
    else
      Icon_Shade(icndiag_window,icndiag_VALID);
    Icon_Unshade(icndiag_window,icndiag_MAXLEN);
    Icon_Unshade(icndiag_window,icndiag_MINMAX);
    if (len >= Icon_GetInteger(icndiag_window,icndiag_MAXLEN))
      Icon_SetInteger(icndiag_window,icndiag_MAXLEN,len + 1);
  }
  else
  {
    Icon_SetText(icndiag_window,icndiag_VALID,"");
    Icon_Shade(icndiag_window,icndiag_VALID);
    Icon_SetText(icndiag_window,icndiag_MAXLEN,"");
    Icon_Shade(icndiag_window,icndiag_MAXLEN);
    Icon_Shade(icndiag_window,icndiag_MINMAX);
  }

  if (Icon_GetSelect(icndiag_window,icndiag_ANTIALIASED))
  {
    Icon_Unshade(icndiag_window,icndiag_FONTNAME);
    Icon_Unshade(icndiag_window,icndiag_FONTMENU);
    Icon_Unshade(icndiag_window,icndiag_FONTHEIGHT);
    Icon_Unshade(icndiag_window,icndiag_DECFONTHEIGHT);
    Icon_Unshade(icndiag_window,icndiag_INCFONTHEIGHT);
    Icon_Unshade(icndiag_window,icndiag_FONTASPECT);
    Icon_Unshade(icndiag_window,icndiag_FONTVALID);
  }
  else
  {
    Icon_Shade(icndiag_window,icndiag_FONTNAME);
    Icon_Shade(icndiag_window,icndiag_FONTMENU);
    Icon_Shade(icndiag_window,icndiag_FONTHEIGHT);
    Icon_Shade(icndiag_window,icndiag_DECFONTHEIGHT);
    Icon_Shade(icndiag_window,icndiag_INCFONTHEIGHT);
    Icon_Shade(icndiag_window,icndiag_FONTASPECT);
    Icon_Shade(icndiag_window,icndiag_FONTVALID);
  }

  if (icndiag_icon == -1)
  {
    Icon_Shade(icndiag_window,icndiag_BORDER);
    Icon_Shade(icndiag_window,icndiag_FILLED);
    Icon_Shade(icndiag_window,icndiag_NEEDSHELP);
    Icon_Shade(icndiag_window,icndiag_BTNTYPE);
    Icon_Shade(icndiag_window,icndiag_BTNMENU);
    Icon_Shade(icndiag_window,icndiag_ESG);
    Icon_Shade(icndiag_window,icndiag_DECESG);
    Icon_Shade(icndiag_window,icndiag_INCESG);
    Icon_Shade(icndiag_window,icndiag_FORECOL);
    Icon_Shade(icndiag_window,icndiag_DECFORECOL);
    Icon_Shade(icndiag_window,icndiag_INCFORECOL);
    Icon_Shade(icndiag_window,icndiag_FORECOLMENU);
    Icon_Shade(icndiag_window,icndiag_BACKCOL);
    Icon_Shade(icndiag_window,icndiag_DECBACKCOL);
    Icon_Shade(icndiag_window,icndiag_INCBACKCOL);
    Icon_Shade(icndiag_window,icndiag_BACKCOLMENU);
  }
  else
  {
    Icon_Unshade(icndiag_window,icndiag_BORDER);
    Icon_Unshade(icndiag_window,icndiag_FILLED);
    Icon_Unshade(icndiag_window,icndiag_NEEDSHELP);
    Icon_Unshade(icndiag_window,icndiag_BTNTYPE);
    Icon_Unshade(icndiag_window,icndiag_BTNMENU);
    Icon_Unshade(icndiag_window,icndiag_ESG);
    Icon_Unshade(icndiag_window,icndiag_DECESG);
    Icon_Unshade(icndiag_window,icndiag_INCESG);
    Icon_Unshade(icndiag_window,icndiag_FORECOL);
    Icon_Unshade(icndiag_window,icndiag_DECFORECOL);
    Icon_Unshade(icndiag_window,icndiag_INCFORECOL);
    Icon_Unshade(icndiag_window,icndiag_FORECOLMENU);
    Icon_Unshade(icndiag_window,icndiag_BACKCOL);
    Icon_Unshade(icndiag_window,icndiag_DECBACKCOL);
    Icon_Unshade(icndiag_window,icndiag_INCBACKCOL);
    Icon_Unshade(icndiag_window,icndiag_BACKCOLMENU);
  }

  return TRUE;
}

BOOL icndiag_keypress(event_pollblock *event,void *reference)
{
  icndiag_affect(event,reference);
  if (event->data.key.code == 13)
  {
    event_pollblock dummyev;

    dummyev.type = event_CLICK;
    dummyev.data.mouse.window = icndiag_window;
    dummyev.data.mouse.icon = icndiag_UPDATE;
    dummyev.data.mouse.button.value = button_SELECT;
    return icndiag_update(&dummyev,reference);
  }
  else if (event->data.key.code == 27)
  {
    icndiag_close();
    return TRUE;
  }
  Wimp_ProcessKey(event->data.key.code);

  return TRUE;
}

static void icndiag_readdbox(icon_block *iblock)
{
  int esg;

  iblock->flags.data.text         = Icon_GetSelect(icndiag_window, icndiag_TEXT);
  iblock->flags.data.sprite       = Icon_GetSelect(icndiag_window, icndiag_SPRITE);
  iblock->flags.data.indirected   = Icon_GetSelect(icndiag_window, icndiag_INDIRECTED);
  iblock->flags.data.buttontype   = diagutils_readbtype(icndiag_window,  icndiag_BTNTYPE);
  iblock->flags.data.text         = Icon_GetSelect(icndiag_window, icndiag_TEXT);
  iblock->flags.data.border       = Icon_GetSelect(icndiag_window, icndiag_BORDER);
  iblock->flags.data.hcentre      = Icon_GetSelect(icndiag_window, icndiag_HCENTRE);
  iblock->flags.data.vcentre      = Icon_GetSelect(icndiag_window, icndiag_VCENTRE);
  iblock->flags.data.filled       = Icon_GetSelect(icndiag_window, icndiag_FILLED);
  iblock->flags.data.needshelp    = Icon_GetSelect(icndiag_window, icndiag_NEEDSHELP);
  iblock->flags.data.rightjustify = Icon_GetSelect(icndiag_window, icndiag_RALIGN);
  iblock->flags.data.allowadjust  = Icon_GetSelect(icndiag_window, icndiag_ALLOWADJUST);
  iblock->flags.data.halfsize     = Icon_GetSelect(icndiag_window, icndiag_HALFSIZE);
  iblock->flags.data.selected     = Icon_GetSelect(icndiag_window, icndiag_SELECTED);
  iblock->flags.data.shaded       = Icon_GetSelect(icndiag_window, icndiag_SHADED);
  iblock->flags.data.deleted      = Icon_GetSelect(icndiag_window, icndiag_DELETED);
  iblock->flags.data.numeric      = Icon_GetSelect(icndiag_window, icndiag_NUMERIC);
  esg = Icon_GetInteger(icndiag_window,icndiag_ESG);
  if (esg < 0)
    esg = 0;
  else if (esg > 15)
    esg = 15;
  iblock->flags.data.esg = esg;
  iblock->flags.data.font = Icon_GetSelect(icndiag_window,icndiag_ANTIALIASED);
  if (iblock->flags.data.font)
  {
    template_fontinfo fontinfo;

    Icon_GetText(icndiag_window,icndiag_FONTNAME,fontinfo.name);
    fontinfo.size.y = (int) (Icon_GetDouble(icndiag_window,
    		       	     		    icndiag_FONTHEIGHT) * 16.0);
    fontinfo.size.x = (int) ((double) fontinfo.size.y *
    		       	     Icon_GetDouble(icndiag_window,
    		       	     		    icndiag_FONTASPECT) / 100.0);
    iblock->flags.font.handle = tempfont_findfont(icndiag_winentry->browser, &fontinfo);
  }
  else
  {
    iblock->flags.data.foreground = diagutils_readcolour(icndiag_window,
    				   			icndiag_FORECOL);
    iblock->flags.data.background = diagutils_readcolour(icndiag_window,
    				   			icndiag_BACKCOL);
  }

  if (iblock->flags.data.indirected)
  {
    if (iblock->flags.data.text || iblock->flags.data.sprite)
    {
      iblock->data.indirecttext.bufflen = Icon_GetInteger(icndiag_window,
      				       	 		 icndiag_MAXLEN);
      if (icndiag_icon == -1)
        iblock->data.indirecttext.buffer = (char *)
          icnedit_addindirectstring(icndiag_winentry,
          			    Icon_GetTextPtr(icndiag_window,
          			    		    icndiag_STRING));
      else
        iblock->data.indirecttext.buffer = (char *)
          Icon_GetTextPtr(icndiag_window,icndiag_STRING);
    }
    if (iblock->flags.data.text)
    {
      if (icndiag_icon == -1)
      {
        char *valid = Icon_GetTextPtr(icndiag_window,icndiag_VALID);

        if (strlencr(valid))
          iblock->data.indirecttext.validstring = (char *)
            icnedit_addindirectstring(icndiag_winentry,valid);
        else
          iblock->data.indirecttext.validstring = (char *) -1;
      }
      else
        iblock->data.indirecttext.validstring = (char *)
          Icon_GetTextPtr(icndiag_window,icndiag_VALID);
    }
    else if (iblock->flags.data.sprite)
    {
      iblock->data.indirectsprite.spritearea = (void *) user_sprites;
    }
  }
  else
    strncpycr(iblock->data.text,Icon_GetTextPtr(icndiag_window,icndiag_STRING),
    	      12);
}

BOOL icndiag_update(event_pollblock *event,void *reference)
{
  icon_block iblock;

  Log(log_DEBUG, "icndiag_update");

  if (!event->data.mouse.button.data.adjust &&
      !event->data.mouse.button.data.select)
    return FALSE;

  icndiag_affect(event,reference);

  if (icndiag_icon == -1)
  {
    /* Have to unprocess logical data here */
    icon_flags *flags = &icndiag_winentry->window->window.titleflags;

    if (flags->data.font)
      tempfont_losefont(icndiag_winentry->browser,flags->font.handle);
    icnedit_deleteindirected(icndiag_winentry,-1);
  }

  icndiag_readdbox(&iblock);

  if (icndiag_icon == -1)
  {
    icndiag_winentry->window->window.title = iblock.data;
    icndiag_winentry->window->window.titleflags = iblock.flags;
    viewer_reopen(icndiag_winentry,TRUE);
  }
  else
  {
    iblock.workarearect =
      icndiag_winentry->window->icon[icndiag_icon].workarearect;
    icnedit_changedata(icndiag_winentry,icndiag_icon,&iblock);
  }

  /* Resize if necessary */
  if (((event->data.mouse.icon == icndiag_UPDATE && choices->safe_icons == FALSE) ||
      (event->data.mouse.icon == icndiag_ALTUPDATE && choices->safe_icons == TRUE)) && icndiag_icon != -1)
  {
    wimp_point minsize;
    BOOL resize = FALSE;

    icnedit_minsize(icndiag_winentry,icndiag_icon,&minsize);
    if (iblock.workarearect.max.x - iblock.workarearect.min.x < minsize.x)
    {
      Log(log_DEBUG, " Too small maxx:%d minx:%d minsize:%d", iblock.workarearect.max.x,iblock.workarearect.min.x,minsize.x);
      resize = TRUE;
      iblock.workarearect.max.x = iblock.workarearect.min.x + minsize.x;
    }
    if (iblock.workarearect.max.y - iblock.workarearect.min.y < minsize.y)
    {
      resize = TRUE;
      iblock.workarearect.max.y = iblock.workarearect.min.y + minsize.y;
    }
    if (resize)
    {
      Log(log_DEBUG, " Resizerin'");
      icnedit_moveicon(icndiag_winentry,icndiag_icon,&iblock.workarearect);
    }
  }
  if (event->data.mouse.button.data.select)
    icndiag_close();

  browser_settitle(icndiag_winentry->browser,0,TRUE);

  return TRUE;
}

BOOL icndiag_fontmenu(event_pollblock *event,void *reference)
{
  icndiag_menu = fontpick_makemenu(Icon_GetTextPtr(icndiag_window,
  	   	  		    		   icndiag_FONTNAME));
  if (!icndiag_menu)
    return TRUE;

  Menu_PopUpAuto(icndiag_menu);
  Event_Claim(event_MENU,event_ANY,event_ANY,icndiag_choosefont,0);
  EventMsg_Claim(message_MENUSDELETED,event_ANY,icndiag_releasefontmenu,0);
  help_claim_menu("Font");

  return TRUE;
}

BOOL icndiag_choosefont(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  char fontid[256];

  Wimp_GetPointerInfo(&ptrinfo);

  fontpick_findselection(event->data.selection,fontid);
  icndiag_menu = fontpick_makemenu(fontid);
  fontpick_findname(fontid,
  		    (char *) Icon_GetTextPtr(icndiag_window,icndiag_FONTNAME));
  Icon_ForceRedraw(icndiag_window,icndiag_FONTNAME);

  if (ptrinfo.button.data.adjust)
    WinEd_CreateMenu(icndiag_menu,menu_currentpos.x + 64,menu_currentpos.y);
  else
    icndiag_releasefontmenu(event,reference);
  return TRUE;
}

BOOL icndiag_releasefontmenu(event_pollblock *event,void *reference)
{
  fontpick_freemenu();
  Event_Release(event_MENU,event_ANY,event_ANY,icndiag_choosefont,0);
  EventMsg_Release(message_MENUSDELETED,event_ANY,icndiag_releasefontmenu);
  help_release_menu();
  return TRUE;
}

BOOL icndiag_fontvalid(event_pollblock *event,void *reference)
{
  char newvalid[64];
  char *oldvalid;
  int findex;
  char *afterf;

  if (!event->data.mouse.button.data.adjust &&
      !event->data.mouse.button.data.select)
    return FALSE;

  Icon_Select(icndiag_window,icndiag_INDIRECTED);
  sprintf(newvalid,"F%x%x;",
  	diagutils_readcolour(icndiag_window,icndiag_BACKCOL),
  	diagutils_readcolour(icndiag_window,icndiag_FORECOL));
  oldvalid = (char *) Icon_GetTextPtr(icndiag_window,icndiag_VALID);
  findex = Validation_ScanString(oldvalid,'F');
  if (!findex && strlen(oldvalid))
    strcatcr(newvalid,oldvalid);
  else if (findex == 1)
  {
    afterf = strchr(oldvalid,';');
    if (afterf)
      strcatcr(newvalid,afterf + 1);
  }
  else
  {
    /* Copy bit before F */
    strncat(newvalid,oldvalid,findex - 2);
    /* And after */
    afterf = strchr(oldvalid + findex,';');
    if (afterf)
      strcatcr(newvalid,afterf);
  }
  if (newvalid[strlen(newvalid) - 1] == ';')
    newvalid[strlen(newvalid) - 1] = 0;

  Icon_SetText(icndiag_window,icndiag_VALID,newvalid);

  return icndiag_affect(event,reference);
}

BOOL icndiag_minmax(event_pollblock *event,void *reference)
{
  Icon_SetText(icndiag_window,icndiag_MAXLEN,"0");
  return icndiag_affect(event,reference);
}

BOOL icndiag_help(event_pollblock *event,void *reference)
{
  char tag[20], *suffix = "";

  /* Add in a suffix to the window name, to give ICC_S for the two update
   * action buttons if we're in "safe icons" mode, ICC_T for the main
   * update button if we're editing a window title, and ICC otherwise. */

  if ((icndiag_icon == -1) && (event->data.message.data.helprequest.where.icon == icndiag_UPDATE))
    suffix = "_T";
  else if (choices->safe_icons && (event->data.message.data.helprequest.where.icon == icndiag_UPDATE ||
      event->data.message.data.helprequest.where.icon == icndiag_ALTUPDATE))
    suffix = "_S";

  snprintf(tag, 20, "%s%s", (char *) reference, suffix);
  tag[20 - 1] = '\0';

  return help_on_window(event, tag);
}

void icndiag_close()
{
  return_caret(icndiag_window, icndiag_winentry->handle);
  Wimp_CloseWindow(icndiag_window);
  if (choices->editpanes)
    Wimp_CloseWindow(icndiag_pane);
}

void icndiag_responder(choices_str *old, choices_str *new_ch)
{
  window_state wstate;

  Wimp_GetWindowState(icndiag_window, &wstate);
  if (!wstate.flags.data.open) return;

  if (old->editpanes && !new_ch->editpanes)
    Wimp_CloseWindow(icndiag_pane);
  else if (!old->editpanes && new_ch->editpanes)
  {
    event_pollblock dummyev;

    dummyev.data.openblock = wstate.openblock;
    icndiag_OpenWindow(&dummyev, 0);
  }
}
