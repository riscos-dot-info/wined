/* viewer.c */

#include "common.h"

#include "DeskLib:Coord.h"
#include "DeskLib:Font.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:KeyCodes.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Wimp.h"

#include "alndiag.h"
#include "bordiag.h"
#include "browsicons.h"
#include "choices.h"
#include "coorddiag.h"
#include "datatrans.h"
#include "diagutils.h"
#include "drag.h"
#include "icndiag.h"
#include "icnedit.h"
#include "monitor.h"
#include "multiicon.h"
#include "picker.h"
#include "round.h"
#include "rszdiag.h"
#include "spcdiag.h"
#include "tempfont.h"
#include "title.h"
#include "usersprt.h"
#include "viewcom.h"
#include "viewer.h"
#include "viewtools.h"
#include "visarea.h"
#include "wadiag.h"
#include "windiag.h"

#define windowcol_TITLEFORE 0 /* AR Added by me because I can't be bothered to work out where it should come from in DeskLib */

#define inkey_ESC -113

/* Size of cross hatch grid */
#define HATCH_SIZE 48

/* Minimum size for tools around windows */
#define MIN_TOOLS 128
#define MIN_CLOSE 64

/* Whether an icon can be selected with menu click */
BOOL viewer_selection_withmenu = TRUE;

/* Modify window flags for editable window */
void viewer_modifyflags(window_block *window);

/* Create a window from a browser_winblock */
BOOL viewer_createwindow(browser_winentry *winentry,
     			 browser_winblock *winblock);

/* mallocs a winblock the right size, and makes it suitable for opening */
BOOL viewer_makewinblock(browser_winentry *winentry,
     			 BOOL wineditable,BOOL icneditable,
     			 browser_winblock **winblock);

/* Claim events for editable window */
void viewer_claimeditevents(browser_winentry *winentry);

/* Redraw, including fake borders for R6 icons nad hatch */
BOOL viewer_redraw(event_pollblock *event,void *reference);

/* Close window in response to close event */
BOOL viewer_closeevent(event_pollblock *event,void *reference);

/* Round window during open events */
BOOL viewer_openevent(event_pollblock *event,void *reference);

/* Handle mouse events */
BOOL viewer_click(event_pollblock *event,void *reference);

/* Hot keys */
BOOL viewer_hotkey(event_pollblock *event,void *reference);

/* Mouse click handler */
BOOL preview_click(event_pollblock *event,void *reference);

/* Set renumber fields in menus */
void viewer_numbermenu(int value);

/* Set 'Icon nnnn' field in parent menu */
void viewer_iconmenufield(int value);

/* Open main menu */
void viewer_openmenu(event_pollblock *event,browser_winentry *winentry);

/* Prepare main menu depending on selection state */
void viewer_preparemenu(browser_winentry *winentry);

/* Menu selection handler */
BOOL viewer_menuselect(event_pollblock *event,void *reference);

/* Menu selection handler */
BOOL preview_menuselect(event_pollblock *event,void *reference);

/* Release menu handlers */
BOOL viewer_releasemenu(event_pollblock *event,void *reference);

/* Release menu handlers */
BOOL preview_releasemenu(event_pollblock *event,void *reference);

/* Handle completion of icon resize drag */
BOOL viewer_resizedrag(event_pollblock *event,void *reference);

/* Completion of single icon move drag */
BOOL viewer_moveonedrag(event_pollblock *event,void *reference);

/* Completion of multiple icon move drag */
BOOL viewer_movemanydrag(event_pollblock *event,void *reference);

/* Scroll window during drag */
BOOL viewer_scrolldrag(event_pollblock *event,window_handle window);

/* Respond to iconise events */
BOOL viewer_iconise(event_pollblock *event,browser_winentry *winentry);

/* Return scroll bars to position before drag */
void viewer_resetscrollbars(void);

/* Prepares menu tree, opens it and claims handlers */
static void viewer_openmenu_at(browser_winentry *winentry, int x, int y);

/* Align a window_openblock */
/*void viewer_roundopenblock(window_openblock *openblock);*/
#define viewer_roundopenblock(o)

/* Find new position (newrect) of icon based on displacement of where drag
   started (startrect) to where it ended (dragrect) */
void viewer_newbounds(browser_winentry *winentry,int icon,
     		      wimp_rect *newrect,wimp_rect *dragrect,
     		      wimp_rect *startrect);
/* Copy/move selected icons:
   source    	      :  source winentry
   dest		      :	 dest winentry
   oldrect	      :	 original bounds of selection
   newrect	      :	 bounds of area to copy/move selection to
   shift	      :	 whether shift was pressed at start of drag
 */
void viewer_copyselection(browser_winentry *source,browser_winentry *dest,
     			  wimp_rect *oldrect,wimp_rect *newrect,
     			  BOOL shift);

/* Swap two icons */
void viewer_swapicons(browser_winentry *winentry,int icon,int value);

/* Make menus */
void viewer_createmenus(void);

/* Put invisible caret in browser */
void viewer_claimcaret(browser_winentry *winentry);

/* Allows handlers as references without irksome cast warnings.
   Inefficiency's less evil than warnings ;-) */
typedef union {
  event_handler handler;
  void *reference;
} viewer_cancelref;

/* Cancel drags when Esc pressed */
static void viewer_canceldrag(viewer_cancelref);
static BOOL viewer_canceldraghandler(event_pollblock *,void *);

/* Change from preview to editable or vice versa */
static void viewer_view(browser_winentry *winentry,BOOL editable)
{
  if (winentry->status)
    viewer_close(winentry);
  viewer_open(winentry,editable);
}

/* Data needed when drag is completed */
static struct {
  browser_winentry *winentry;
  icon_handle icon;   /* Physical icon under pointer at start of drag */
  wimp_point scroll;  /* Scroll offsets before dragging */
  BOOL shift;         /* Whether shift pressed at start of drag */
  wimp_rect startbox; /* Work area coords of start of drag */
} viewer_dragref;

/* Preview menu */
static menu_ptr preview_menu;
typedef enum {
  preview_CLOSE,
  preview_SAVEPOS,
  preview_SAVESTATE,
  preview_EDIT
} preview_menuentries;

/* Main viewer menu, submenu, copy menu */
static menu_ptr viewer_parentmenu = 0;
static menu_ptr viewer_iconmenu = 0;
static menu_ptr viewer_copymenu = 0;

static BOOL viewer_menuopen = FALSE;

typedef enum {
  viewer_ICON,
  viewer_SELALL,
  viewer_CLEARSEL,
  viewer_WORKAREA,
  viewer_VISAREA,
  viewer_EDITWIN,
  viewer_EDITTITLE,
  viewer_CLOSE,
  viewer_PREVIEW,

  viewer_DELETE = 0,
  viewer_RENUMBER,
  viewer_COPY,
  viewer_EDITICON,
  viewer_ALIGN,
  viewer_SPACEOUT,
  viewer_RESIZE,
  viewer_COORDS,
  viewer_BORDER,

  viewer_DOWN = 0,
  viewer_RIGHT,
  viewer_LEFT,
  viewer_UP
} viewer_menuentries;

/* 'Virtual icon' for drawing R6 borders */
static icon_block viewer_dummyborder;
static char viewer_dbvalid[] = "R2";

/* Used to indicate whether a transient dbox popping up is causing a menu to
   close and hence dbox has to ignore the next message_MENUSDELETED */
/*extern BOOL menu_destroy;*/

/* Renumber mini-dbox */
static window_handle renum_dbox;
static BOOL renum_open = FALSE;

void viewcom_quickrenum(browser_winentry *winentry)
{
  int icon;
  int value;
  int selections;
  icon_handle *table;

  /* Find number to start renumbering from */
  value = Icon_GetInteger(renum_dbox, 2);
  selections = count_selections(winentry->handle);
  if (selections == 0)
    return;
  if (selections + value > winentry->window->window.numicons)
  {
    MsgTrans_Report(messages,"IconRange",FALSE);
    return;
  }

  /* Make table to hold handles of selected icons */
  table = malloc((selections + 1) * sizeof(icon_handle));
  if (!table)
  {
    MsgTrans_Report(0,"NoStore",FALSE);
    return;
  }

  /* Fill in table */
  Wimp_WhichIcon(winentry->handle,table,icon_SELECTED,icon_SELECTED);

  /* Renumber all icons in table. If changing to higher numbers, have to go
     in reverse order */
  if (value > table[0])
  {
    value += selections - 1;
    for (icon = selections - 1;icon >= 0;icon--)
    {
      if (value != table[icon])
        viewer_swapicons(winentry,table[icon],value);
      value--;
    }
    value += selections + 1;
  }
  else
  {
    for (icon = 0;icon < selections;icon++)
    {
      if (value != table[icon])
        viewer_swapicons(winentry,table[icon],value);
      value++;
    }
  }

  free(table);

  /* Update menus with value */
  if (value >= winentry->window->window.numicons)
    value = winentry->window->window.numicons - 1;
  viewer_numbermenu(value);
  viewer_iconmenufield(value);
}

static BOOL renum_clicked(event_pollblock *event,void *reference)
{
  if (event->type == event_KEY && event->data.key.code != 13)
  {
    Wimp_ProcessKey(event->data.key.code);
    return TRUE;
  }
  if (event->type == event_CLICK && event->data.mouse.button.data.menu)
    return FALSE;
  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu(0, -1, -1);

  viewcom_quickrenum(reference);

  return TRUE;
}

static void release_renum()
{
  EventMsg_ReleaseWindow(renum_dbox);
  Event_ReleaseWindow(renum_dbox);
  help_release_window(renum_dbox);
  renum_open = FALSE;
}

static BOOL release_renum_msg(event_pollblock *e, void *r)
{
  release_renum();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_renum_msg);
  return TRUE;
}

void viewcom_renum(BOOL submenu, int x, int y,browser_winentry *reference)
{
  if (renum_open)
    release_renum();
  if (!submenu)
  {
    mouse_block ptrinfo;
    Wimp_GetPointerInfo(&ptrinfo);
    x = ptrinfo.pos.x - 64;
    y = ptrinfo.pos.y + 64;
  }
  if (submenu)
    Wimp_CreateSubMenu((menu_ptr) renum_dbox, x, y);
  else
  {
    WinEd_CreateMenu((menu_ptr) renum_dbox, x, y);
    EventMsg_Claim(message_MENUSDELETED, event_ANY, release_renum_msg, 0);
  }
  Event_Claim(event_CLICK,renum_dbox,0,renum_clicked,reference);
  Event_Claim(event_KEY,renum_dbox,2,renum_clicked,reference);
  Event_Claim(event_CLICK,renum_dbox,1,kill_menus,reference);
  Event_Claim(event_OPEN,renum_dbox,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,renum_dbox,event_ANY,kill_menus,0);
  help_claim_window(renum_dbox, "NUM");
  renum_open = TRUE;
}

void viewer_createmenus()
{
  char menutext[256], title[64];

  /* Main menu and subs */
  if (viewer_parentmenu) Menu_FullDispose(viewer_parentmenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"ViewMenK",menutext,256);
  else
    MsgTrans_Lookup(messages,"ViewMen",menutext,256);

  viewer_parentmenu = Menu_New(APPNAME,menutext);
  if (!viewer_parentmenu) MsgTrans_Report(messages,"NoMenu",TRUE);
  Menu_SetOpenShaded(viewer_parentmenu, viewer_ICON);

  /* "Icon" menu */
  if (viewer_iconmenu) Menu_FullDispose(viewer_iconmenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"IconMenK",menutext,256);
  else
    MsgTrans_Lookup(messages,"IconMen",menutext,256);
  viewer_iconmenu = Menu_New("",menutext);
  if (!viewer_iconmenu) MsgTrans_Report(messages,"NoMenu",TRUE);

  /* Copy menu */
  if (viewer_copymenu)
    Menu_FullDispose(viewer_copymenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"CopyMenK",menutext,256);
  else
    MsgTrans_Lookup(messages,"CopyMen",menutext,256);
  MsgTrans_Lookup(messages,"CopyName",title,64);
  viewer_copymenu = Menu_New(title,menutext);
  if (!viewer_copymenu) MsgTrans_Report(messages,"NoMenu",TRUE);

  /* Attachments */
  Menu_AddSubMenu(viewer_parentmenu, viewer_ICON, viewer_iconmenu);
  Menu_AddSubMenu(viewer_iconmenu,   viewer_COPY, viewer_copymenu);
}

void viewer_init()
{
  window_block *templat;

  char menutitle[32],menutext[256];

  /*MsgTrans_Lookup(messages,"Number",menutitle,32);
  viewer_renummenu = Menu_New(menutitle,"1234");
  if (!viewer_renummenu)
    MsgTrans_Report(messages,"NoMenu",TRUE);*/
  /* Make renumber writable */
  /*Menu_MakeWritable(viewer_renummenu,0,
  		    viewer_renumbuffer,5,viewer_renumvalid);*/
  /* Preview menu */
  MsgTrans_Lookup(messages,"Preview",menutitle,32);
  MsgTrans_Lookup(messages,"PrevMen",menutext,256);
  preview_menu = Menu_New(menutitle,menutext);
  if (!preview_menu)
    MsgTrans_Report(messages,"NoMenu",TRUE);

  viewer_createmenus();

  /* Dialog boxes */
  windiag_init();
  diagutils_init();
  wadiag_init();
  visarea_init();
  icndiag_init();
  multiicon_init();
  alndiag_init();
  spcdiag_init();
  rszdiag_init();
  coorddiag_init();
  bordiag_init();
  viewtools_init();
  templat = templates_load("Renumber",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&renum_dbox));
  free(templat);

  /* Dummy border icon */
  viewer_dummyborder.flags.value = icon_TEXT | icon_BORDER | icon_FILLED |
  				   icon_INDIRECTED;
  viewer_dummyborder.flags.data.background = colour_CREAM;
  viewer_dummyborder.data.indirecttext.buffer = viewer_dbvalid + 2;
  viewer_dummyborder.data.indirecttext.validstring = viewer_dbvalid;
  viewer_dummyborder.data.indirecttext.bufflen = 0;
}

void viewer_closechildren(browser_winentry *winentry)
{
  if (windiag_winentry == winentry)
    windiag_close();
  if (wadiag_winentry == winentry)
    wadiag_close();
  if (icndiag_winentry == winentry)
    icndiag_close();
  if (alndiag_winentry == winentry)
    alndiag_close();
  if (spcdiag_winentry == winentry)
    spcdiag_close();
  if (rszdiag_winentry == winentry)
    rszdiag_close();
  if (coorddiag_winentry == winentry)
    coorddiag_close();
  if (bordiag_winentry == winentry)
    bordiag_close();
  if (visarea_winentry == winentry)
    visarea_close();
  if (multiicon_winentry == winentry)
    multiicon_close();
}

void viewer_close(browser_winentry *winentry)
{
  window_info winfo;
  icon_handle icon;

  if (!winentry->status)
    return;

  /* Release handlers */
  EventMsg_ReleaseWindow(winentry->handle);
  Event_ReleaseWindow(winentry->handle);

  /* Get rid of all indirected data */
  /* Title */
  Window_GetInfo3(winentry->handle,&winfo);
  icnedit_freeindirected(&winfo.block.titleflags,&winfo.block.title);
  /* Icons */
  for (icon = 0;icon < winfo.block.numicons;icon++)
  {
    icon_block istate;

    Error_Check(Wimp_GetIconState(winentry->handle,icon,&istate));
    if (!istate.flags.data.deleted)
      icnedit_freeindirected(&istate.flags,&istate.data);
  }

  /* Lose fonts */
  if (winentry->fontarray)
  {
    Font_LoseAllFonts(winentry->fontarray);
    flex_free((flex_ptr) &winentry->fontarray);
    winentry->fontarray = 0;
  }

  if (selection_viewer == winentry)
  {
    selection_viewer = NULL;
    selection_browser = NULL;
    viewer_selection_withmenu = TRUE;
  }

  /* Close picker/monitor */
  if (winentry != &picker_winentry)
  {
    if (choices->monitor)
      monitor_activedec();
    if (choices->picker && winentry->status == status_EDITING)
      picker_activedec();
    if (choices->hotkeys)
      return_caret(winentry->handle, winentry->browser->window);
  }

  /* Delete window */
  Error_Check(Wimp_DeleteWindow(winentry->handle));
  /* Clear up winentry */
  winentry->status = status_CLOSED;
  /* Close any editing windows */
  viewer_closechildren(winentry);
  /* Close tool pane */
  if (choices->viewtools)
    viewtools_deletepane(winentry);
}

void viewer_modifyflags(window_block *window)
{
  window->flags.data.moveable = 1;
  window->flags.data.autoredraw = 0;
  window->flags.data.nobounds = 0;
  window->flags.data.scrollrq = 0;
  window->flags.data.scrollrqdebounced = 0;
  window->flags.data.hotkeys = 0;
  window->flags.data.keeponscreen = 0;
  window->flags.data.ignoreright = 0;
  window->flags.data.ignorebottom = 0;
  if (choices->furniture)
  {
    window->flags.data.titlebar = 1;
    if (window->screenrect.max.x - window->screenrect.min.x > MIN_CLOSE)
      window->flags.data.closeicon = 1;
    if (window->screenrect.max.x - window->screenrect.min.x > MIN_TOOLS)
    {
      window->flags.data.backicon = 1;
      window->flags.data.toggleicon = 1;
      window->flags.data.hscroll = 1;
      window->flags.data.adjusticon = 1;
    }
    if (window->screenrect.max.y - window->screenrect.min.y > MIN_TOOLS)
    {
      window->flags.data.adjusticon = 1;
      window->flags.data.vscroll = 1;
    }
    window->flags.data.newflags = 1;
    if (window->colours.vals.colours[windowcol_TITLEFORE] == 0xff)
      window->colours.vals.colours[windowcol_TITLEFORE] = colour_BLACK;
  }
  window->workflags.data.buttontype = iconbtype_DOUBLECLICKDRAG;
}

void viewer_claimeditevents(browser_winentry *winentry)
{
  Event_Claim(event_REDRAW,winentry->handle,event_ANY, viewer_redraw,     winentry);
  Event_Claim(event_OPEN,  winentry->handle,event_ANY, viewer_openevent,  winentry);
  Event_Claim(event_CLOSE, winentry->handle,event_ANY, viewer_closeevent, winentry);
  Event_Claim(event_CLICK, winentry->handle,event_ANY, viewer_click,      winentry);
  Event_Claim(event_KEY,   winentry->handle,event_ANY, viewer_hotkey,     winentry);
  EventMsg_Claim(message_WINDOWINFO,winentry->handle, (event_handler) viewer_iconise,winentry);
  Event_Claim(event_PTRLEAVE,winentry->handle,event_ANY, monitor_deactivate, winentry);
  Event_Claim(event_PTRENTER,winentry->handle,event_ANY, monitor_activate,   winentry);
  if (winentry != &picker_winentry)
    help_claim_window(winentry->handle,"View");
  else
    help_claim_window(winentry->handle,"Pick");
}

BOOL viewer_createwindow(browser_winentry *winentry,
     			 browser_winblock *winblock)
{
  int icon;

  if (Error_Check(Wimp_CreateWindow(&winblock->window,&winentry->handle)))
  {
    /* Get rid of data/fonts we've just created */
    /* Title */
    icnedit_freeindirected(&winblock->window.titleflags,
    			  &winblock->window.title);
    /* Icons */
    for (icon = 0;icon < winblock->window.numicons;icon++)
    {
      if (!winblock->icon[icon].flags.data.deleted)
        icnedit_freeindirected(&winblock->icon[icon].flags,
        		       &winblock->icon[icon].data);
    }

    /* Lose fonts */
    if (winentry->fontarray)
    {
      Font_LoseAllFonts(winentry->fontarray);
      flex_free((flex_ptr) &winentry->fontarray);
      winentry->fontarray = 0;
    }
    return FALSE;
  }

  return TRUE;
}

BOOL viewer_makewinblock(browser_winentry *winentry,
     			 BOOL wineditable,BOOL icneditable,
     			 browser_winblock **winblock)
{
  BOOL sofar = TRUE;
  int icon;

  /* Create replica of window info */
  *winblock = malloc(sizeof(window_block) +
  	     	     winentry->window->window.numicons * sizeof(icon_block));

  if (!*winblock)
  {
    MsgTrans_Report(messages,"ViewMem",FALSE);
    return FALSE;
  }
  memcpy(*winblock,winentry->window,sizeof(window_block) +
  	 winentry->window->window.numicons * sizeof(icon_block));

  /* Make sure pane, furniture and back flags aren't set */
  (*winblock)->window.flags.data.pane = 0;
  (*winblock)->window.flags.data.backwindow = 0;
  (*winblock)->window.flags.data.childfurniture = 0;

  if (wineditable)
    viewer_modifyflags(&(*winblock)->window);
  else
  {
    if ((*winblock)->window.workflags.data.buttontype ==
          iconbtype_WRITECLICKDRAG ||
        (*winblock)->window.workflags.data.buttontype == iconbtype_WRITABLE)
      (*winblock)->window.workflags.data.buttontype = iconbtype_WRITECLICKDRAG;
    else
      (*winblock)->window.workflags.data.buttontype = iconbtype_CLICKDRAG;
  }

  (*winblock)->window.behind = -1;
  /*if ((*winblock)->window.spritearea != (unsigned int *) 1)*/
  (*winblock)->window.spritearea = (unsigned int *) user_sprites;

  /* Process icons */
  /* Title */
  if (!icnedit_processicon(winentry,
      			   &(*winblock)->window.titleflags,
      			   &(*winblock)->window.title,0,wineditable))
  {
    sofar = FALSE;
    MsgTrans_Report(messages,"IconMem",FALSE);
  }
  /* Icons */
  for (icon = 0;icon < (*winblock)->window.numicons;icon++)
  {
    if (!icnedit_processicon(winentry,&(*winblock)->icon[icon].flags,
    			    &(*winblock)->icon[icon].data,
    			    &(*winblock)->icon[icon].workarearect,
    			    icneditable))
    {
      if (sofar)
      {
        MsgTrans_Report(messages,"IconMem",FALSE);
        sofar = FALSE;
      }
    }
  }
  return TRUE;
}

static void viewer_claimpreviewevents(browser_winentry *winentry)
{
  Event_Claim(event_REDRAW, winentry->handle, event_ANY, Handler_NullRedraw, 0);
  Event_Claim(event_OPEN,   winentry->handle, event_ANY, viewer_openevent,   winentry);
  Event_Claim(event_CLOSE,  winentry->handle, event_ANY, viewer_closeevent,  winentry);
  Event_Claim(event_CLICK,  winentry->handle, event_ANY, preview_click,      winentry);
  EventMsg_Claim(message_WINDOWINFO,winentry->handle, (event_handler) viewer_iconise, winentry);
  Event_Claim(event_PTRLEAVE, winentry->handle, event_ANY, monitor_deactivate, winentry);
  Event_Claim(event_PTRENTER, winentry->handle, event_ANY, monitor_activate,   winentry);
  help_claim_window(winentry->handle,"Prev");
}

void viewer_open(browser_winentry *winentry,BOOL editable)
{
  browser_winblock *winblock;
  window_state wstate;

  /* Make winblock */
  if (!viewer_makewinblock(winentry,editable,editable,&winblock))
    return;

  /* Create window */
  if (!viewer_createwindow(winentry,winblock))
  {
    free(winblock);
    return;
  }

  winentry->pane = 0;


  if (editable)
  {
    winentry->status = status_EDITING;
    viewer_claimeditevents(winentry);

    if (choices->viewtools)
      viewtools_newpane(winentry);
  }
  else
  {
    winentry->status = status_PREVIEW;
    viewer_claimpreviewevents(winentry);
  }

  /* Open monitor & picker */
  if (choices->monitor)
    monitor_activeinc();
  if (choices->picker && editable)
    picker_activeinc();

  /* Display window */
  Wimp_GetWindowState(winentry->handle,&wstate);
  wstate.openblock.behind = -1;
  if (editable && choices->viewtools)
    viewtools_open(&wstate.openblock,winentry->pane);
  else
    Wimp_OpenWindow(&wstate.openblock);
  if (editable && choices->hotkeys && winentry != &picker_winentry)
    viewer_claimcaret(winentry);

  /* Set menu number entries */
  viewer_numbermenu(0);

  free(winblock);
}

static void viewer_deregisterwindow(browser_winentry *winentry)
{
  /* Delete window and lose handlers */
  Wimp_CloseWindow(winentry->handle);
  Wimp_DeleteWindow(winentry->handle);
  EventMsg_ReleaseWindow(winentry->handle);
  Event_ReleaseWindow(winentry->handle);
}

/* Clears old window and prepares to reopen it */
static window_info *viewer_preparewinblock(browser_winentry *winentry,
	window_handle *behind, BOOL *monitor_was_active, BOOL newtitle)
{
  window_state wstate;
  window_info *winfo = malloc(sizeof(window_info) +
  	winentry->window->window.numicons * sizeof(icon_block));
  if (!winfo)
  {
    viewer_deregisterwindow(winentry);
    MsgTrans_Report(messages,"ViewMem",FALSE);
    return NULL;
  }
  winfo->window = winentry->handle;
  if (Error_Check(Wimp_GetWindowInfo(winfo)))
  {
    viewer_deregisterwindow(winentry);
    free (winfo);
    return NULL;
  }
  Error_Check(Wimp_GetWindowState(winentry->handle,&wstate));
  *behind = wstate.openblock.behind;
  /* Deactivate monitor if necessary */
  if (monitor_isactive && monitor_winentry == winentry)
  {
    *monitor_was_active = TRUE;
    monitor_deactivate(NULL,winentry);
  }
  else
    *monitor_was_active = FALSE;
  /* Set up new window_block in winfo */
  if (newtitle)
  {
    icnedit_unprocessicon(winentry,&winfo->block.titleflags,
    			  &winfo->block.title);
    winfo->block = winentry->window->window;
    icnedit_processicon(winentry,&winfo->block.titleflags,
    			&winfo->block.title,0,TRUE);
  }
  else /* Remember original title */
  {
    icon_flags titleflags = winfo->block.titleflags;
    icon_data titledata = winfo->block.title;
    winfo->block = winentry->window->window;
    winfo->block.titleflags = titleflags;
    winfo->block.title = titledata;
  }
  viewer_modifyflags(&winfo->block);
  winfo->block.flags.data.pane = 0;
  winfo->block.flags.data.backwindow = 0;
  viewer_deregisterwindow(winentry);
  return winfo;
}

/* Reopens window regardless of whether it was editable */
static BOOL viewer_rawreopen(browser_winentry *winentry, window_info *winfo,
	window_handle behind, BOOL monitor_was_active)
{
  window_state wstate;
  /* Create new window */
  if (!viewer_createwindow(winentry,(browser_winblock *) &winfo->block))
    return FALSE;
  Wimp_GetWindowState(winentry->handle,&wstate);
  wstate.openblock.behind = behind;
  if (winentry->status == status_EDITING && choices->viewtools)
    viewtools_open(&wstate.openblock,winentry->pane);
  else
    Wimp_OpenWindow(&wstate.openblock);
  viewer_roundopenblock(&wstate.openblock);
  winentry->window->window.screenrect = wstate.openblock.screenrect;
  winentry->window->window.scroll = wstate.openblock.scroll;

  /* Reactivate monitor if necessary */
  if (monitor_was_active)
    monitor_activate(NULL,winentry);
  return TRUE;
}

void viewer_reopen(browser_winentry *winentry,BOOL newtitle)
{
  window_info *winfo;
  window_handle behind;
  icon_createblock iblock;
  char *desvalid;
  BOOL monitor_was_active;

  winfo = viewer_preparewinblock(winentry, &behind, &monitor_was_active,
  	newtitle);
  /* Stop here if something went wrong */
  if (!winfo)
  {
    winentry->status = status_CLOSED;
    if (winentry->fontarray)
      Font_LoseAllFonts(winentry->fontarray);
    return;
  }

  if (!viewer_rawreopen(winentry, winfo, behind, monitor_was_active))
  {
    winentry->status = status_CLOSED;
    if (winentry->fontarray)
      Font_LoseAllFonts(winentry->fontarray);
    free(winfo);
    return;
  }
  free(winfo);
  viewer_claimeditevents(winentry);

  /* Change icon if necessary */
  if (winentry->window->window.flags.data.hscroll ||
      winentry->window->window.flags.data.vscroll)
    desvalid = browser_windowvalid;
  else
    desvalid = browser_dialogvalid;
  Wimp_GetIconState(winentry->browser->window,winentry->icon,&iblock.icondata);
  if (iblock.icondata.data.indirecttext.validstring != desvalid)
  {
    Wimp_DeleteIcon(winentry->browser->window,winentry->icon);
    iblock.window = winentry->browser->window;
    iblock.icondata.data.indirecttext.validstring = desvalid;
    Wimp_CreateIcon(&iblock,&winentry->icon);
    Icon_ForceRedraw(winentry->browser->window,winentry->icon);
  }
}

BOOL viewer_openevent(event_pollblock *event,void *reference)
{
  browser_winentry *winentry = reference;
  window_state wstate;

  /* To see if window has moved */
  Wimp_GetWindowState(event->data.openblock.window,&wstate);
  viewer_roundopenblock(&event->data.openblock);
  if (choices->viewtools && winentry != &picker_winentry &&
  	winentry->status == status_EDITING)
    viewtools_open(&event->data.openblock,winentry->pane);
  else
    Wimp_OpenWindow(&event->data.openblock);
  if (winentry->status == status_EDITING)
  {
    if (event->data.openblock.screenrect.min.x !=
    	wstate.openblock.screenrect.min.x ||
    	event->data.openblock.screenrect.min.y !=
    	wstate.openblock.screenrect.min.y ||
    	event->data.openblock.screenrect.max.x !=
    	wstate.openblock.screenrect.max.x ||
    	event->data.openblock.screenrect.max.y !=
    	wstate.openblock.screenrect.max.y ||
    	event->data.openblock.scroll.x != wstate.openblock.scroll.x ||
	event->data.openblock.scroll.y != wstate.openblock.scroll.y)
    {
      /* Change data in stored template */
      if (winentry != &picker_winentry)
        browser_settitle(winentry->browser,NULL,TRUE);
      winentry->window->window.screenrect = event->data.openblock.screenrect;
      winentry->window->window.scroll = event->data.openblock.scroll;
    }
  }

  return TRUE;
}

BOOL viewer_closeevent(event_pollblock *event,void *reference)
{
  viewer_close(reference);
  return TRUE;
}

BOOL preview_click(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
  {
    WinEd_CreateMenu(preview_menu,event->data.mouse.pos.x,event->data.mouse.pos.y);
    Event_Claim(event_MENU,event_ANY,event_ANY,preview_menuselect,reference);
    EventMsg_Claim(message_MENUSDELETED,event_ANY,
    		 preview_releasemenu,reference);
    help_claim_menu("PRM");
  }
  else if (event->data.mouse.button.data.dragselect && Kbd_KeyDown(inkey_CTRL))
  {
    drag_block drag;

    drag.window = event->data.mouse.window;
    drag.type = drag_MOVEWINDOW;
    Wimp_DragBox(&drag);
  }
  else if (event->data.mouse.button.data.dragadjust && Kbd_KeyDown(inkey_CTRL))
  {
    drag_block drag;

    drag.window = event->data.mouse.window;
    drag.type = drag_RESIZEWINDOW;
    Wimp_DragBox(&drag);
  }

  return TRUE;
}

BOOL preview_releasemenu(event_pollblock *event,void *reference)
{
  Event_Release(event_MENU,event_ANY,event_ANY,preview_menuselect,reference);
  EventMsg_Release(message_MENUSDELETED,event_ANY,preview_releasemenu);
  help_release_menu();
  return TRUE;
}

BOOL preview_menuselect(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  window_state wstate;
  int icon;
  browser_winentry *winentry = reference;

  Wimp_GetPointerInfo(&ptrinfo);

  switch (event->data.selection[0])
  {
    case preview_CLOSE:
      viewer_close(winentry);
      break;
    case preview_SAVEPOS:
      Error_Check(Wimp_GetWindowState(winentry->handle,&wstate));
      viewer_roundopenblock(&wstate.openblock);
      Error_Check(Wimp_OpenWindow(&wstate.openblock));
      Error_Check(Wimp_GetWindowState(winentry->handle,&wstate));
      winentry->window->window.screenrect = wstate.openblock.screenrect;
      winentry->window->window.scroll = wstate.openblock.scroll;
      browser_settitle(winentry->browser,NULL,TRUE);
      break;
    case preview_SAVESTATE:
      for (icon = 0;icon < winentry->window->window.numicons;icon++)
        winentry->window->icon[icon].flags.data.selected =
          Icon_GetSelect(winentry->handle,icon);
      browser_settitle(winentry->browser,NULL,TRUE);
      break;
    case preview_EDIT:
      viewer_view(winentry,TRUE);;
      break;
  }

  if (ptrinfo.button.data.adjust &&
  	event->data.selection[0] != preview_CLOSE &&
  	event->data.selection[0] != preview_EDIT)
    Menu_ShowLast();

  return TRUE;
}

/*
void viewer_roundopenblock(window_openblock *openblock)
{
  round_down_box(&openblock->screenrect);
  openblock->scroll.x = round_down_int(openblock->scroll.x);
  openblock->scroll.y = round_up_int(openblock->scroll.y);
}
*/

BOOL viewer_click(event_pollblock *event,void *reference)
{
  browser_winentry *winentry = reference;

  viewer_closechildren(winentry);

  switch (event->data.mouse.button.value)
  {
    case button_MENU:
      if (winentry != &picker_winentry)
      {
        if (choices->hotkeys)
          viewer_claimcaret(winentry);
        viewer_openmenu(event,reference);
      }
      break;
    case button_SELECT: /* Double click */
      if (winentry != &picker_winentry)
      {
        if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
        {
          if (Kbd_KeyDown(inkey_SHIFT))
            viewcom_edittitle(winentry);
          else
            windiag_open(winentry);
        }
        else
          viewcom_editicon(winentry);
      }
      else
        viewcom_clearselection();
      break;
    case button_ADJUST: /* Double click */
      if (winentry != &picker_winentry)
      {
        if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
          wadiag_open(winentry);
        else
        {
          wimp_rect newrect;

          newrect = winentry->window->icon[event->data.mouse.icon].workarearect;
          icnedit_minsize(winentry,event->data.mouse.icon,&newrect.max);
          newrect.max.x += newrect.min.x;
          newrect.max.y += newrect.min.y;
          icnedit_moveicon(winentry,event->data.mouse.icon,&newrect);
          browser_settitle(winentry->browser,0,TRUE);
        }
      }
      break;
    case button_DRAGSELECT:
      if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
      {
        if (Kbd_KeyDown(inkey_SHIFT))
        {
          drag_block drag;

          drag.window = event->data.mouse.window;
          drag.type = drag_MOVEWINDOW;
          Wimp_DragBox(&drag);
        }
        else
        drag_rubberbox(event,winentry,TRUE);
      }
      else
      {
        window_state wstate;
        int selections;
        BOOL choriz = FALSE, cvert = FALSE;

        Wimp_GetWindowState(winentry->handle,&wstate);
        Icon_Select(event->data.mouse.window,event->data.mouse.icon);
        viewer_dragref.winentry = winentry;
        viewer_dragref.icon = event->data.mouse.icon;
        viewer_dragref.scroll = wstate.openblock.scroll;
        viewer_dragref.shift = Kbd_KeyDown(inkey_SHIFT);
        if (Kbd_KeyDown(inkey_ALT))
        {
          wimp_rect srect;
          /*int bl, tr;
          int h, v; */

          srect.min.x =
            winentry->window->icon[event->data.mouse.icon].workarearect.min.x
            + wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x;
          srect.min.y =
            winentry->window->icon[event->data.mouse.icon].workarearect.min.y
            + wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y;
          srect.max.x =
            winentry->window->icon[event->data.mouse.icon].workarearect.max.x
            + wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x;
          srect.max.y =
            winentry->window->icon[event->data.mouse.icon].workarearect.max.y
            + wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y;
          switch (drag_nearestedge(&srect,
          	event->data.mouse.pos.x, event->data.mouse.pos.y))
          {
            case drag_LeftEdge:
            case drag_RightEdge:
              choriz = FALSE;
              cvert = TRUE;
              break;
            case drag_BottomEdge:
            case drag_TopEdge:
              choriz = TRUE;
              cvert = FALSE;
              break;
          }
          /*
          bl = event->data.mouse.pos.x - srect.min.x;
          tr = srect.max.x - event->data.mouse.pos.x;
          h = bl < tr ? bl : tr;
          bl = event->data.mouse.pos.y - srect.min.y;
          tr = srect.max.y - event->data.mouse.pos.y;
          v = bl < tr ? bl : tr;
          cvert = v > h;
          choriz = !cvert;
          */
        }
        else
          choriz = cvert = FALSE;
        selections = count_selections(event->data.mouse.window);
        if (selections == 1)
        {
          viewer_cancelref vcr;

          drag_moveicon(winentry,event->data.mouse.icon,choriz,cvert);
          Event_Claim(event_USERDRAG,event_ANY,event_ANY,
          	    viewer_moveonedrag,reference);
          if (winentry != &picker_winentry)
            Event_Claim(event_NULL,event_ANY,event_ANY,
            	        (event_handler) viewer_scrolldrag,
            	        (void *) winentry->handle);
          vcr.handler = viewer_moveonedrag;
          Event_Claim(event_KEY, winentry->handle, event_ANY,
          	viewer_canceldraghandler, vcr.reference);
        }
        else
        {
          viewer_cancelref vcr;

          drag_moveselection(winentry,&viewer_dragref.startbox,choriz,cvert);
          Event_Claim(event_USERDRAG,event_ANY,event_ANY,
          	      viewer_movemanydrag,reference);
          if (winentry != &picker_winentry)
            Event_Claim(event_NULL,event_ANY,event_ANY,
          	        (event_handler) viewer_scrolldrag,
          	        (void *) winentry->handle);
          vcr.handler = viewer_movemanydrag;
          Event_Claim(event_KEY, winentry->handle, event_ANY,
          	viewer_canceldraghandler, vcr.reference);
        }
      }
      break;
    case button_DRAGADJUST:
      if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
      {
        if (Kbd_KeyDown(inkey_SHIFT))
        {
          drag_block drag;

          drag.window = event->data.mouse.window;
          drag.type = drag_RESIZEWINDOW;
          Wimp_DragBox(&drag);
        }
        else
          drag_rubberbox(event,winentry,TRUE);
      }
      else if (winentry != &picker_winentry)
      {
        viewer_cancelref vcr;

        viewer_dragref.winentry = winentry;
        viewer_dragref.icon = event->data.mouse.icon;
	viewer_dragref.shift = Kbd_KeyDown(inkey_SHIFT);
        drag_resizeicon(winentry,event);
        Event_Claim(event_USERDRAG,event_ANY,event_ANY,
        	    viewer_resizedrag,reference);
        vcr.handler = viewer_resizedrag;
        Event_Claim(event_KEY, winentry->handle, event_ANY,
        	viewer_canceldraghandler, vcr.reference);
      }
      break;
    case button_CLICKSELECT: /* Single click */
      if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
        viewcom_clearselection();
      else
      {
        /* Clear previous selection if icon under pointer not already selected
         */
        if (!Icon_GetSelect(winentry->handle,event->data.mouse.icon))
        {
          BOOL tools = choices->viewtools;
          choices->viewtools = FALSE;
          viewcom_clearselection();
          choices->viewtools = tools;
        }
        viewer_setselect(winentry,event->data.mouse.icon,TRUE);
        selection_viewer = winentry;
        viewer_selection_withmenu = FALSE;
      }
      if (choices->hotkeys && winentry != &picker_winentry)
        viewer_claimcaret(winentry);
      break;
    case button_CLICKADJUST: /* Single click */
      if (event->data.mouse.icon != -1)
      {
        if (selection_viewer != winentry)
          viewcom_clearselection();
        viewer_setselect(winentry,event->data.mouse.icon,
        	       !Icon_GetSelect(winentry->handle,
        	       		       event->data.mouse.icon));
        selection_viewer = winentry;
        viewer_selection_withmenu = (count_selections(winentry->handle) == 0);
      }
      if (choices->hotkeys && winentry != &picker_winentry)
        viewer_claimcaret(winentry);
      break;
  }
  if (choices->viewtools)
    viewtools_shadeapp(winentry);
  return TRUE;
}

BOOL viewer_resizedrag(event_pollblock *event,void *reference)
{
  window_state wstate;
  wimp_point origin;
  wimp_rect box;
  viewer_cancelref vcr;

  vcr.handler = viewer_resizedrag;
  viewer_canceldrag(vcr);

  Wimp_GetWindowState(viewer_dragref.winentry->handle,&wstate);
  /* Convert drag screen coords to work area coords */
  origin.x = wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x;
  origin.y = wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y;
  box.min.x = event->data.screenrect.min.x - origin.x;
  box.min.y = event->data.screenrect.min.y - origin.y;
  box.max.x = event->data.screenrect.max.x - origin.x;
  box.max.y = event->data.screenrect.max.y - origin.y;
  if (box.min.x > box.max.x)
  {
    box.min.x ^= box.max.x;
    box.max.x ^= box.min.x;
    box.min.x ^= box.max.x;
  }
  if (box.min.y > box.max.y)
  {
    box.min.y ^= box.max.y;
    box.max.y ^= box.min.y;
    box.min.y ^= box.max.y;
  }
  if (Kbd_KeyDown(inkey_SHIFT))
    /* If alt is pressed, only round down by 2, not 4 */
    round_down_box_less(&box);
  else
    round_down_box(&box);

  /* Resize, passing logical icon */
  icnedit_moveicon(viewer_dragref.winentry,viewer_dragref.icon,&box);
  browser_settitle(viewer_dragref.winentry->browser,0,TRUE);
  return FALSE;
}

void viewer_clearselection()
{
  window_info winfo;
  int icon;
  icon_handle *selection;

  if (!selection_viewer)
    return;

  Hourglass_On();

  Window_GetInfo3(selection_viewer->handle,&winfo);
  selection = malloc((winfo.block.numicons + 1) * sizeof(icon_handle));
  if (!selection)
  {
    MsgTrans_Report(messages,"ViewMem",FALSE);
    return;
  }
  Wimp_WhichIcon(selection_viewer->handle, selection,
  	icon_SELECTED, icon_SELECTED);
  for (icon = 0; selection[icon] != -1; icon++)
    viewer_setselect(selection_viewer,selection[icon],FALSE);
  free(selection);
  if (choices->viewtools)
    viewtools_shadeapp(selection_viewer);
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  selection_viewer = NULL;
  viewer_selection_withmenu = TRUE;
  Hourglass_Off();
}

static void viewer_canceldrag(viewer_cancelref vcr)
{
  char ptr_name[] = "ptr_default";
  mouse_block ptrinfo;
  browser_winentry *destination;

  Event_Release(event_USERDRAG,event_ANY,event_ANY, vcr.handler,viewer_dragref.winentry);

  if (vcr.handler != viewer_resizedrag && viewer_dragref.winentry != &picker_winentry)
    Event_Release(event_NULL,event_ANY,event_ANY, (event_handler) viewer_scrolldrag,
          	  (void *) viewer_dragref.winentry->handle);

  Event_Release(event_KEY, viewer_dragref.winentry->handle, event_ANY,
  	viewer_canceldraghandler, vcr.reference);

  /* Monitor update routine needs to know as we do things differently while dragging... */
  monitor_dragging = FALSE;
  drag_sidemove.min.x = FALSE;
  drag_sidemove.max.x = FALSE;
  drag_sidemove.min.y = FALSE;
  drag_sidemove.max.y = FALSE;

  /* At the end of the drag, we might have moved from one window to another.   */
  /* As initiating a drag /doesn't/ deactivate the monitor we need to reset it */
  /* to make sure it's monitoring the right window.                            */

  /* Deactivate old monitor */
  monitor_deactivate(NULL, monitor_winentry);

  Wimp_GetPointerInfo(&ptrinfo);
  /* Find window at end of drag and activate monitor (returns NULL if none found - e.g. someone else's window) */
  destination = find_winentry_from_window(ptrinfo.window);
  if (destination) monitor_activate(NULL, destination);

  /* Reset pointer */
  Error_Check(SWI(8, 0, SWI_Wimp_SPRITEOP, 36, 0, ptr_name, 1, 0, 0, 0, 0));
}

static BOOL viewer_canceldraghandler(event_pollblock *event,void *reference)
{
  if (event->data.key.code == keycode_ESCAPE)
  {
    viewer_cancelref vcr;

    Wimp_DragBox(NULL);
    vcr.reference = reference;
    viewer_resetscrollbars();
    viewer_canceldrag(vcr);
    return TRUE;
  }
  return FALSE;
}

BOOL viewer_moveonedrag(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  wimp_box workarearect;
  window_state wstate;
  viewer_cancelref vcr;

  vcr.handler = viewer_moveonedrag;
  viewer_canceldrag(vcr);

  /* Find destination window */
  Wimp_GetPointerInfo(&ptrinfo);

  /* Find workarea coords of drag */
  if (ptrinfo.window != -1 && ptrinfo.window != -2)
  {
    workarearect = event->data.screenrect;
    Wimp_GetWindowState(ptrinfo.window,&wstate);
    Coord_RectToWorkArea(&workarearect,
    			 (convert_block *) &wstate.openblock.screenrect);
    round_down_box(&workarearect);
  }

  if (ptrinfo.window == viewer_dragref.winentry->handle)
  {
    if (viewer_dragref.winentry != &picker_winentry)
    {
      if (!viewer_dragref.shift)
      {
        /* Move, passing logical icon */
        icnedit_moveicon(viewer_dragref.winentry,viewer_dragref.icon,
      		         &workarearect);
      }
      else /* Copy to same winentry */
      {
        icnedit_copyicon(viewer_dragref.winentry,viewer_dragref.icon,
      		         viewer_dragref.winentry,&workarearect);
      }
      browser_settitle(viewer_dragref.winentry->browser,0,TRUE);
    }
  }
  else /* export */
  {
    browser_winentry *dest;

    dest = find_winentry_from_window(ptrinfo.window);
    if (dest)
    {
      icnedit_copyicon(viewer_dragref.winentry,viewer_dragref.icon,
      			dest,&workarearect);
      browser_settitle(dest->browser,0,TRUE);
      if (viewer_dragref.shift &&
          viewer_dragref.winentry != &picker_winentry) /* Delete
      	  		       	  	      		  original */
      {
        icnedit_deleteicon(viewer_dragref.winentry,viewer_dragref.icon);
        browser_settitle(viewer_dragref.winentry->browser,0,TRUE);
      }
    }
    viewer_resetscrollbars();
  }
  if (choices->viewtools)
    viewtools_shadeapp(viewer_dragref.winentry);
  return FALSE;
}

void viewer_newbounds(browser_winentry *winentry,int icon,
     		      wimp_rect *newrect,wimp_rect *dragrect,
     		      wimp_rect *startrect)
{
  wimp_point size;

  /* Copy icon's original bounds to newrect to avoid typing nested
     structure 4 times */
  *newrect = winentry->window->icon[icon].workarearect;
  /* Find size of icon */
  size.x = newrect->max.x - newrect->min.x;
  size.y = newrect->max.y - newrect->min.y;
  /* New position of icon is position of dragrect plus difference between
     startrect and old icon position */
  newrect->min.x = dragrect->min.x +
  		   newrect->min.x - startrect->min.x;
  newrect->min.y = dragrect->min.y +
  		   newrect->min.y - startrect->min.y;
  /* Opposite corner found by adding size */
  newrect->max.x = newrect->min.x + size.x;
  newrect->max.y = newrect->min.y + size.y;
}

void viewer_copyselection(browser_winentry *source,browser_winentry *dest,
     			  wimp_rect *oldrect,wimp_rect *newrect,
     			  BOOL shift)
{
  icon_handle icon, newicon;
  /* We don't want to carry on looping through the new selected icons we create */
  int existing_selected = source->window->window.numicons;

  Hourglass_On();

  for (icon = 0; icon < existing_selected; icon++)
  {
    icon_block istate;

    /* Find each selected icon */
    Wimp_GetIconState(source->handle,icon,&istate);
    if (!istate.flags.data.deleted && istate.flags.data.selected)
    {
      wimp_rect newbounds;

      /* Find its new bounds */
      viewer_newbounds(source,icon,&newbounds,newrect,oldrect);

      /* Do copy */
      if (source == dest) /* Within same window */
      {
        if (source != &picker_winentry)
        {
          if (!shift) /* Shift not pressed so simply move */
            icnedit_moveicon(source,icon,&newbounds);
          else /* Copy */
          {
            viewer_setselect(source, icon, FALSE);
            newicon = icnedit_copyicon(source,icon,dest,&newbounds); /* Adds 1 to source->window->window.numicons */
            viewer_setselect(dest, newicon, TRUE); /* though source == dest */
          }
        }
      }
      else /* To a different window */
      {
        icnedit_copyicon(source,icon,dest,&newbounds);
      }
    }
  }
  if (source != &picker_winentry)
    browser_settitle(source->browser,0,TRUE);
  if (source != dest)
  {
    browser_settitle(dest->browser,0,TRUE);
    if (shift && source != &picker_winentry) /* Shift pressed so
        	     	      	 		delete originals (move) */
      viewcom_delete(source);
  }

  Hourglass_Off();
}

BOOL viewer_movemanydrag(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  wimp_rect workarearect; /* Of where drag has finished */
  window_info winfo;
  browser_winentry *dest;
  viewer_cancelref vcr;

  vcr.handler = viewer_movemanydrag;
  viewer_canceldrag(vcr);

  /* Find destination window */
  Wimp_GetPointerInfo(&ptrinfo);

  if (ptrinfo.window == -1 || ptrinfo.window == -2)
  {
    viewer_resetscrollbars();
    return FALSE;
  }

  /* Find destination winentry */
  dest = find_winentry_from_window(ptrinfo.window);

  if (!dest)
  {
    viewer_resetscrollbars();
    return FALSE;
  }

  /* Find work area bounds of drag */
  workarearect = event->data.screenrect;
  Window_GetInfo3(ptrinfo.window,&winfo);
  Coord_RectToWorkArea(&workarearect,
  			 (convert_block *) &winfo.block.screenrect);
  round_down_box(&workarearect);

  viewer_copyselection(viewer_dragref.winentry,dest,
  		       &viewer_dragref.startbox,&workarearect,
  		       viewer_dragref.shift);

  /* Return source's scroll bars to original position if 'exporting' */
  if (dest != viewer_dragref.winentry)
    viewer_resetscrollbars();

  return FALSE;
}

BOOL viewer_scrolldrag(event_pollblock *event,window_handle window)
{
  window_state wstate;
  mouse_block ptrinfo;
  wimp_point scrollby;

  Wimp_GetWindowState(window,&wstate);
  Wimp_GetPointerInfo(&ptrinfo);

  if (ptrinfo.pos.x < wstate.openblock.screenrect.min.x)
    scrollby.x = ptrinfo.pos.x - wstate.openblock.screenrect.min.x;
  else if (ptrinfo.pos.x > wstate.openblock.screenrect.max.x)
    scrollby.x = ptrinfo.pos.x - wstate.openblock.screenrect.max.x;
  else
    scrollby.x = 0;
  if (ptrinfo.pos.y < wstate.openblock.screenrect.min.y)
    scrollby.y = ptrinfo.pos.y - wstate.openblock.screenrect.min.y;
  else if (ptrinfo.pos.y > wstate.openblock.screenrect.max.y)
    scrollby.y = ptrinfo.pos.y - wstate.openblock.screenrect.max.y;
  else
    scrollby.y = 0;

  if (scrollby.x || scrollby.y)
  {
    wstate.openblock.scroll.x += scrollby.x;
    wstate.openblock.scroll.y += scrollby.y;
    Wimp_OpenWindow(&wstate.openblock);
  }

  if (monitor_isactive)
    return FALSE;
  return TRUE;
}

void viewer_resetscrollbars()
{
  event_pollblock event;

  event.type = event_OPEN;
  Wimp_GetWindowState(viewer_dragref.winentry->handle,
  		      (window_state *) &event.data.openblock);
  event.data.openblock.scroll = viewer_dragref.scroll;
  viewer_openevent(&event,viewer_dragref.winentry);
}

BOOL viewer_iconise(event_pollblock *event,browser_winentry *winentry)
{
  message_block message = event->data.message;

  /*if (event->data.message.data.windowinfo.window != winentry->handle)
    return FALSE;*/

  message.header.size = sizeof(message_header) + sizeof(message_windowinfo);
  /* message.header.sender = 0; */
  message.header.yourref = message.header.myref;
  /* message.header.myref = 0;
  message.header.action = message_WINDOWINFO; */

  if (winentry->window->window.flags.data.vscroll ||
      winentry->window->window.flags.data.hscroll)
    strcpy(message.data.windowinfo.spritename,"window");
  else
    strcpy(message.data.windowinfo.spritename,"dialog");
  strncpy(message.data.windowinfo.title,winentry->identifier,20);
  message.data.windowinfo.title[19] = 0;
  Error_Check(Wimp_SendMessage(event_SEND,&message,
                               event->data.message.header.sender,0));

  viewer_closechildren(winentry);

  return TRUE;
}

BOOL viewer_menuselect(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  wimp_point lastmenupos;
  browser_winentry *winentry = reference;

  Wimp_GetPointerInfo(&ptrinfo);

  switch (event->data.selection[0])
  {
    case viewer_ICON:
      switch (event->data.selection[1])
      {
        case viewer_DELETE:
          viewcom_delete(winentry);
          viewer_preparemenu(winentry);
          break;
        case viewer_COPY:
          switch (event->data.selection[2])
          {
            case viewer_DOWN:
              viewcom_copy(winentry, copy_DOWN);
              break;
            case viewer_RIGHT:
              viewcom_copy(winentry, copy_RIGHT);
              break;
            case viewer_LEFT:
              viewcom_copy(winentry, copy_LEFT);
              break;
            case viewer_UP:
              viewcom_copy(winentry, copy_UP);
              break;
          }
          break;
        case viewer_RENUMBER:
          viewcom_quickrenum(winentry);
          break;
        case viewer_EDITICON:
          viewcom_editicon(winentry);
          break;
        case viewer_ALIGN:
          viewcom_align(winentry);
          break;
        case viewer_SPACEOUT:
          viewcom_spaceout(winentry);
          break;
        case viewer_RESIZE:
          viewcom_resize(winentry);
          break;
        case viewer_COORDS:
          viewcom_coords(winentry);
          break;
        case viewer_BORDER:
          viewcom_border(winentry);
          break;
      }
      break;
    case viewer_SELALL:
      lastmenupos = menu_currentpos;
      viewcom_selectall(winentry);
      if (ptrinfo.button.data.adjust)
      {
        viewer_openmenu_at(winentry, lastmenupos.x + 64, lastmenupos.y);
        return TRUE;
      }
      break;
    case viewer_CLEARSEL:
      lastmenupos = menu_currentpos;
      viewcom_clearselection();
      if (ptrinfo.button.data.adjust)
      {
        viewer_openmenu_at(winentry, lastmenupos.x + 64, lastmenupos.y);
        return TRUE;
      }
      break;
    case viewer_WORKAREA:
      viewcom_editworkarea(winentry);
      break;
    case viewer_VISAREA:
      viewcom_editvisarea(winentry);
      break;
    case viewer_EDITWIN:
      viewcom_editwindow(winentry);
      break;
    case viewer_EDITTITLE:
      viewcom_edittitle(winentry);
      break;
    case viewer_CLOSE:
      viewer_close(winentry);
      break;
    case viewer_PREVIEW:
      viewer_view(winentry,FALSE);
      break;
  }

  if (ptrinfo.button.data.adjust &&
  	event->data.selection[0]!=viewer_CLOSE &&
  	event->data.selection[0]!=viewer_PREVIEW)
    Menu_ShowLast();

  return TRUE;
}

BOOL viewer_sublink(event_pollblock *event,void *reference)
{
  if (event->data.message.data.menuwarn.selection[0] == viewer_ICON &&
  	event->data.message.data.menuwarn.selection[1] == viewer_RENUMBER)
  {
    viewcom_renum(TRUE, event->data.message.data.menuwarn.openpos.x,
      	  	event->data.message.data.menuwarn.openpos.y, reference);
  }

  /*menu_destroy = FALSE;*/

  return TRUE;
}

BOOL viewer_releasemenu(event_pollblock *event,void *reference)
{
  Event_Release(event_MENU,event_ANY,event_ANY,viewer_menuselect,reference);
  EventMsg_Release(message_MENUSDELETED,event_ANY,viewer_releasemenu);
  EventMsg_Release(message_MENUWARN,event_ANY,viewer_sublink);
  help_release_menu();
  viewer_menuopen = FALSE;
  if (/*!menu_destroy && */renum_open)
    release_renum();
  return TRUE;
}

void viewer_numbermenu(int value)
{
  char buffer[32];
  MsgTrans_Lookup(messages,"Renumber",buffer,24);
  sprintf(buffer + strlen(buffer)," %d",value);
  if (choices->hotkeys)
  {
    if (value < 100)
      strcat(buffer," ");
    if (value < 10)
      strcat(buffer," ");
    strcat(buffer," ^N");
  }
  Menu_SetText(viewer_iconmenu,viewer_RENUMBER,buffer);
  Icon_SetInteger(renum_dbox,2,value);
}

void viewer_iconmenufield(int value)
{
  char buffer[32];

  MsgTrans_Lookup(messages,"VMIcon",buffer,32);
  sprintf(buffer + strlen(buffer)," %d",value);
  Menu_SetText(viewer_parentmenu,viewer_ICON,buffer);
}

static void viewer_shademultisel()
{
  char buffer[64];

  Menu_SetFlags(viewer_iconmenu,viewer_COORDS,0,1);
  MsgTrans_Lookup(messages,"BMSel",buffer,64);
  strncpy(viewer_iconmenu->title,buffer,12);
  Menu_SetText(viewer_parentmenu,viewer_ICON,buffer);
}

static void viewer_selunshade()
{
  Menu_SetFlags(viewer_parentmenu,viewer_ICON,0,0);
  Menu_SetFlags(viewer_parentmenu,viewer_CLEARSEL,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_DELETE,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_RENUMBER,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_DELETE,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_SPACEOUT,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_RESIZE,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_BORDER,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_ALIGN,0,0);
  Menu_SetFlags(viewer_iconmenu,viewer_EDITICON,0,0);
}

void viewer_preparemenu(browser_winentry *winentry)
{
  int selections;
  int selected; /* logical icon */
  char buffer[64];

  selections = count_selections(winentry->handle);
  switch (selections)
  {
    case 0:
      Menu_SetFlags(viewer_parentmenu,viewer_ICON,0,1);
      Menu_SetFlags(viewer_parentmenu,viewer_CLEARSEL,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_DELETE,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_RENUMBER,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_DELETE,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_SPACEOUT,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_RESIZE,0,1);
      Menu_SetFlags(viewer_iconmenu,viewer_BORDER,0,1);
      viewer_shademultisel();
      break;
    case 1:
      for (selected = 0;selected < winentry->window->window.numicons;selected++)
        if (Icon_GetSelect(winentry->handle,selected))
          break;
      MsgTrans_Lookup(messages,"VMIcon",buffer,64);
      strncpy(viewer_iconmenu->title,buffer,12);
      viewer_iconmenufield(selected);
      Menu_SetFlags(viewer_iconmenu,viewer_COORDS,0,0);
      viewer_selunshade();
      break;
    default:
      viewer_selunshade();
      viewer_shademultisel();
      break;
  }
}

static void viewer_openmenu_at(browser_winentry *winentry, int x, int y)
{
  viewer_preparemenu(winentry);
  WinEd_CreateMenu(viewer_parentmenu,x,y);
  /*menu_destroy = */ viewer_menuopen = TRUE;
  Event_Claim(event_MENU,event_ANY,event_ANY,viewer_menuselect,winentry);
  EventMsg_Claim(message_MENUSDELETED,event_ANY,viewer_releasemenu,winentry);
  EventMsg_Claim(message_MENUWARN,event_ANY,viewer_sublink,winentry);
  help_claim_menu("VRM");
}

void viewer_openmenu(event_pollblock *event,browser_winentry *winentry)
{
  /* Work out whether this is a multiple selection or single icon */
  if (viewer_selection_withmenu && event->data.mouse.icon != -1)
  {
    /* Select icon under pointer */
    viewcom_clearselection();
    Icon_Select(winentry->handle,event->data.mouse.icon);
    selection_viewer = winentry;
  }
  viewer_openmenu_at(winentry,event->data.mouse.pos.x,event->data.mouse.pos.y);
}

BOOL viewer_redraw(event_pollblock *event,void *reference)
{
  browser_winentry *winentry = reference;
  int icon;
  window_redrawblock redraw;
  BOOL more;
  int origin_x,origin_y;

  redraw.window = winentry->handle;
  Wimp_RedrawWindow(&redraw,&more);
  origin_x = redraw.rect.min.x - redraw.scroll.x;
  origin_y = redraw.rect.max.y - redraw.scroll.y;
  while (more)
  {
    /* Plot cross-hatching if necessary */
    if (choices->hatchredraw &&
        !winentry->window->window.flags.data.autoredraw &&
    	winentry != &picker_winentry)
    {
      int x;
      int height = redraw.cliprect.max.y - redraw.cliprect.min.y;
      int xstart = redraw.cliprect.min.x - height;
      int xcorr = (xstart - origin_x) % HATCH_SIZE;
      int ycorr = (origin_y - redraw.cliprect.min.y) % HATCH_SIZE;
      for (x = xstart-xcorr-ycorr;
      	   x < redraw.cliprect.max.x; x += HATCH_SIZE)
      {
        GFX_Move(x, redraw.cliprect.min.y);
        GFX_DrawBy(height, height);
      }
      ycorr = (origin_y - redraw.cliprect.max.y) % HATCH_SIZE;
      for (x = xstart-xcorr+ycorr-HATCH_SIZE;
      	   x < redraw.cliprect.max.x; x += HATCH_SIZE)
      {
        GFX_Move(x, redraw.cliprect.max.y-1);
        GFX_DrawBy(height, -height);
      }
    }

    /* Convert cliprect to work area coords */
    Coord_RectToWorkArea(&redraw.cliprect,(convert_block *) &redraw.rect);
    /* Find icons within this rect that have R6 validation */
    for (icon = 0;icon < winentry->window->window.numicons;icon++)
    {
      if (Coord_RectsOverlap(&redraw.cliprect,
      	  		     &winentry->window->icon[icon].workarearect) &&
          icnedit_valid6(winentry,icon))
      {
        /* Draw 'moat' border around icon by plotting dummyborder */
        viewer_dummyborder.workarearect =
          winentry->window->icon[icon].workarearect;
        Wimp_PlotIcon(&viewer_dummyborder);
      }
    }
    Wimp_GetRectangle(&redraw,&more);
  }
  return TRUE;
}


void viewcom_copy(browser_winentry *winentry,viewer_copydirection direction)
{
  wimp_rect sourcerect,destrect;
  wimp_point size;

  drag_getselectionbounds(winentry,&sourcerect,&destrect);
                                                           /* destrect is
  			  				      dummy here */
  /* Find size of selection bounds */
  size.x = sourcerect.max.x - sourcerect.min.x;
  size.y = sourcerect.max.y - sourcerect.min.y;
  switch (direction)
  {
    case copy_LEFT:
      destrect.min.x = sourcerect.min.x - size.x - copy_SPACE;
      destrect.min.y = sourcerect.min.y;
      destrect.max.x = destrect.min.x + size.x;
      destrect.max.y = destrect.min.y + size.y;
      break;
    case copy_RIGHT:
      destrect.min.x = sourcerect.min.x + size.x + copy_SPACE;
      destrect.min.y = sourcerect.min.y;
      destrect.max.x = destrect.min.x + size.x;
      destrect.max.y = destrect.min.y + size.y;
      break;
    case copy_UP:
      destrect.min.x = sourcerect.min.x;
      destrect.min.y = sourcerect.min.y + size.y + copy_SPACE;
      destrect.max.x = destrect.min.x + size.x;
      destrect.max.y = destrect.min.y + size.y;
      break;
    case copy_DOWN:
      destrect.min.x = sourcerect.min.x;
      destrect.min.y = sourcerect.min.y - size.y - copy_SPACE;
      destrect.max.x = destrect.min.x + size.x;
      destrect.max.y = destrect.min.y + size.y;
      break;
    default:
      destrect.min.x = sourcerect.min.x + copy_DISTANCE;
      destrect.min.y = sourcerect.min.y - copy_DISTANCE;
      destrect.max.x = sourcerect.max.x + copy_DISTANCE;
      destrect.max.y = sourcerect.max.y - copy_DISTANCE;
      break;
  }
  viewer_copyselection(winentry,winentry,&sourcerect,&destrect,TRUE);
}


void viewcom_delete(browser_winentry *winentry)
{
  icon_handle icon;

  if (choices->confirm)
  {
    os_error err;
    err.errnum = 0;
    MsgTrans_Lookup(messages,"ConfirmDel",err.errmess,252);
    if (!ok_report(&err))
      return;
  }

  Hourglass_On();

  for (icon = winentry->window->window.numicons - 1; icon >= 0; icon--)
  {
    icon_block istate;

    Wimp_GetIconState(winentry->handle,icon,&istate);
    if (istate.flags.data.selected && !istate.flags.data.deleted)
      icnedit_deleteicon(winentry,icon);
  }
  browser_settitle(winentry->browser,0,TRUE);
  if (choices->viewtools)
    viewtools_shadeapp(winentry);

  Hourglass_Off();
}

void viewer_swapicons(browser_winentry *winentry,int icon,int value)
{
  icon_block swap;
  icon_createblock orig,with;

  /* Swap icon blocks */
  swap = winentry->window->icon[icon];
  winentry->window->icon[icon] = winentry->window->icon[value];
  winentry->window->icon[value] = swap;
  browser_settitle(winentry->browser,0,TRUE);

  /* Make sure physical icons are stacked in correct order */
  orig.window = with.window = winentry->handle;
  Wimp_GetIconState(winentry->handle,icon,&orig.icondata);
  Wimp_GetIconState(winentry->handle,value,&with.icondata);
  /* We're recreating these icons straight away, so use simple delete */
  Wimp_DeleteIcon(winentry->handle,icon);
  Wimp_DeleteIcon(winentry->handle,value);
  if (icon < value) /* New 'icon' is > new 'value' therefore create new
      	     	       'value' (in 'with') 1st */
  {
    Wimp_CreateIcon(&with,&icon);
    Wimp_CreateIcon(&orig,&value);
  }
  else /* Vice versa */
  {
    Wimp_CreateIcon(&orig,&value);
    Wimp_CreateIcon(&with,&icon);
  }

  /* Redraw them */
  icnedit_forceredraw(winentry,icon);
  icnedit_forceredraw(winentry,value);
}

void viewcom_selectall(browser_winentry *winentry)
{
  int icon;

  Hourglass_On();
  viewcom_clearselection();
  for (icon = 0;icon < winentry->window->window.numicons;icon++)
    viewer_setselect(winentry,icon,TRUE);
  if (choices->viewtools)
    viewtools_shadeapp(winentry);
  viewer_selection_withmenu = FALSE;
  selection_viewer = winentry;
  Hourglass_Off();
}

void viewcom_editicon(browser_winentry *winentry)
{
  switch (count_selections(winentry->handle))
  {
    case 0:
      return;
    case 1:
      icndiag_open(winentry,Icon_WhichRadioInEsg(winentry->handle,1));
      break;
    default:
      multiicon_open(winentry);
      break;
  }
}

void viewer_claimcaret(browser_winentry *winentry)
{
  caret_block caret;

  caret.window = winentry->handle;
  caret.icon = -1;
  caret.offset.x = 0;
  caret.offset.y = 0;
  caret.height = 40 | (1 << 25);
  caret.index = -1;
  Wimp_SetCaretPosition(&caret);
}

void viewer_moveselection(browser_winentry *winentry, int xby, int yby, BOOL snap)
{
  icon_handle i;

  for (i = 0; i < winentry->window->window.numicons; i++)
  {
    if (Icon_GetSelect(winentry->handle, i))
    {
      wimp_rect newrect;
      newrect.min.x = winentry->window->icon[i].workarearect.min.x + xby;
      newrect.min.y = winentry->window->icon[i].workarearect.min.y + yby;
      newrect.max.x = winentry->window->icon[i].workarearect.max.x + xby;
      newrect.max.y = winentry->window->icon[i].workarearect.max.y + yby;

      if (snap) round_down_box(&newrect);

      icnedit_moveicon(winentry, i, &newrect);
    }
  }
  monitor_moved = TRUE;
  browser_settitle(winentry->browser,0,TRUE);
}

static void viewer_movepointer(int x,int y)
{
  struct {
    char reason;
    char x_lsb,x_msb,y_lsb,y_msb;
  } ptrblk;
  short int xpos,ypos;

  ptrblk.reason = 6;  /* Read pointer */
  OS_Word(osword_DEFINEPOINTERANDMOUSE,&ptrblk);

  xpos = ptrblk.x_lsb + (ptrblk.x_msb << 8) + x;
  ypos = ptrblk.y_lsb + (ptrblk.y_msb << 8) + y;

  ptrblk.x_lsb = xpos & 0xff;
  ptrblk.x_msb = xpos >> 8;
  ptrblk.y_lsb = ypos & 0xff;
  ptrblk.y_msb = ypos >> 8;

  ptrblk.reason = 3;  /* Set mouse */
  OS_Word(osword_DEFINEPOINTERANDMOUSE,&ptrblk);
}

BOOL viewer_hotkey(event_pollblock *event,void *reference)
{
  browser_winentry *winentry = reference;
  int selections = count_selections(winentry->handle);

  switch (event->data.key.code)
  {
    /* Move by 4 pixels a shot */
    case keycode_CURSORLEFT:
    {
      if (selections && choices->mouseless_move) viewer_moveselection(winentry, -4, 0, TRUE);
      viewer_movepointer(-4, 0);
      return TRUE;
    }
    case keycode_CURSORRIGHT:
      if (selections && choices->mouseless_move) viewer_moveselection(winentry, 4, 0, TRUE);
      viewer_movepointer(4, 0);
      return TRUE;
    case keycode_CURSORUP:
      if (selections && choices->mouseless_move) viewer_moveselection(winentry, 0, 4, TRUE);
      viewer_movepointer(0, 4);
      return TRUE;
    case keycode_CURSORDOWN:
      if (selections && choices->mouseless_move) viewer_moveselection(winentry, 0, -4, TRUE);
      viewer_movepointer(0, -4);
      return TRUE;

    /* Move by 2 pixels a shot */
    case keycode_SHIFT_CURSORLEFT:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, -2, 0, FALSE);
      viewer_movepointer(-2, 0);
      return TRUE;
    case keycode_SHIFT_CURSORRIGHT:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 2, 0, FALSE);
      viewer_movepointer(2, 0);
      return TRUE;
    case keycode_SHIFT_CURSORUP:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 0, 2, FALSE);
      viewer_movepointer(0, 2);
      return TRUE;
    case keycode_SHIFT_CURSORDOWN:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 0, -2, FALSE);
      viewer_movepointer(0, -2);
      return TRUE;

    /* Move by 1 pixel a shot */
    case keycode_CTRL_CURSORLEFT:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, -1, 0, FALSE);
      viewer_movepointer(-1, 0);
      return TRUE;
    case keycode_CTRL_CURSORRIGHT:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 1, 0, FALSE);
      viewer_movepointer(1, 0);
      return TRUE;
    case keycode_CTRL_CURSORUP:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 0, 1, FALSE);
      viewer_movepointer(0, 1);
      return TRUE;
    case keycode_CTRL_CURSORDOWN:
      if (selections && choices->mouseless_move)
        viewer_moveselection(winentry, 0, -1, FALSE);
      viewer_movepointer(0, -1);
      return TRUE;

    case keycode_CTRL_F2:
      if (viewer_menuopen)
      {
        WinEd_CreateMenu((menu_ptr) -1, 0, 0);
        viewer_releasemenu(NULL, winentry);
      }
      viewcom_closewindow(winentry);
      break;
    case 'Z' - 'A' + 1:
      viewcom_clearselection();
      break;
    case 'A' - 'A' + 1:
      viewcom_selectall(winentry);
      break;
    case 'K' - 'A' + 1:
      if (selections)
        viewcom_delete(winentry);
      break;
    case keycode_DELETE: /* For some reason this seems to be backspace, not delete        */
                         /* The delete key seems to return the same as ctrl-cursor-right! */
      if (selections)
        viewcom_delete(winentry);
      break;
    case 'O' - 'A' + 1:
      viewcom_editworkarea(winentry);
      break;
    case 'V' - 'A' + 1:
      viewcom_editvisarea(winentry);
      break;
    case 'W' - 'A' + 1:
      viewcom_editwindow(winentry);
      break;
    case 'T' - 'A' + 1:
      viewcom_edittitle(winentry);
      break;
    case 'N' - 'A' + 1:
      if (selections)
      {
        if (Kbd_KeyDown(inkey_SHIFT))
          viewcom_quickrenum(winentry);
        else
          viewcom_renum(FALSE, 0, 0, reference);
      }
      break;
    case 'E' - 'A' + 1:
      if (selections)
        viewcom_editicon(winentry);
      break;
    case 'L' - 'A' + 1:
      if (selections)
        viewcom_align(winentry);
      break;
    case 'S' - 'A' + 1:
      if (selections)
        viewcom_spaceout(winentry);
      break;
    case 'R' - 'A' + 1:
      if (selections)
        viewcom_resize(winentry);
      break;
    case 'D' - 'A' + 1:
      if (selections == 1)
        viewcom_coords(winentry);
      break;
    case 'F' - 'A' + 1:
      if (selections)
        viewcom_border(winentry);
      break;
    case 'P' - 'A' + 1:
      viewer_view(winentry,FALSE);
      break;
    /* Note: the following numbers are for ctrl+keypad.  */
    /* However, they also work without ctrl held down... */
    /* ... and for the main keyboard numbers...          */
    /* I haven't taken the time to work out the correct way to impliment this */
    case 50:
      viewcom_copy(winentry, copy_DOWN);
      break;
    case 54:
      viewcom_copy(winentry, copy_RIGHT);
      break;
    case 52:
      viewcom_copy(winentry, copy_LEFT);
      break;
    case 56:
      viewcom_copy(winentry, copy_UP);
      break;
    default:
      Wimp_ProcessKey(event->data.key.code);
      break;
  }
  return TRUE;
}

void viewer_responder(browser_winentry *winentry,
                      choices_str *old,choices_str *new_ch)
{
  window_state wstate;

  if (old->viewtools && !new_ch->viewtools)
  {
    viewtools_deletepane(winentry);
  }
  else if (!old->viewtools && new_ch->viewtools)
  {
    viewtools_newpane(winentry);
    Wimp_GetWindowState(winentry->handle,&wstate);
    viewtools_open(&wstate.openblock,winentry->pane);
  }
  if (old->hotkeys && !new_ch->hotkeys)
  {
    caret_block caret;

    Wimp_GetCaretPosition(&caret);
    if (caret.window == winentry->handle)
    {
      caret.window = -1;
      Wimp_SetCaretPosition(&caret);
    }
    viewer_createmenus();
  }
  else if (!old->hotkeys && new_ch->hotkeys)
    viewer_createmenus();
  if (old->hatchredraw != new_ch->hatchredraw)
  {
    window_redrawblock redraw;

    Wimp_GetWindowState(winentry->handle,&wstate);
    redraw.window = winentry->handle;
    redraw.rect = wstate.openblock.screenrect;
    Coord_RectToWorkArea(&redraw.rect,
                         (convert_block *) &wstate.openblock.screenrect);
    Wimp_ForceRedraw(&redraw);
  }
  if (old->furniture != new_ch->furniture)
    viewer_reopen(winentry,FALSE);
  if (old->borders != new_ch->borders)
  {
    int icon;

    for (icon = 0; icon < winentry->window->window.numicons; ++icon)
    {
      Wimp_SetIconState(winentry->handle, icon,
      	new_ch->borders ? icon_BORDER :
      		winentry->window->icon[icon].flags.value & icon_BORDER,
      	icon_BORDER);
    }
  }
}

void viewer_forceusrsprt(browser_winentry *winentry)
{
  winentry->window->window.spritearea = user_sprites;
  viewer_reopen(winentry, FALSE);
}

void viewer_setselect(browser_winentry *winentry,int icon,BOOL state)
{
  if (state)
    Wimp_SetIconState(winentry->handle,icon,(1<<21)|(1<<5),(1<<21)|(1<<5));
    	/* Selected and filled */
  else
    Wimp_SetIconState(winentry->handle,icon,
    	winentry->window->icon[icon].flags.data.filled << 5,
    	(1<<21)|(1<<5));
}

void viewer_changefonts(browser_winentry *winentry)
{
  int icon;
  window_info *winfo;
  window_handle behind;
  BOOL monitor_was_active;

  winfo = viewer_preparewinblock(winentry, &behind, &monitor_was_active,
  	winentry->window->window.titleflags.data.font);

  Font_LoseAllFonts(winentry->fontarray);

  /* Stop here if something went wrong */
  if (!winfo)
  {
    winentry->status = status_CLOSED;
    return;
  }

  /* Change icon fonts */
  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
    if (winentry->window->icon[icon].flags.data.font)
      ((icon_block *) &winfo[1])[icon].flags.font.handle =
      	icnedit_findfont(&winentry->browser->
      		fontinfo[winentry->window->icon[icon].flags.font.handle-1],
        &winentry->fontarray);
  }

  if (!viewer_rawreopen(winentry, winfo, behind, monitor_was_active))
  {
    winentry->status = status_CLOSED;
    free(winfo);
    return;
  }
  free(winfo);
  if (winentry->status == status_EDITING)
    viewer_claimeditevents(winentry);
  else
    viewer_claimpreviewevents(winentry);
}
