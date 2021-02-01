/* browser.c */
/* Handles browser windows and management of templat files */

#include <ctype.h>

#include "common.h"

#include "DeskLib:Str.h"
#include "DeskLib:Font.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:KeyCodes.h"
#include "DeskLib:Environment.h"
#include "DeskLib:Time.h"
#include "DeskLib:Filing.h"

#include "browser.h"
#include "browtools.h"
#include "choices.h"
#include "datatrans.h"
#include "drag.h"
#include "export.h"
#include "globals.h"
#include "icndiag.h"
#include "icnedit.h"
#include "monitor.h"
#include "stats.h"
#include "tempfont.h"
#include "title.h"
#include "unsaved.h"
#include "usersprt.h"
#include "viewer.h"
#include "windiag.h"
#include "ide.h"

/* Dimensions in browser */
#define DEFAULTNUMCOLUMNS 4
#define MAXNUMCOLUMNS browser_maxnumcolumns
#define MINNUMROWS 3
#define MARGIN 8
#define WIDTH 244
#define HEIGHT 44

/* Induce the Anni bug (define this, then load Anni template file) */
//#define REGRESSION_ANNI

/* Number of columns supported by current mode */
static int browser_maxnumcolumns;

/* Location and size of templat data for browser */
static window_block *browser_templat;
static char *browser_indirected_buffer;
static int browser_indirected_size;

/* Entry for default window */
static browser_winblock *browser_default;

/* Offset of window from centre */
static int browser_offset;

/* Sprite area containing icons */
sprite_area browser_sprites;

/* Validation strings for browser icons */
char browser_dialogvalid[]  = "Scdialogue";
char browser_odialogvalid[] = "Sodialogue";
char browser_windowvalid[]  = "Scwindow";
char browser_owindowvalid[] = "Sowindow";


/* Menus */
static menu_ptr browser_parentmenu = 0;
static menu_ptr browser_submenu = 0;
static BOOL browser_menuopen = FALSE;

/* Bodge to make right clicking on select all followed by choosing a subwindow off the menu, work */
static BOOL rightclickonselectall = FALSE;

/* Whether an icon can be selected with Menu click */
BOOL browser_selection_withmenu = TRUE;

typedef enum {
  parent_CREATE,
  parent_templat,
  parent_SELALL,
  parent_CLEAR,
  parent_SAVE,
  parent_STATS,
  submenu_COPY = 0,
  submenu_RENAME,
  submenu_DELETE,
  submenu_EDIT,
  submenu_PREVIEW,
  submenu_EXPORT
} browser_menuentries;



/* Tidy up before quitting (eg lose fonts) */
void browser_shutdown(void);

/* Open browser, optionally with pane */
BOOL browser_OpenWindow(event_pollblock *event,void *reference);

/* Delete window and associated file */
void browser_close(browser_fileinfo *browser);

/* Force browser to close whether unsaved or not */
void browser_forceclose(browser_fileinfo *browser);

/* Handle close events */
BOOL browser_closeevent(event_pollblock *event,void *reference);

/* Handle mouse events in browser */
BOOL browser_click(event_pollblock *event,void *reference);

/* Handle keyboard events in browser */
BOOL browser_hotkey(event_pollblock *event,void *reference);

/* Handle scroll events in browser */
BOOL browser_scrollevent(event_pollblock *event,void *reference);

/* Delete a window definition (doesn't remove icon from browser) */
void browser_delwindow(browser_winentry *winentry);

/* As above but does delete icon */
void browser_deletewindow(browser_winentry *winentry);

/* Create a new icon in a browser */
icon_handle browser_newicon(browser_fileinfo *browser,int index,
                            browser_winentry *winentry,int selected);

/* Load a file's data into a browser */
BOOL browser_getfile(char *filename,int filesize,browser_fileinfo *browser);

/* Merge a file into a browser */
BOOL browser_merge(char *filename,int filesize,void *reference);

/* Respond to load events (ie merges) in browser windows */
BOOL browser_loadhandler(event_pollblock *event,void *reference);

/* Respond to save events (merging from another app) */
BOOL browser_savehandler(event_pollblock *event,void *reference);

/* Respond to browser iconise events */
BOOL browser_iconise(event_pollblock *event,void *reference);

/* Check for unsaved data if PreQuit received */
BOOL browser_prequit(event_pollblock *event,void *reference);

/* Respond to browser menu selection */
BOOL browser_menuselect(event_pollblock *event,void *reference);

/* Delete menus & handlers when not needed */
BOOL browser_releasemenus(event_pollblock *event,void *reference);

/* Open appropriate dialogue boxes when submenu warning received */
BOOL browser_sublink(event_pollblock *event,void *reference);

/* Set number of columns after mode changes */
BOOL browser_modechange(event_pollblock *,void *);

/* Find named window in templat group (browser) */
browser_winentry *browser_findnamedentry(browser_fileinfo *browser,char *id);

/* Find next selected window *after* index (use -1 to start); max is max number
   of icons in browser (not necessarily == numwindows in middle of deletion
   sequence) */
browser_winentry *browser_findselection(browser_fileinfo *browser,
		 			int *index,int max);

/* Find winentry corresponding to icon number */
browser_winentry *browser_findwinentry(browser_fileinfo *browser,int icon);

/* Return FALSE if named window already exists and user doesn't want to
   overwrite it; if user does want existing entry overwritten this function
   does so. */
BOOL browser_overwrite(browser_fileinfo *browser,char *id);

/* Make menu tree and open parent */
void browser_makemenus(browser_fileinfo *browser,
                       int x,int y,icon_handle icon);

/* Create new winentry and copy data from windata */
browser_winentry *browser_copywindow(browser_fileinfo *browser,
		 		     char *identifier,
		 		     browser_winblock **windata);

/* When save is completed */
void browser_savecomplete(void *ref,BOOL successful,BOOL safe,
     			  char *filename,BOOL selection);
/* Make menus */
void browser_createmenus(void);

/* Put invisible caret in browser */
void browser_claimcaret(browser_fileinfo *browser);

/* Make changes in response to choices */
void browser_responder(choices_str *old,choices_str *new_ch);

/* Set a browser's extent based on number of icons and mode width */
void browser_setextent(browser_fileinfo *browser);

/* Change sprite area of window and all icons in a window_block */
void browser_changesparea(browser_winblock *win, void *sparea);

/* Exports icon names (from N validation) */
BOOL browser_export(char *filename, void *ref, BOOL selection);
void browser_exportcomplete(void *ref,BOOL successful,BOOL safe,char *filename,
     			  BOOL selection);

/* Create a sorted list of windows in a browser. */
browser_winentry **browser_sortwindows(browser_fileinfo *browser, BOOL sort);

/* Used as comparison function for qsort in alphabetsort */
int alphacomp(const void *one, const void *two);

/* Convert a string to lower case */
char *lower_case(char *s);

/* Overwrite-check window handlers */
BOOL overwrite_checkanswer(event_pollblock *event,void *reference);

/* Used to indicate whether a transient dbox popping up is causing a menu to
   close and hence dbox has to ignore the next message_MENUSDELETED */
/*BOOL menu_destroy = FALSE;*/

BOOL browser_userquit = FALSE;

/* Handles of mini dboxes */
static window_handle create_dbox;
static BOOL create_open = FALSE;
static window_handle copy_dbox;
static BOOL copy_open = FALSE;
static window_handle rename_dbox;
static BOOL rename_open = FALSE;

/* Overwrite warning box */
window_handle overwrite_warn;
BOOL overwrite_warn_open = FALSE;
/* Use globals to track items for browser_save */
char overwrite_check_filename[1024];
BOOL overwrite_check_selection;
browser_fileinfo *overwrite_check_browser;
BOOL overwrite_save_from_close = FALSE;

/* externs used in browser_iconise */
extern browser_winentry picker_winentry; /* Set up in picker.c */
extern BOOL monitor_isopen; /* Set up in monitor.c */
extern window_handle monitor_window; /* Set up in monitor.c */

/**
 * (Re-)Create the browser window menu structures from the messages.
 */
void browser_createmenus()
{
  char menutext[256];
  char subtitle[32];

  Log(log_DEBUG, "browser_createmenus");

  /* Main menu */
  if (browser_parentmenu)
    Menu_FullDispose(browser_parentmenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"BrowMenK",menutext,256);
  else
    MsgTrans_Lookup(messages,"BrowMen",menutext,256);
  browser_parentmenu = Menu_New(APPNAME,menutext);

  /* This line removed to stop shaded items getting opened - don't know why it's here?
  Menu_SetOpenShaded(browser_parentmenu,parent_templat); */

  /* Copy/delete submenu */
  if (browser_submenu)
    Menu_FullDispose(browser_submenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"BrowSubK",menutext,256);
  else
    MsgTrans_Lookup(messages,"BrowSub",menutext,256);

  MsgTrans_Lookup(messages,"BMSel",subtitle,32);
  browser_submenu = Menu_New(subtitle,menutext);

  /* Attach submenus */
  Menu_AddSubMenu(browser_parentmenu,parent_templat,browser_submenu);
}

/**
 * Process clicks on the Create button in the Create Window dialogue, or
 * Return keypresses.
 *
 * \param *event      The event data.
 * \param *reference  The parent browser instance into which to create the window.
 * \return            TRUE if the event was processed.
 */
static BOOL       create_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  Log(log_DEBUG, "create_clicked");

  if (event->type == event_KEY && event->data.key.code != 13)
  {
    Wimp_ProcessKey(event->data.key.code);
    return TRUE;
  }
  if (event->type == event_CLICK && event->data.mouse.button.data.menu)
    return FALSE;
  Icon_GetText(create_dbox, 2, buffer);
  if (!buffer[0])
  {
    WinEd_MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }

 if (browser_copywindow(browser,buffer,&browser_default))
  {
    browser_settitle(browser,NULL,TRUE);
    browser_sorticons(browser,TRUE,TRUE,FALSE);
  }

  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu((menu_ptr) -1,0,0);
  return TRUE;
}

static void       release_create()
{
  Log(log_DEBUG, "release_create");

  EventMsg_ReleaseWindow(create_dbox);
  Event_ReleaseWindow(create_dbox);
  help_release_window(create_dbox);
  create_open = FALSE;
}

static BOOL       release_create_msg(event_pollblock *e, void *r)
{
  Log(log_DEBUG, "release_create_msg");

  release_create();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_create_msg);
  return TRUE;
}

void              browcom_create(BOOL submenu, int x, int y,void *reference)
{
  Log(log_DEBUG, "browcom_create");

  if (create_open)
    release_create();
  if (!submenu)
  {
    mouse_block ptrinfo;
    Wimp_GetPointerInfo(&ptrinfo);
    x = ptrinfo.pos.x - 64;
    y = ptrinfo.pos.y + 64;
  }
  Icon_SetText(create_dbox, 2, "");
  if (submenu)
    Wimp_CreateSubMenu((menu_ptr) create_dbox, x, y);
  else
  {
    WinEd_CreateMenu((menu_ptr) create_dbox, x, y);
    EventMsg_Claim(message_MENUSDELETED, event_ANY, release_create_msg, 0);
  }
  Event_Claim(event_CLICK,create_dbox,0,create_clicked,reference);
  Event_Claim(event_KEY,create_dbox,2,create_clicked,reference);
  Event_Claim(event_CLICK,create_dbox,1,kill_menus,reference);
  Event_Claim(event_OPEN,create_dbox,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,create_dbox,event_ANY,kill_menus,0);
  help_claim_window(create_dbox, "CRE");
  create_open = TRUE;
}

static BOOL       copy_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  browser_winentry *winentry;
  int index = -1;

  Log(log_DEBUG, "copy_clicked");

  if (event->type == event_KEY && event->data.key.code != 13)
  {
    Wimp_ProcessKey(event->data.key.code);
    return TRUE;
  }
  if (event->type == event_CLICK && event->data.mouse.button.data.menu)
    return FALSE;
  Icon_GetText(copy_dbox, 2, buffer);
  if (!buffer[0])
  {
    WinEd_MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }

  winentry = browser_findselection(browser,&index,browser->numwindows);
  if (winentry == NULL)
    return TRUE;
  /* Ignore if giving it the same name */
  if (!strcmp(winentry->identifier,buffer))
    return TRUE;

  if (browser_copywindow(browser,buffer,&winentry->window))
  {
    browser_settitle(browser,NULL,TRUE);
    browser_sorticons(browser,TRUE,TRUE,FALSE);
  }

  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu((menu_ptr) -1,0,0);
  return TRUE;
}

static void       release_copy()
{
  Log(log_DEBUG, "release_copy");

  EventMsg_ReleaseWindow(copy_dbox);
  Event_ReleaseWindow(copy_dbox);
  help_release_window(copy_dbox);
  copy_open = FALSE;
}

static BOOL       release_copy_msg(event_pollblock *e, void *r)
{
  Log(log_DEBUG, "release_copy_msg");

  release_copy();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_copy_msg);
  return TRUE;
}

void              browcom_copy(BOOL submenu, int x, int y,void *reference)
{
  browser_winentry *winentry;
  Log(log_DEBUG, "browcom_copy");

  int index = -1;
  browser_fileinfo *browser = reference;
  if (copy_open)
    release_copy();
  if (!submenu)
  {
    mouse_block ptrinfo;
    Wimp_GetPointerInfo(&ptrinfo);
    x = ptrinfo.pos.x - 64;
    y = ptrinfo.pos.y + 64;
  }
  winentry = browser_findselection(browser,&index,browser->numwindows);
  if (winentry == NULL)
    return;

  Icon_SetText(copy_dbox, 2, winentry->identifier);
  if (submenu)
    Wimp_CreateSubMenu((menu_ptr) copy_dbox, x, y);
  else
  {
    WinEd_CreateMenu((menu_ptr) copy_dbox, x, y);
    EventMsg_Claim(message_MENUSDELETED, event_ANY, release_copy_msg, 0);
  }
  Event_Claim(event_CLICK,copy_dbox,0,copy_clicked,reference);
  Event_Claim(event_KEY,copy_dbox,2,copy_clicked,reference);
  Event_Claim(event_CLICK,copy_dbox,1,kill_menus,reference);
  Event_Claim(event_OPEN,copy_dbox,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,copy_dbox,event_ANY,kill_menus,0);
  help_claim_window(copy_dbox, "COP");
  copy_open = TRUE;
}

static BOOL       rename_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  int index;
  browser_winentry *winentry;

  Log(log_DEBUG, "rename_clicked");

  if (event->type == event_KEY && event->data.key.code != 13)
  {
    Wimp_ProcessKey(event->data.key.code);
    return TRUE;
  }
  if (event->type == event_CLICK && event->data.mouse.button.data.menu)
    return FALSE;
  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu((menu_ptr) -1,0,0);
  Icon_GetText(rename_dbox, 2, buffer);
  if (!buffer[0])
  {
    WinEd_MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }
  if (!browser_overwrite(browser,buffer))
    return TRUE;
  index = -1;
  winentry = browser_findselection(browser,&index,browser->numwindows);
  if (winentry == NULL)
    return TRUE;

  strncpy(winentry->identifier, buffer, sizeof(winentry->identifier) - 1);
  winentry->identifier[sizeof(winentry->identifier) - 1] = '\0';

  browser_settitle(browser,NULL,TRUE);
  browser_sorticons(browser,TRUE,TRUE,FALSE);
  Icon_ForceWindowRedraw(browser->window,index);
  if (monitor_isactive && monitor_winentry == winentry)
  {
    monitor_deactivate(NULL,winentry);
    monitor_activate(NULL,winentry);
  }
  return TRUE;
}

static void       release_rename()
{
  Log(log_DEBUG, "release_rename");

  EventMsg_ReleaseWindow(rename_dbox);
  Event_ReleaseWindow(rename_dbox);
  help_release_window(rename_dbox);
  rename_open = FALSE;
}

static BOOL       release_rename_msg(event_pollblock *e, void *r)
{
  Log(log_DEBUG, "release_rename_msg");

  release_rename();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_rename_msg);
  return TRUE;
}

void              browcom_rename(BOOL submenu, int x, int y,void *reference)
{
  int index = -1;
  browser_winentry *winentry;
  browser_fileinfo *browser = reference;
  Log(log_DEBUG, "browcom_rename");

  if (rename_open)
    release_rename();
  if (!submenu)
  {
    mouse_block ptrinfo;
    Wimp_GetPointerInfo(&ptrinfo);
    x = ptrinfo.pos.x - 64;
    y = ptrinfo.pos.y + 64;
  }

  winentry = browser_findselection(browser, &index,browser->numwindows);
  if (winentry == NULL)
    return;

  Icon_SetText(rename_dbox, 2, winentry->identifier);

  if (submenu)
    Wimp_CreateSubMenu((menu_ptr) rename_dbox, x, y);
  else
  {
    WinEd_CreateMenu((menu_ptr) rename_dbox, x, y);
    EventMsg_Claim(message_MENUSDELETED, event_ANY, release_rename_msg, 0);
  }
  Event_Claim(event_CLICK,rename_dbox,0,rename_clicked,reference);
  Event_Claim(event_KEY,rename_dbox,2,rename_clicked,reference);
  Event_Claim(event_CLICK,rename_dbox,1,kill_menus,reference);
  Event_Claim(event_OPEN,rename_dbox,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,rename_dbox,event_ANY,kill_menus,0);
  help_claim_window(rename_dbox, "REN");
  rename_open = TRUE;
}

void              create_minidboxes()
{
  window_block *templat;

  Log(log_DEBUG, "create_minidboxes");

  templat = templates_load("Create",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&create_dbox));
  free(templat);

  templat = templates_load("Copy",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&copy_dbox));
  free(templat);

  templat = templates_load("Rename",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&rename_dbox));
  free(templat);

  templat = templates_load("Overwrite",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&overwrite_warn));
  free(templat);
}

/* Convert all string terminators to CR or NULL */
static void       browser_cr(browser_fileinfo *browser)
{
  browser_winentry *winentry;

  Log(log_DEBUG, "browser_cr");

  for (winentry = LinkList_NextItem(&browser->winlist); winentry;
       winentry = LinkList_NextItem(winentry))
  {
    int icon;
    int n;

    /* Note only bother with non-indirected icons because
       indirected string pool can be processed in one go */
    /* Check title */
    if (!winentry->window->window.titleflags.data.indirected)
    {
      int i;

      for (i = 0;  i < wimp_MAXNAME && winentry->window->window.title.text[i] > 31;  ++i);
        if (i < wimp_MAXNAME)
          winentry->window->window.title.text[i] = choices->formed ? 13 : 0;
    }
    /* Check each icon */
    for (icon = 0; icon < winentry->window->window.numicons; ++icon)
    {
      if (!winentry->window->icon[icon].flags.data.indirected)
      {
        int i;

        for (i = 0;  i < wimp_MAXNAME && winentry->window->icon[icon].data.text[i] > 31;  ++i);
          if (i < wimp_MAXNAME)
            winentry->window->icon[icon].data.text[i] = choices->formed ? 13 : 0;
      }
    }
    /* Indirected string pool */
    for (n = sizeof(window_block) +
    		winentry->window->window.numicons * sizeof(icon_block);
    	n < flex_size((flex_ptr) &winentry->window);
    	++n)
    {
      if (((char *) winentry->window)[n] < 32)
        ((char *) winentry->window)[n] = choices->formed ? 13 : 0;
    }
  }
  /* Font names */
  if (browser->fontinfo)
  {
    int f;

    for (f = 0;
    	f * sizeof(template_fontinfo) <
    		flex_size((flex_ptr) &browser->fontinfo);
    	++f)
    {
      int i;

      for (i = 0; i < 40 && browser->fontinfo[f].name[i] > 31; ++i);
      if (i < 40)
        browser->fontinfo[f].name[i] = choices->formed ? 13 : 0;
    }
  }
}

/**
 * Save a file to an open file handle.
 * 
 * \param *filename   The filename of the file, used for messages.
 * \param *browser    The browser instance to be saved.
 * \param sorted      TRUE if the templates should be sorted alphabetically in the file.
 * \param selection   TRUE if only the selected templates should be saved.
 * \param fp          The file handle to write the data to.
 * \return            TRUE if successful; FALSE on failure.
 */
static BOOL browser_dosave(char *filename, browser_fileinfo *browser, BOOL sorted, BOOL selection, file_handle fp)
{
  template_header header;
  browser_winentry **winentries, *winentry;
  int index, offset, windowcount = 0;
  #ifdef DeskLib_DEBUG
  int corrupt_type = 0;
  #endif

  Log(log_DEBUG, "browser_dosave - saving to %s", filename);

  #ifdef DeskLib_DEBUG
    /* Only provide functionality to output dodgy test files if we're debugging */
    if (!strncmp("~WinEdCorrupt~", Filing_FindLeafname(filename), 14))
      /* Find type of file to generate, based on final digit */
      corrupt_type = atoi(&filename[strlen(filename) - 1]);
      /*
          1: Corrupt header          (header magic numbers not magic)
          2: (Not used)
          3: Corrupt fonts           (font offset larger than file)
          4: Exotic window           (first window has type = -99)
      */
  #endif

  browser_cr(browser);

  winentries = browser_sortwindows(browser, sorted);
  if (winentries == NULL)
  {
    WinEd_MsgTrans_ReportPS(messages, "CantSave", FALSE, filename, 0, 0, 0);
    return FALSE;
  }

  /* Save header */
  if (browser->fontinfo)
  {
    /* Calculate eventual offset of font data */
    offset = sizeof(template_header);
    for (index = 0; index < browser->numwindows; index++)
    {
      winentry = winentries[index];

      if (!selection || Icon_GetSelect(browser->window, winentry->icon))
        offset += sizeof(template_index) + flex_size((flex_ptr) &winentry->window);
    }
    header.fontoffset = offset + 4; /* Skip terminating zero */

    #ifdef DeskLib_DEBUG
      if (corrupt_type == 3)
      {
        Log(log_NOTICE, "Corrupting font data!");
        header.fontoffset += 1000000;
      }
    #endif
  }
  else
    header.fontoffset = -1;

  header.dummy[0] = header.dummy[1] = header.dummy[2] = 0;

  #ifdef DeskLib_DEBUG
    if (corrupt_type == 1)
    {
      Log(log_NOTICE, "Corrupting header magic number!");
      header.dummy[1] = -99;
    }
  #endif

  if (File_WriteBytes(fp,&header,sizeof(template_header)))
  {
    if (file_lasterror)
      Error_Check(file_lasterror);
    else
      WinEd_MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
    free(winentries);
    return FALSE;
  }

  Log(log_DEBUG, " header written");

  /* Work out eventual offset for start off window data */
  offset = sizeof(template_header);
  for (index = 0; index < browser->numwindows; index++)
  {
    winentry = winentries[index];

    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
      offset += sizeof(template_index);
  }
  offset += 4; /* Skip terminating zero word */

  /* Save index entry for each window */
  for (index = 0; index < browser->numwindows; index++)
  {
    winentry = winentries[index];

    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
    {
      template_index index;
      int i;

      windowcount++;

      index.offset = offset;
      index.size = flex_size((flex_ptr) &winentry->window);
      index.type = 1;

      #ifdef DeskLib_DEBUG
        if ((corrupt_type == 4) && (windowcount == 1))
        {
          Log(log_NOTICE, "Corrupting object type!");
          index.type = -99;
        }
      #endif

      /* Don't use strcpycr, we need to choose between terminators */
      for (i = 0; i < 12; i++) /* Titles can only be 11 chars long + terminator */
      {
        index.identifier[i] = winentry->identifier[i];
        if (index.identifier[i] < 32)
        {
          index.identifier[i] = choices->formed ? 13 : 0;
          break;
        }
      }

      if (File_WriteBytes(fp,&index,sizeof(template_index)))
      {
        if (file_lasterror)
          Error_Check(file_lasterror);
        else
          WinEd_MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        free(winentries);
        return FALSE;
      }
      offset += index.size;
    }
  }

  /* Save index-terminating zero */
  if (Error_Check(File_Write32(fp,0)))
  {
    free(winentries);
    return FALSE;
  }

  Log(log_DEBUG, " index written");

  /* Save actual data for each window */
  windowcount = 0;
  for (index = 0; index < browser->numwindows; index++)
  {
    winentry = winentries[index];
    windowcount++;

    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
    {
      browser_changesparea(winentry->window, (void *) 1);
      if (File_WriteBytes(fp,winentry->window,
                          flex_size((flex_ptr) &winentry->window)))
      {
        if (file_lasterror)
          Error_Check(file_lasterror);
        else
          WinEd_MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        browser_changesparea(winentry->window, user_sprites);
        free(winentries);
        return FALSE;
      }
      browser_changesparea(winentry->window, user_sprites);
    }
  }

  /* Save font data */
  if (browser->fontinfo)
  {
    if (File_WriteBytes(fp,browser->fontinfo,
                          flex_size((flex_ptr) &browser->fontinfo)))
      {
        if (file_lasterror)
          Error_Check(file_lasterror);
        else
          WinEd_MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        free(winentries);
        return FALSE;
      }
  }

  free(winentries);

  Log(log_DEBUG, "End of do_save");

  return TRUE;
}

/* Saves all altered files to scrap if it can */
static void       browser_safetynet()
{
  BOOL reported = FALSE;
  browser_fileinfo *browser;
  Log(log_DEBUG, "browser_safetynet");

  if (browser_userquit)
    return;
  browser_userquit = TRUE;
  for (browser = LinkList_NextItem(&browser_list);
       browser;
       browser = LinkList_NextItem(browser))
  {
    if (browser->altered)
    {
      file_handle fp;
      strcpy(browser->title,"<Wimp$ScrapDir>.WinEd");
      if (!reported)
      {
        char buffer[50];
        WinEd_MsgTrans_Report(messages,"Safety",FALSE);
        Log(log_ERROR, "Unsaved templates present - saving in <Wimp$ScrapDir>.WinEd");
        Wimp_CommandWindow(-1);
        /* Create directory */
        if (SWI(5,0,SWI_OS_File,8,browser->title,0,0,0))
          return;
        snprintf(buffer, sizeof(buffer), "Filer_OpenDir %s", browser->title);
        OS_CLI(buffer);
        reported = TRUE;
      }
      strcat(browser->title,".");
      strcat(browser->title,strrchr(tmpnam(NULL),'.')+1);
      fp = File_Open(browser->title,file_WRITE);
      if (fp)
      {
        browser_dosave(browser->title, browser, FALSE, FALSE, fp);
        browser->altered = FALSE;
        File_Close(fp);
        File_SetType(browser->title, filetype_TEMPLATE);
      }
    }
  }
}

void              browser_init(void)
{
  template_block templat;
  char filename[64] = "WinEdRes:Sprites22";
  char *tempind = NULL;
  int indsize;

  Log(log_DEBUG, "browser_init");

  atexit(browser_safetynet);
  Screen_CacheModeInfo();
  browser_maxnumcolumns = (screen_size.x-MARGIN-32) / (WIDTH + MARGIN);

  browser_sprites = Sprite_LoadFile(filename);
  if (!browser_sprites)
  {
    Log(log_WARNING, " Problem loading %s", filename);
  }

  LinkList_Init(&browser_list);
  browser_templat = templates_load("Browser",0,0,&browser_indirected_buffer,
                                    &browser_indirected_size);
  browser_templat->spritearea = browser_sprites;

  /* Load default window */
  templat.buffer = (window_block *) -1;
  templat.workfree = (char *) -1;
  templat.font = (font_array *) -1;
  templat.name = "Default\0\0\0\0\0";
  templat.index = 0;
  if (Error_Check(Wimp_LoadTemplate(&templat)))
  {
    Wimp_CloseTemplate();
    exit(0);
  }
  if (!templat.index)
  {
    Wimp_CloseTemplate();
    WinEd_MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
  }
  if (!flex_alloc((flex_ptr) &browser_default,
      		 (int) templat.buffer))
  {
    Wimp_CloseTemplate();
    WinEd_MsgTrans_ReportPS(messages,"TempMem",TRUE,"Default",0,0,0);
  }
  /* Create temporary buffer for indirected data */
  indsize = (int) templat.workfree;
  if (templat.workfree)
  {
    tempind = malloc(indsize);
    if (!tempind)
    {
      Wimp_CloseTemplate();
      WinEd_MsgTrans_ReportPS(messages,"TempMem",TRUE,"Default",0,0,0);
    }
    templat.workfree = tempind;
    templat.workend = tempind + indsize;
  }
  else
  {
    templat.workfree = 0;
    templat.workend = 0;
  }
  templat.buffer = &browser_default->window;
  templat.index = 0;
  if (Error_Check(Wimp_LoadTemplate(&templat)))
  {
    Wimp_CloseTemplate();
    exit(0);
  }
  if (!templat.index)
  {
    Wimp_CloseTemplate();
    WinEd_MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
  }

  /* If title is indirected, 'compress' data and
     change pointers to offset from start of entry */
  if (browser_default->window.titleflags.data.indirected)
  {
    char *copyto;
    int copysize;

    if ((int) browser_default->window.title.indirecttext.buffer > 0)
    {
      copysize = flex_size((flex_ptr) &browser_default);
      if (!flex_midextend((flex_ptr) &browser_default,
      	  		 copysize,
                         strlencr(browser_default->
                         	  window.title.indirecttext.buffer)))
      {
        Wimp_CloseTemplate();
        WinEd_MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
      }
      copyto = (char *) browser_default + copysize;
      strcpycr(copyto,browser_default->window.title.indirecttext.buffer);
      browser_default->window.title.indirecttext.buffer = copyto -
        (int) browser_default;
    }
    if (browser_default->window.titleflags.data.text &&
    	(int) browser_default->window.title.indirecttext.validstring > 0)
    {
      copysize = flex_size((flex_ptr) &browser_default);
      if (!flex_midextend((flex_ptr) &browser_default,
      	  		 copysize,
                         strlencr(browser_default->
                         	  window.title.indirecttext.buffer)))
      {
        Wimp_CloseTemplate();
        WinEd_MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
      }
      copyto = (char *) browser_default + copysize;
      strcpycr(copyto,browser_default->window.title.indirecttext.buffer);
      browser_default->window.title.indirecttext.validstring = copyto -
        (int) browser_default;
    }
  }

  if (tempind)
    free(tempind);

  choices_init(browser_responder);
  browtools_init();
  unsaved_init();
  viewer_init();
  stats_init();

  browser_offset = 256;

  create_minidboxes();

  /* Make name submenu */
  /*
  MsgTrans_Lookup(messages,"BrowLeaf",filename,32);
  browser_leafmenu = Menu_New(filename,"12345678901");
  Menu_MakeWritable(browser_leafmenu,0,
                    browser_leafdata,12,(char *) -1);
  *browser_leafdata = 0;
  */

  browser_createmenus();
  atexit(browser_shutdown);

  EventMsg_Claim(message_PREQUIT,event_ANY,browser_prequit,0);
  EventMsg_Claim(message_MODECHANGE,event_ANY,browser_modechange,0);

}

browser_fileinfo *browser_newbrowser()
{
  browser_fileinfo *newfile;
  window_state wstate;
  int index;
/*
  int width,height;
*/
  Log(log_INFORMATION, "browser_newbrowser - creating new browser");

  /* Create new info block */
  newfile = malloc(sizeof(browser_fileinfo));
  if (!newfile)
  {
    WinEd_MsgTrans_Report(messages,"BrowMem",FALSE);
    return NULL;
  }
  newfile->fontinfo = NULL;
  newfile->numfonts = 0;
  for (index = 1;index < 256;index++)
    newfile->fontcount[index] = 0;
  newfile->numwindows = 0;
  newfile->numcolumns = DEFAULTNUMCOLUMNS;
  newfile->altered = FALSE;
  newfile->iconised = FALSE;
  newfile->stats = 0;
  newfile->idetask = 0; /* 0 used to indicate no registered IDE app */
  LinkList_AddToTail(&browser_list,&newfile->header);
  LinkList_Init(&newfile->winlist);

  /* Create new browser window */
  MsgTrans_Lookup(messages,"Untitled",newfile->title,256);
  MsgTrans_Lookup(messages,"ExportLeaf",newfile->namesfile,256);
  browser_templat->title.indirecttext.buffer = newfile->title;
  browser_templat->workarearect.min.x = 0;
  browser_templat->workarearect.min.y = - MARGIN - MINNUMROWS * (MARGIN + HEIGHT);
  browser_templat->workarearect.max.x = MARGIN + MAXNUMCOLUMNS * (MARGIN + WIDTH);
  browser_templat->workarearect.max.y = 0;
  if (choices->browtools)
    browser_templat->workarearect.min.y -= browtools_paneheight;
  if (Error_Check(Wimp_CreateWindow(browser_templat,&newfile->window)))
  {
    free(newfile);
    return 0;
  }

  /* Handlers, using browser_fileinfo block as reference. */
  /* These are all released in browser_close() */
  Event_Claim(event_OPEN,   newfile->window, event_ANY, browser_OpenWindow,  newfile);
  Event_Claim(event_CLOSE,  newfile->window, event_ANY, browser_closeevent,  newfile);
  Event_Claim(event_CLICK,  newfile->window, event_ANY, browser_click,       newfile);
  Event_Claim(event_KEY,    newfile->window, event_ANY, browser_hotkey,      newfile);
  Event_Claim(event_SCROLL, newfile->window, event_ANY, browser_scrollevent, newfile);
  EventMsg_Claim(message_DATALOAD,              newfile->window, browser_loadhandler, newfile);
  EventMsg_Claim(message_DATASAVE,              newfile->window, browser_savehandler, newfile);
  EventMsg_Claim(message_WINDOWINFO,            newfile->window, browser_iconise,     newfile);
  help_claim_window(newfile->window,"Brow");

  /* Create tool pane if wanted */
  if (choices->browtools)
    browtools_newpane(newfile);

  /* Open window, centred with offset */
  Wimp_GetWindowState(newfile->window,&wstate);
/*
  width = wstate.openblock.screenrect.max.x -
          wstate.openblock.screenrect.min.x;
  height = wstate.openblock.screenrect.max.y -
           wstate.openblock.screenrect.min.y;
  Screen_CacheModeInfo();
  wstate.openblock.screenrect.min.x = (screen_size.x - width) / 2;
  wstate.openblock.screenrect.min.y = (screen_size.y - height) / 2 +
                                      browser_offset;
  wstate.openblock.screenrect.max.x = wstate.openblock.screenrect.min.x +
                                      width;
  wstate.openblock.screenrect.max.y = wstate.openblock.screenrect.min.y +
                                      height;
*/
  wstate.openblock.screenrect.max.x = wstate.openblock.screenrect.min.x +
  	MARGIN + DEFAULTNUMCOLUMNS * (MARGIN + WIDTH);
  wstate.openblock.screenrect.max.y += browser_offset;
  wstate.openblock.screenrect.min.y = wstate.openblock.screenrect.max.y +
  	browser_templat->workarearect.min.y;
  if (choices->browtools)
  {
    browtools_shadenosel(newfile->pane);
    browtools_open(&wstate.openblock,newfile,TRUE);
  }
  else
    Wimp_OpenWindow(&wstate.openblock);
  if (choices->hotkeys)
    browser_claimcaret(newfile);
  browser_offset -= 64;
  if (browser_offset < -256)
    browser_offset = 256;

  return newfile;
}

BOOL              browser_OpenWindow(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  BOOL opening_behind = FALSE;
  Log(log_DEBUG, "browser_OpenWindow");
  Log(log_DEBUG, " iconised:%d behind:%d", browser->iconised, event->data.openblock.behind);

  if (event->data.openblock.behind < -1)
    opening_behind = TRUE;

  int ncols = (event->data.openblock.screenrect.max.x -
  	event->data.openblock.screenrect.min.x) / (WIDTH + MARGIN);
  if (ncols != browser->numcolumns)
  {
    browser->numcolumns = ncols;
    browser_sorticons(browser,TRUE,FALSE,TRUE);
  }
  if (choices->browtools)
    browtools_open(&event->data.openblock,browser,TRUE);
  else
    Wimp_OpenWindow(&event->data.openblock);

  /* Only worry about re-opening viewers if the browser was previously iconised and
     is asked to be displayed visibly (e.g. not iconised or behind window stack) */
  Log(log_DEBUG, " iconised:%d behind:%d", browser->iconised, event->data.openblock.behind);
  if (browser->iconised && (!opening_behind))
  {
    browser_winentry *winentry;
    /* Open viewers, which were open before */
    winentry = LinkList_NextItem(&browser->winlist);
    while (winentry)
    {
      if (!winentry->iconised) /* Only open viewer automatically if it isn't already iconised */
      {
        if (winentry->status) Window_Show(winentry->handle, open_WHEREVER);
        if (choices->viewtools) Window_Show(winentry->pane, open_WHEREVER);
      }
      winentry = LinkList_NextItem(winentry);
    }

    /* Re-open monitor and picker, if they were open */
    if (monitor_isopen)
      Window_Show(monitor_window, open_WHEREVER);
    if (picker_winentry.status != status_CLOSED)
      Window_Show(picker_winentry.handle, open_WHEREVER);

    /* Reset iconised flag */
    browser->iconised = FALSE;
  }

  return TRUE;
}

BOOL              browser_iconise(event_pollblock *event,void *reference)
{
  message_block message = event->data.message;
  char *leaf;
  browser_fileinfo *browser = reference;
  browser_winentry *winentry;

  Log(log_DEBUG, "browser_iconise");

  /* Ignore iconise messages for other windows */
  if (event->data.message.data.windowinfo.window != browser->window)
    return FALSE;

  message.header.size = sizeof(message_header) + sizeof(message_windowinfo);
  /* message.header.sender = 0; */
  message.header.yourref = message.header.myref;
  /* message.header.myref = 0;
  message.header.action = message_WINDOWINFO; */

  strcpy(message.data.windowinfo.spritename,"temp");
  if (leaf = strrchr(browser->title,'.'),leaf)
    leaf++;
  else
    leaf = browser->title;
  strncpy(message.data.windowinfo.title,leaf,20);
  if (leaf[strlen(leaf) - 1] == '*')
    message.data.windowinfo.title[strlen(leaf) - 2] = 0;
  message.data.windowinfo.title[19] = 0;
  Error_Check(Wimp_SendMessage(event_SEND,&message, event->data.message.header.sender,0));

  browser->iconised = TRUE;

  /* Hide (not close) all 'children' */
  winentry = LinkList_NextItem(&browser->winlist);
  while (winentry)
  {
    if (winentry->status)
    {
      Window_Hide(winentry->handle);
      /* Close (permanently) all viewer dialogues */
      viewer_closechildren(winentry);
      /* Close (permanently) tool pane */
      if (choices->viewtools)
        Window_Hide(winentry->pane);
    }
    winentry = LinkList_NextItem(winentry);
  }
  stats_close(browser);

  /* Close monitor and picker, if open */
  if (monitor_isopen)
    Window_Hide(monitor_window);
  if (picker_winentry.status != status_CLOSED)
    Window_Hide(picker_winentry.handle);

  return TRUE;
}

void              browser_close(browser_fileinfo *browser)
{
  browser_winentry *entry;

  Log(log_DEBUG, "browser_close");

  /* Check for unsaved data */
  if (browser->altered)
  {
    overwrite_save_from_close = TRUE;
    unsaved_close(browser_forceclose,browser_save_check,browser);
    return;
  }

  if (selection_browser == browser)
  {
    selection_browser = NULL;
    selection_viewer = NULL;
    browser_selection_withmenu = TRUE;
  }

  /* Get rid of stats */
  stats_close(browser);

  /* Free data */
  entry = LinkList_NextItem(&browser->winlist);
  while (entry)
  {
    browser_winentry *next_entry;

    next_entry = LinkList_NextItem(&entry->header);
    browser_deletewindow(entry);
    entry = next_entry;
  }

  Wimp_DeleteWindow(browser->window);

  if (choices->browtools)
  {
    browtools_deletepane(browser);
  }

  /* Release handlers */
  EventMsg_ReleaseWindow(browser->window);
  Event_ReleaseWindow(browser->window);
  EventMsg_ReleaseWindow(browser->window); /* includes help */

  LinkList_Unlink(&browser_list,&browser->header);
  free(browser);
}

void              browser_forceclose(browser_fileinfo *browser)
{
  Log(log_DEBUG, "browser_forceclose");

  /* Reset the global variable which tracks if the user clicked on the close icon */
  overwrite_save_from_close = FALSE;

  browser->altered = FALSE;
  browser_close(browser);
}

/**
 * Force close all of the broswer windows, including unsaved ones.
 * This is intended for scenarios where the user has OK'd the
 * destruction of all files, such as approved desktop exit.
 */
void browser_forcecloseall()
{
  browser_fileinfo *browser = LinkList_FirstItem(&browser_list);

  Log(log_DEBUG, "browser_forcecloseall");

  while (browser)
  {
    browser->altered = FALSE;
    browser_close(browser);

    browser = LinkList_FirstItem(&browser_list);
  }
}

BOOL              browser_closeevent(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  browser_fileinfo *browser = reference;
  BOOL shift;

  Log(log_DEBUG, "browser_closeevent");

  /* If Adjust, open parent directory */
  Wimp_GetPointerInfo(&ptrinfo);
  shift = Kbd_KeyDown(inkey_SHIFT);
  if (ptrinfo.button.data.adjust)
  {
    char *lastdot;
    char opendir[272] = "Filer_OpenDir ";

    strcpycr(opendir + 14,browser->title);
    lastdot = strrchr(opendir,'.');
    if (lastdot)
    {
      *lastdot = 0;
      Error_Check(Wimp_StartTask(opendir));
    }

    /* Keep window open if Shift held */
    if (shift)
      return TRUE;
  }

  browser_close(browser);
  return TRUE;
}

BOOL              browser_click(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  BOOL tools;

  Log(log_DEBUG, "browser_click");

  /* Note click/double events are transposed */
  switch (event->data.mouse.button.value)
  {
    case button_CLICKSELECT:
    case button_CLICKADJUST:
      tools = choices->browtools;
      choices->browtools = FALSE;
      if (((event->data.mouse.button.value == button_CLICKSELECT) &&
      	(!Icon_GetSelect(event->data.mouse.window,event->data.mouse.icon))) || (browser != selection_browser))
        browcom_clearsel();
      choices->browtools = tools;
      if (event->data.mouse.icon != -1)
      {
        Icon_SetSelect(event->data.mouse.window,event->data.mouse.icon,
        	event->data.mouse.button.value == button_CLICKSELECT ?
        	TRUE : !Icon_GetSelect(event->data.mouse.window,
                                       event->data.mouse.icon));
        selection_browser = browser;
        browser_selection_withmenu =
        	(count_selections(event->data.mouse.window) == 0);
      }
      if (choices->hotkeys)
        browser_claimcaret(browser);
      break;
    case button_SELECT:
    case button_ADJUST:
      if (event->data.mouse.icon != -1)
      {
        browser_winentry *winentry =
          browser_findwinentry(browser,event->data.mouse.icon);

        if (winentry->status)
        {
          viewer_close(winentry);
          /* Refresh browser to reflect closed state of viewer */
          browser_sorticons(winentry->browser,TRUE,FALSE,TRUE);
        }
        else
          viewer_open(winentry,event->data.mouse.button.data.select);
      }
      break;
    case button_MENU:
      if (choices->hotkeys)
        browser_claimcaret(browser);
      browser_makemenus(browser,
                        event->data.mouse.pos.x,event->data.mouse.pos.y,
                        event->data.mouse.icon);
      break;
    case button_DRAGADJUST:
      drag_rubberbox(event,browser,FALSE);
      break;
    case button_DRAGSELECT:
      if (event->data.mouse.icon == -1 || Kbd_KeyDown(inkey_CTRL))
        drag_rubberbox(event,browser,FALSE);
      else
        datatrans_dragndrop(browser_save_check, browser_savecomplete, browser);
      break;
  }
  if (choices->browtools)
    browtools_shadeapp(browser);
  return TRUE;
}

BOOL              browser_loadhandler(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "browser_loadhandler");

  if (event->data.message.data.dataload.filetype == filetype_TEMPLATE)
  {
    datatrans_load(event,browser_merge,reference);
    return TRUE;
  }
  return FALSE;
}

BOOL              browser_savehandler(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "browser_savehandler");

  if (event->data.message.data.datasave.filetype == filetype_TEMPLATE)
  {
    datatrans_saveack(event);
    return TRUE;
  }
  return FALSE;
}

BOOL              browser_load(char *filename,int filesize,void *reference)
{
  browser_fileinfo *browser;
  BOOL returnvalue;

  Log(log_INFORMATION, "browser_load - loading template %s", filename);

  browser = browser_newbrowser();

  if (browser_getfile(filename,filesize,browser))
  {
    browser_sorticons(browser,FALSE,TRUE,FALSE);
    if (strcmp(filename,"<Wimp$Scrap>"))
      browser_settitle(browser,filename,FALSE);
    else
    {
      char buffer[32];

      MsgTrans_Lookup(messages,"Untitled",buffer,32);
      browser_settitle(browser,buffer,TRUE);
    }
    if (choices->autosprites)
    {
      char spritefile[256];
      char *leaf;
      int size;

      Log(log_DEBUG, " in browser_load... autoloading sprites");
      strcpy(spritefile, filename);
      leaf = strrchr(spritefile,'.');
      Log(log_DEBUG, " leaf='%s'", leaf);
      if (leaf)
      {
        strcpy(leaf+1,"Sprites");
        usersprt_modfilename(spritefile);
        Log(log_DEBUG, " Trying '%s'", spritefile);
        if (!File_Exists(spritefile))
        {
          /* Try next level up in directory structure */
          /* (e.g. sprites are in Resources.Sprites and Templates are in Resources.UK.Templates */
          strcpy(leaf, "");
          leaf = strrchr(spritefile,'.');
          if (leaf)
          {
            strcpy(leaf+1,"Sprites");
            usersprt_modfilename(spritefile);
            Log(log_DEBUG, " Trying '%s'", spritefile);
          }
        }
        size = File_Size(spritefile);
        if (size > 0)
        {
          Log(log_INFORMATION, " Sprites found.");
          usersprt_merge(spritefile,size,0);
        }
        else;
        {
          Log(log_INFORMATION, " No sprites found");
        }
      }
    }
    returnvalue = TRUE;
  }
  else
  {
    browser_close(browser);
    returnvalue = FALSE;
  }

  if (returnvalue)
    Log(log_INFORMATION, "Templates loaded OK");
  else
    Log(log_ERROR, "Problem loading templates");

  return returnvalue;
}

BOOL              browser_merge(char *filename,int filesize,void *reference)
{
  browser_fileinfo *browser = reference;

  Log(log_DEBUG, "browser_merge");

  if (browser_getfile(filename,filesize,browser))
  {
    browser_sorticons(browser,TRUE,TRUE,FALSE);
    browser_settitle(browser,0,TRUE);
    return TRUE;
  }
  return FALSE;
}

typedef enum {
  load_OK = 0,
  load_MemoryError,     /* Stop loading file, maybe discard completely */
  load_FileError,       /* Some significant file structure error - stop loading file, maybe discard completely */
  load_FatalError,      /* Bad thing. Corrupt file header, font data, indirected data etc. Abandon whole file. */
  load_IconError,       /* Error in icon definitions, skip window */
  load_DefinitionError, /* Error in window structure, skip window */
  load_Finished
} load_result;

/* Lengths exclude terminators */
typedef struct {
  int buf;
  int buflen;
  int valid;
  int validlen;
} ind_table;


/* icon = real icon # +1, 0 = title because of indtable indexing */
static BOOL       browser_resolve_shared(browser_winentry *winentry,
                                   int *string, ind_table *indtable,
                                   int icon, BOOL valid)
{
  int i;
  char *str;

  if (*string <= 0)
    return TRUE;
  str = (char *) winentry->window + *string;


  /* Note <= in conditon because indexes in indtable are +1 with title at 0 */
  for (i = 0; i <= winentry->window->window.numicons; ++i)
  {
    if (i == icon)
    {
      if ((valid && (*string >= indtable[i].buf)
                && (*string <= (indtable[i].buf + indtable[i].buflen)))
           || ((!valid) && (*string >= indtable[i].valid)
                     && (*string <= (indtable[i].valid + indtable[i].validlen))))
      {
        *string = icnedit_addindirectstring(winentry, str);
        if (*string <= 0)
          return FALSE;
        break;
      }
    }
    else
    {
      if (((*string >= indtable[i].buf)
                   && (*string <= (indtable[i].buf + indtable[i].buflen)))
           || ((*string >= indtable[i].valid)
                   && *string <= (indtable[i].valid + indtable[i].validlen)))
      {
        *string = icnedit_addindirectstring(winentry, str);
        if (*string <= 0)
          return FALSE;
        break;
      }
    }
  }
  return TRUE;
}

static BOOL browser_badind_reported;

/* icon = icon number or -1 for title */
/* if justbuild, table is built but not checked */
static load_result browser_check_indirected(browser_winentry *winentry,
                                            icon_flags *flags, icon_data *data,
                                            ind_table *indtable,
                                            int icon, BOOL justbuild)
{
  BOOL broken = FALSE;
  #ifdef DeskLib_DEBUG
    os_error error;
  #endif

  ++icon;     /* title is first entry in table, so simply inc index */

  /* Initialise with neutral values on first pass */
  if (justbuild)
  {
    indtable[icon].buf = indtable[icon].valid = -1;
    indtable[icon].buflen = indtable[icon].validlen = 0;
  }
  /*
  if (!strcmpcr(winentry->identifier, "WCBox"))
  {
    if (justbuild)
    {
      fprintf(stderr, "\nInitialising WCBox idt entry %d to %d:%d %d:%d\n",
              icon - 1, indtable[icon].buf, indtable[icon].buflen,
              indtable[icon].valid, indtable[icon].validlen);
    }
    else fprintf(stderr, "\n");
  }
  */

  if (!flags->data.indirected)
  {
    return load_OK;
  }
  if (!flags->data.text && !flags->data.sprite)
  {
    data->indirecttext.buffer = data->indirecttext.validstring = 0;
    data->indirecttext.bufflen = 0;
    return load_OK;
  }
  if (flags->data.text || flags->data.sprite)
  {
    if (!justbuild &&
        !browser_resolve_shared(winentry, (int *) &data->indirecttext.buffer,
                                indtable, icon, FALSE))
      return load_MemoryError;
    indtable[icon].buf = (int) data->indirecttext.buffer;
    indtable[icon].buflen = (int) data->indirecttext.buffer > 0
                            ? strlencr((char *) winentry->window +
                                       (int) data->indirecttext.buffer)
                            : 0;
    /*
    if (!strcmpcr(winentry->identifier, "WCBox"))
    {
      fprintf(stderr, "WCBox icon %d is ind text/sprite, so changing entry to "
                      "%d:%d %d:%d\n",
              icon - 1, indtable[icon].buf, indtable[icon].buflen,
              indtable[icon].valid, indtable[icon].validlen);
    }
    */

    if ((int) data->indirecttext.buffer >=
        flex_size((flex_ptr) &winentry->window))
    {
      broken = TRUE;
      data->indirecttext.buffer = 0;
      data->indirecttext.bufflen = 0;
    }
  }
  if (flags->data.text)
  {
    if (!justbuild &&
        !browser_resolve_shared(winentry,
                                (int *) &data->indirecttext.validstring,
                                indtable, icon, TRUE))
      return load_MemoryError;
    indtable[icon].valid = (int) data->indirecttext.validstring;
    indtable[icon].validlen = (int) data->indirecttext.validstring > 0
                              ? strlencr((char *) winentry->window +
                                         (int) data->indirecttext.validstring)
                              : 0;
    /*
    if (!strcmpcr(winentry->identifier, "WCBox"))
    {
      fprintf(stderr, "WCBox icon %d is ind text only, so changing entry to "
                      "%d:%d %d:%d\n",
              icon - 1, indtable[icon].buf, indtable[icon].buflen,
              indtable[icon].valid, indtable[icon].validlen);
    }
    */

    if ((int) data->indirecttext.validstring >=
        flex_size((flex_ptr) &winentry->window))
    {
      broken = TRUE;
      data->indirecttext.validstring = 0;
    }
  }
  if (flags->data.sprite && !flags->data.text &&
  	!data->indirectsprite.nameisname)
  {
    broken = TRUE;
    data->indirectsprite.nameisname = wimp_MAXNAME;
  }

  if (!broken)
    return load_OK;

  if (!browser_badind_reported)
  {
    browser_badind_reported = TRUE;
    Log(log_ERROR, "This file has invalid indirected data and cannot be loaded. Please report the problem to the WinEd maintainer.");
    #ifdef DeskLib_DEBUG
      snprintf(error.errmess, sizeof(error.errmess), "Debug: Problem with indirected data - load anyway?");
      if (WinEd_Wimp_ReportErrorR(&error, 11, event_taskname) == 1/* OK */)
        return load_OK;
    #endif
    return load_FatalError;
  }
  return load_OK;
}

/* Check header and read font array */
static load_result browser_load_header(browser_fileinfo *browser,
                                       int *fontoffset,
                                       file_handle fp)
{
  template_header header;
  #ifdef DeskLib_DEBUG
    os_error error;
  #endif

  Log(log_DEBUG, "browser_load_header");

  if (File_ReadBytes(fp,&header,sizeof(template_header)))
    return load_FileError;

  if (header.dummy[0] || header.dummy[1] || header.dummy[2])
  {
    Log(log_ERROR, "This file has an apparently invalid header (%d:%d:%d) and cannot be loaded. Please report the problem to the WinEd maintainer.", header.dummy[0], header.dummy[1], header.dummy[2]);

    #ifdef DeskLib_DEBUG
      snprintf(error.errmess, sizeof(error.errmess), "Debug: Corrupt header - load anyway?");
      if (WinEd_Wimp_ReportErrorR(&error, 11, event_taskname) == 1/* OK */)
      {
        *fontoffset = header.fontoffset;
        return load_OK;
      }
    #endif

    return load_FatalError;
  }

  *fontoffset = header.fontoffset;
  return load_OK;
}

static load_result browser_load_fonts(browser_fileinfo *browser,
                                      template_fontinfo **newfontinfo,
                                      int fontoffset, int filesize,
                                      file_handle fp, char *filename)
{
  Log(log_DEBUG, "browser_load_fonts");

  if (fontoffset != -1)
  {
    if ((fontoffset < filesize) && fontoffset > 0)
    {
      int fonts;
      fonts = (filesize - fontoffset) / sizeof(template_fontinfo);
      Log(log_INFORMATION, " Loading font data (%d bytes = %d fonts)...", filesize - fontoffset, fonts);

      if (!flex_alloc((flex_ptr) newfontinfo, filesize - fontoffset))
      {
        WinEd_MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
        return load_FatalError;
      }

      if (Error_Check(File_Seek(fp,fontoffset)))
      {
        flex_free((flex_ptr) newfontinfo);
        return load_FatalError;
      }

      if (File_ReadBytes(fp,*newfontinfo,filesize - fontoffset))
      {
        if (file_lasterror)
          Error_Check(file_lasterror);
        else
          WinEd_MsgTrans_ReportPS(messages,"BadFileFull",FALSE,filename,0,0,0);

        flex_free((flex_ptr) newfontinfo);
        return load_FatalError;
      }
      else
      {
        int i;
        for (i = 0; i < fonts; i++)
        {
          Log(log_DEBUG, " Font %d: %dx%d %s", i,
           (*newfontinfo)[i].size.x / 16,
           (*newfontinfo)[i].size.y / 16,
           LogPreBuffer((*newfontinfo)[i].name));
        }
      }
    }
    else
    {
      Log(log_ERROR, " Invalid font offset: %d", fontoffset);
      return load_FatalError;
    }
  }
  else
  {
    Log(log_INFORMATION, " No font data found. Font memory not allocated.");
  }

  return load_OK;
}

static load_result browser_load_index_entry(template_index *entry, int index,
                                            file_handle fp, char *filename)
{
  Log(log_DEBUG, "browser_load_index_entry");

  /* Find index'th index entry in file */
  if (Error_Check(File_Seek(fp,sizeof(template_header) + index * sizeof(template_index))))
    return load_FatalError;

  /* Read index entry */
  if (File_ReadBytes(fp,entry,sizeof(template_index)) >= sizeof(template_index))
    return load_FileError;

  return load_OK;
}

static void        browser_make_new_toggle(browser_winentry *winentry)
{
  Log(log_DEBUG, "browser_make_new_toggle");

  winentry->window->window.flags.data.toggleicon = FALSE;
  if (winentry->window->window.flags.data.vscroll)
  {
    winentry->window->window.flags.data.toggleicon = TRUE;
  }
  if (winentry->window->window.flags.data.hscroll &&
      winentry->window->window.flags.data.titlebar)
  {
    winentry->window->window.flags.data.toggleicon = TRUE;
  }
}

static void        browser_make_flags_new(browser_winentry *winentry)
{
  /* Conditions handled rather oddly due to Norcroft bug:
     unable to spill registers when debugging enabled... */

  Log(log_DEBUG, "browser_make_flags_new");

  winentry->window->window.flags.data.titlebar =
            winentry->window->window.flags.data.hastitle;

  if (winentry->window->window.flags.data.titlebar &&
      !winentry->window->window.flags.data.nobackclose)
  {
    winentry->window->window.flags.data.backicon = TRUE;
  }
  else
  {
    winentry->window->window.flags.data.backicon = FALSE;
  }
  winentry->window->window.flags.data.closeicon =
            winentry->window->window.flags.data.backicon;

  winentry->window->window.flags.data.vscroll =
            winentry->window->window.flags.data.hasvscroll;
  winentry->window->window.flags.data.hscroll =
            winentry->window->window.flags.data.hashscroll;

  if (winentry->window->window.flags.data.hscroll ||
      winentry->window->window.flags.data.vscroll)
  {
    winentry->window->window.flags.data.adjusticon = TRUE;
  }
  else
  {
    winentry->window->window.flags.data.adjusticon = FALSE;
  }

  if (winentry->window->window.flags.data.hscroll ||
      winentry->window->window.flags.data.vscroll)
  {
    winentry->window->window.flags.data.adjusticon = TRUE;
  }
  else
  {
    winentry->window->window.flags.data.adjusticon = FALSE;
  }

  /* This seems to be culprit, requiring separate function etc */
  browser_make_new_toggle(winentry);

  winentry->window->window.flags.data.newflags = 1;
}

static load_result browser_all_indirected(browser_fileinfo *browser,
                                          browser_winentry *winentry,
                                          template_index *entry)
{
  int pass;
  load_result result;
  int icon;
  ind_table *indtable;  /* Table of indirected text offsets
                           to resolve shared strings */

  Log(log_DEBUG, "browser_all_indirected");

  indtable = malloc(sizeof(ind_table) *
                    (winentry->window->window.numicons + 1));
  if (!indtable)
  {
    return load_MemoryError;
  }

  /* Resolving shared indirected strings requires 2 passes */
  for (pass = 1; pass >= 0; --pass)
  {
    result = browser_check_indirected(winentry,
                                      &winentry->window->window.titleflags,
                                      &winentry->window->window.title,
                                      indtable, -1, pass);
    if (result)
    {
      /* Could be: load_FatalError, load_MemoryError */
      free(indtable);
      return result;
    }
    for (icon = 0; icon < winentry->window->window.numicons; icon++)
    {
      result = browser_check_indirected(winentry,
                                        &winentry->window->icon[icon].flags,
                                        &winentry->window->icon[icon].data,
                                        indtable, icon, pass);
      if (result)
      {
        /* Could be: load_FatalError, load_MemoryError */
        free(indtable);
        return result;
      }
    }
  }
  free(indtable);
  return load_OK;
}

/* Process font data; out of range font handles are reported,
   icon is converted to sys font in nasty colours */
static void        browser_process_fonts(browser_fileinfo  *browser,
                                         browser_winentry  *winentry,
                                         template_fontinfo **newfontinfo)
{
  BOOL reported = FALSE;
  int icon;

  Log(log_DEBUG, "browser_process_fonts");

  if (winentry->window->window.titleflags.data.font)
  {
    Log(log_DEBUG, " Title uses fonts");
    if (winentry->window->window.titleflags.font.handle *
        sizeof(template_fontinfo) >
        flex_size((flex_ptr) newfontinfo))
    {
      Log(log_ERROR, "  Bad title font: bigger than structure??");
      if (!reported)
      {
        WinEd_MsgTrans_Report(messages,"BadFont",FALSE);
        reported = TRUE;
      }
      winentry->window->window.titleflags.font.handle = 0xef;
      winentry->window->window.titleflags.data.font = 0;
    }
    else
    {
      Log(log_DEBUG, "  Title font OK");
      winentry->window->window.titleflags.font.handle =
        tempfont_findfont(browser, &(*newfontinfo)[winentry->window->window.titleflags.font.handle-1]);
    }
  }

  Log(log_DEBUG, " Checking for icon fonts (in all %d of them)...", winentry->window->window.numicons);
  for (icon = 0; icon < winentry->window->window.numicons; icon++)
  {
    if (winentry->window->icon[icon].flags.data.font)
    {
      Log(log_DEBUG, "  Icon %d uses fonts", icon);

      if (winentry->window->icon[icon].flags.font.handle *
          sizeof(template_fontinfo) >
          flex_size((flex_ptr) newfontinfo))
      {
        Log(log_ERROR, "  Badness. More font handles than fonts");
        if (!reported)
        {
          WinEd_MsgTrans_Report(messages,"BadFont",FALSE);
          reported = TRUE;
        }
        winentry->window->icon[icon].flags.font.handle = 0xef;
        winentry->window->icon[icon].flags.data.font = 0;
      }
      else
      {
        #ifdef REGRESSION_ANNI
          /* For some reason this line resulted in the handle getting corrupted. */
          /* Load the Anni template to see this problem. */
          winentry->window->icon[icon].flags.font.handle =
            tempfont_findfont(browser, &(*newfontinfo)[winentry->window->icon[icon].flags.font.handle-1]);
        #else
          unsigned int returned = 0;
          returned = tempfont_findfont(browser, &(*newfontinfo)[winentry->window->icon[icon].flags.font.handle-1]);
          winentry->window->icon[icon].flags.font.handle = returned;
        #endif

        Log(log_DEBUG, "  Actual font handle for icon %d: %d", icon, winentry->window->icon[icon].flags.font.handle);
      }
    }
  }
}

BOOL               validate_icon_data(icon_block icon)
{
  /* Return FALSE if there's some problem */
  BOOL result = TRUE;

  #ifdef DeskLib_DEBUG
  if ((icon.flags.data.text || icon.flags.data.sprite) && !icon.flags.data.indirected)
    Log(log_DEBUG, " Icon data: %s", LogPreBuffer(icon.data.text));
  #endif

  if (  (icon.workarearect.min.x > icon.workarearect.max.x)
     || (icon.workarearect.min.y > icon.workarearect.max.y) )
  {
    Log(log_WARNING, "\\r Icon dimensions topsy turvy");
    result = FALSE;
  }

  return result;
}

static load_result browser_verify_data(browser_winentry *winentry)
{
  /* This function is used to trap problems in original template, at load time. */

  load_result result = load_OK;
  int icon;

  for (icon = 0; icon < winentry->window->window.numicons; icon++)
  {
    Log(log_TEDIOUS, "Validating icon %d", icon);
    if (!validate_icon_data(winentry->window->icon[icon]))
    {
      result = load_IconError;
      icon = winentry->window->window.numicons; /* break out of for loop */
    }
  }

  return result;
}

static load_result browser_load_templat(browser_fileinfo *browser,
                                         template_index *entry,
                                         template_fontinfo **newfontinfo,
                                         file_handle fp, char *filename)
{
  browser_winentry *winentry;
  load_result result;
  browser_winentry *tempitem;
  BOOL renamed = FALSE;
  char suffix_string[12], *terminator;
  int tries = 1000, suffix = 1, suffix_length;

  #ifdef DeskLib_DEBUG
  int i;
  #endif

  Log(log_DEBUG, "browser_load_templat");

  /* Create new window entry block for it */
  winentry = malloc(sizeof(browser_winentry));
  if (!winentry)
    return load_MemoryError;

  /* Copy the identifier into the target buffer. */
  terminator = strncpycr(winentry->identifier, entry->identifier, sizeof(winentry->identifier));

  /* StrnCpyCr won't terminate the string if it doesn't fit, so an unterminated string is a warning. */
  if (*terminator != '\0')
  {
    winentry->identifier[sizeof(winentry->identifier) - 1] = '\0';
    WinEd_MsgTrans_ReportPS(messages, "LongIdent", FALSE, winentry->identifier, 0, 0, 0);
  }

  /* Check for duplicates (possibly we've re-named to a pre-existing name),
   * by looping through all of the existing names and comparing to each. */
  Log(log_DEBUG, "Checking for duplicates of: %s", LogPreBuffer(winentry->identifier));

  tempitem = LinkList_FirstItem(&browser->winlist);

  while (tempitem && (tries-- > 0))
  {
    Log(log_DEBUG, " checking:%s", LogPreBuffer(tempitem->identifier));

    if (strcmpcr(winentry->identifier, tempitem->identifier) == 0)
    {
      snprintf(suffix_string, sizeof(suffix_string), "~%d", suffix++);
      suffix_string[sizeof(suffix_string) - 1] = '\0';

      suffix_length = strlen(suffix_string) + 1;
      strncpy(winentry->identifier + sizeof(winentry->identifier) - suffix_length, suffix_string, suffix_length);

      renamed = TRUE;

      Log(log_INFORMATION, "Identifier is not unique. Trying '%s'", LogPreBuffer(winentry->identifier));

      /* Go back to start and start the comparisons over again */
      tempitem = LinkList_FirstItem(&browser->winlist);
      Log(log_DEBUG, "Back to the start...");
    }
    else
      /* Carry on to check next item */
      tempitem = LinkList_NextItem(tempitem);
  }

  if (renamed) /* We had to change the name */
  {
    if (tempitem != NULL) /* Couldn't find a unique name */
    {
      /* Free this window */
      free(winentry);
      return load_DefinitionError;
    }
    else
      /* Inform user of changed name */
      WinEd_MsgTrans_ReportPS(messages, "DupeName", FALSE, winentry->identifier, 0, 0, 0);
  }

  /* The window name is OK, to add to the linked list. */

  LinkList_AddToTail(&browser->winlist,&winentry->header);

  winentry->icon = -1;

  /* Allocate memory and load window/icon data */
  if (!flex_alloc((flex_ptr) &winentry->window,entry->size))
  {
    /* Free whole browser */
    LinkList_Unlink(&browser->header,&winentry->header);
    free(winentry);
    return load_MemoryError;
  }
  if (File_Seek(fp,entry->offset) ||
      File_ReadBytes(fp,winentry->window,entry->size))
  {
    /* Free whole browser */
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->header,&winentry->header);
    free(winentry);
    return load_FileError;
  }


  winentry->status = status_CLOSED;
  winentry->iconised = FALSE;
  winentry->fontarray = 0;
  winentry->browser = browser;

  /* Check for old format flags */
  if (!winentry->window->window.flags.data.newflags)
  {
    Log(log_NOTICE, "Old format flags found");
    browser_make_flags_new(winentry);
  }

  /* Check indirected data */
  result = browser_all_indirected(browser, winentry, entry);
  if (result) /* Could be load_MemoryError or load_FatalError */
  {
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->winlist,&winentry->header);
    free(winentry);
    return result;
  }

  /* Verify template data */
  result = browser_verify_data(winentry);
  if (result == load_IconError)
  {
    /* Free this window */
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->winlist,&winentry->header);
    free(winentry);
    return result;
  }

  browser_changesparea(winentry->window, user_sprites);

  browser_process_fonts(browser, winentry, newfontinfo);

  browser->numwindows++;

  #ifdef DeskLib_DEBUG
  /* Check for anni bug */
  for (i = 0; i < winentry->window->window.numicons; i++)
  {
    if ((winentry->window->icon[i].workarearect.min.x > 5000) || (winentry->window->icon[i].workarearect.min.x < -5000))
      Log(log_ERROR, "Thar she blows! Start of icon %d corrupted (probably - min x:%d)", i, winentry->window->icon[i].workarearect.min.x);
  }

  Log(log_NOTICE, " window '%s' loaded. %d icons.", LogPreBuffer(winentry->identifier), winentry->window->window.numicons);
  #endif

  return load_OK;
}


static load_result browser_load_from_fp(browser_fileinfo *browser,
                                        file_handle fp, int filesize,
                                        char *filename)
{
  template_index entry;
  int index;
  int fontoffset;
  template_fontinfo *newfontinfo = NULL;
  load_result result;

  Log(log_DEBUG, "browser_load_from_fp");

  browser_badind_reported = FALSE;
  result = browser_load_header(browser, &fontoffset, fp);
  if (result)
    /* Could be: load_FileError, load_FatalError */
    return result;

  result = browser_load_fonts(browser, &newfontinfo, fontoffset, filesize, fp, filename);
  if (result)
    /* Could be: load_FatalError */
    return result;

  for (index = 0; !result; ++index)
  {
    result = browser_load_index_entry(&entry, index, fp, filename);
    /* Could be load_FatalError or load_FileError */

    if (entry.offset == 0) /* Finished */
      result = load_Finished;

    if (!result) /* Don't bother with this section if there's already been a problem or we've finished loading */
    {
      if (entry.type != 1)
      {
        char num[8];
        #ifdef DeskLib_DEBUG
          os_error error;
        #endif

        snprintf(num, sizeof(num), "%d", entry.type);
        #ifdef DeskLib_DEBUG
           snprintf(error.errmess, sizeof(error.errmess), "Debug: Unknown object type (%d) - load as window?", entry.type);
           if (WinEd_Wimp_ReportErrorR(&error, 11, event_taskname) == 1/* OK */)
             entry.type = 1;
         #else
           WinEd_MsgTrans_ReportPS(messages, "NotWindow", FALSE, filename, num, 0, 0);
        #endif
      }

      if (entry.type == 1)
      {
        char temp_winident[wimp_MAXNAME];

        /* Store identifier for potential use later, if window memory is freed in browser_load_templat */
        strncpycr(temp_winident, entry.identifier, sizeof(temp_winident));
        temp_winident[sizeof(temp_winident) - 1] = '\0';

        result = browser_load_templat(browser, &entry, &newfontinfo, fp, filename);
        /* Result could be:
           1) Drop individual window:         load_DefinitionError, load_IconError
           2) Drop remaining browser windows: load_MemoryError, load_FileError
           3) Disaster. Scrap whole browser:  load_FatalError
        */

        if (result == load_IconError)
        {
          /* Tell user */
          WinEd_MsgTrans_ReportPS(messages, "ContIcon", FALSE, temp_winident, 0, 0, 0);
          /* Reset result to allow for loop to continue */
          result = load_OK;
        }
        else if (result == load_DefinitionError)
        {
          Debug_Printf("its:%s and %s", temp_winident, entry.identifier);

          WinEd_MsgTrans_ReportPS(messages, "DupeNameFail", FALSE, temp_winident, 0, 0, 0);
          result = load_OK;
        }
        /* Remaining clauses (including implied "else") leave result as true, so the loop exits */
        else if (result == load_FileError)
        {
          if (index) /* Don't tell user we've loaded something if we haven't */
            WinEd_MsgTrans_ReportPS(messages, "BadFilePart", FALSE, filename, 0, 0, 0);
        }
        else if (result == load_MemoryError)
        {
          if (index)
            WinEd_MsgTrans_ReportPS(messages, "ContMem", FALSE, filename, 0, 0, 0);
        }
      }
    }
  }

  if (newfontinfo)
    flex_free((flex_ptr) &newfontinfo);

  if (!index) /* E.g. failed to load anything at all and no warning issued yet */
    return result;

  if (result != load_FatalError)
    /* We can't cope with FatalError. All others should be OKish so pass back load_OK to browser_getfile */
    result = load_OK;

  return result;
}

BOOL               browser_getfile(char *filename,int filesize,browser_fileinfo *browser)
{
  file_handle fp;
  load_result result;
  void *checkanc;

  Log(log_DEBUG, "browser_getfile");

  /* Do a quick check to see if there is apparently enough memory */
  if (!flex_alloc(&checkanc,filesize))
  {
     WinEd_MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
     return FALSE;
  }
  flex_free(&checkanc);
  Hourglass_On();

  fp = File_Open(filename,file_READ);
  if (!fp)
  {
    Hourglass_Off();
    if (file_lasterror)
      Error_Check(file_lasterror);
    else
      WinEd_MsgTrans_ReportPS(messages,"WontOpen",FALSE,filename,0,0,0);
    return FALSE;
  }

  result = browser_load_from_fp(browser, fp, filesize, filename);
  /* Results could be: load_OK                                           - all dandy
                       load_FatalError, load_FileError, load_MemoryError - abandon whole browser */

  File_Close(fp);
  Hourglass_Off();

  switch (result)
  {
    case load_FileError:
      if (file_lasterror)
        Error_Check(file_lasterror);
      else
        WinEd_MsgTrans_ReportPS(messages,"BadFileFull",FALSE,filename,0,0,0);
      break;
    case load_MemoryError:
      WinEd_MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
      break;
    case load_FatalError:
      WinEd_MsgTrans_ReportPS(messages,"LoadRep",FALSE,filename,0,0,0);
      break;
    default:
      break; /* Here to suppress compiler warning about not using load_OK etc */
  }

  /* If we return FALSE, the browser is abandoned using browser_close */
  return result ? FALSE : TRUE;
}

icon_handle        browser_newicon(browser_fileinfo *browser,int index, browser_winentry *winentry,int selected)
{
  icon_createblock newicdata;
  icon_handle newichandle;
  int column,row;
  /* window_redrawblock redraw; */

  Log(log_TEDIOUS, "browser_newicon");
  Log(log_DEBUG, "Create icon at index %d for %s from 0x%x", index, winentry->identifier, winentry);

  column = index % browser->numcolumns;
  row = index / browser->numcolumns;
  newicdata.window = browser->window;
  newicdata.icondata.workarearect.min.x = MARGIN + column * (MARGIN + WIDTH);
  newicdata.icondata.workarearect.max.x = newicdata.icondata.workarearect.min.x + WIDTH;
  newicdata.icondata.workarearect.max.y = - MARGIN - row * (MARGIN + HEIGHT);
  if (choices->browtools)
    newicdata.icondata.workarearect.max.y -= browtools_paneheight;
  newicdata.icondata.workarearect.min.y = newicdata.icondata.workarearect.max.y - HEIGHT;
  newicdata.icondata.flags.value = 0;
  newicdata.icondata.flags.data.text = 1;
  newicdata.icondata.flags.data.sprite = 1;
  newicdata.icondata.flags.data.vcentre = 1;
  newicdata.icondata.flags.data.indirected = 1;
  newicdata.icondata.flags.data.allowadjust = 1;
  newicdata.icondata.flags.data.needshelp = 1;
  newicdata.icondata.flags.data.selected = selected;
  newicdata.icondata.flags.data.buttontype = iconbtype_DOUBLECLICKDRAG;
  newicdata.icondata.flags.data.esg = 1;
  newicdata.icondata.flags.data.foreground = 7;
  newicdata.icondata.flags.data.background = 1;
  newicdata.icondata.data.indirecttext.buffer = winentry->identifier;
  newicdata.icondata.data.indirecttext.bufflen = wimp_MAXNAME;
  if (winentry->window->window.flags.data.hscroll || winentry->window->window.flags.data.vscroll)
  {
    if (winentry->status == status_CLOSED)
      newicdata.icondata.data.indirecttext.validstring = browser_windowvalid;
    else
      newicdata.icondata.data.indirecttext.validstring = browser_owindowvalid;
  }
  else
  {
    if (winentry->status == status_CLOSED)
      newicdata.icondata.data.indirecttext.validstring = browser_dialogvalid;
    else
      newicdata.icondata.data.indirecttext.validstring = browser_odialogvalid;
  }

  if (Error_Check(Wimp_CreateIcon(&newicdata,&newichandle)))
    return -1;

  return newichandle;
}

void               browser_delwindow(browser_winentry *winentry)
{
  Log(log_DEBUG, "browser_delwindow");

  if (winentry->status)
    viewer_close(winentry);

  tempfont_losewindow(winentry->browser,winentry);
  winentry->browser->numwindows--;
  flex_free((flex_ptr) &winentry->window);
  LinkList_Unlink(&winentry->browser->winlist,&winentry->header);
  free(winentry);
}

void               browser_deletewindow(browser_winentry *winentry)
{
  Log(log_DEBUG, "browser_deletewindow");
  Log(log_DEBUG, "Associated icon: %d", winentry->icon);

  if (winentry->icon != -1)
    Wimp_DeleteIcon(winentry->browser->window,winentry->icon);

  browser_delwindow(winentry);
}

browser_winentry  *browser_findnamedentry(browser_fileinfo *browser,char *id)
{
  browser_winentry *winentry;

  Log(log_DEBUG, "browser_findnamedentry");

  winentry = LinkList_NextItem(&browser->winlist);
  while (winentry && strcmpcr(winentry->identifier,id))
    winentry = LinkList_NextItem(winentry);
  return winentry;
}

void               browser_setextent(browser_fileinfo *browser)
{
  window_info info;
  wimp_rect extent;
  Log(log_DEBUG, "browser_setextent");

  extent.min.x = 0;
  extent.min.y = - MARGIN -
                 ((browser->numwindows + browser->numcolumns - 1) /
                 browser->numcolumns) * (MARGIN + HEIGHT);
  extent.max.x = MARGIN + MAXNUMCOLUMNS * (WIDTH + MARGIN);
  extent.max.y = 0;
  if (extent.min.y > - MARGIN - 3 * (MARGIN + HEIGHT))
    extent.min.y = - MARGIN - 3 * (MARGIN + HEIGHT);
  if (choices->browtools)
    extent.min.y -= browtools_paneheight;
  Wimp_SetExtent(browser->window,&extent);

  /* Adjust the extent of the toolbar to match. */

  if (choices->browtools)
  {
    info.window = browser->pane;
    Wimp_GetWindowInfoNoIcons(&info);
    extent.min.x = info.block.workarearect.min.x;
    extent.min.y = info.block.workarearect.min.y;
    extent.max.x = extent.min.x + MARGIN + MAXNUMCOLUMNS * (WIDTH + MARGIN);
    extent.max.y = info.block.workarearect.max.y;
    Wimp_SetExtent(browser->pane,&extent);
  }
}

/**
 * Compare two browser_winentry blocks alpabetically by identifier.
 * 
 * \param *one    Pointer to the first block to compare.
 * \param *two    Pointer to the second block to compare.
 * \return        The comparison of the two identifiers.
 */
int alphacomp(const void *one, const void *two)
{
  const browser_winentry **we_one = (const browser_winentry **) one, **we_two = (const browser_winentry **) two;
  char lc_one[wimp_MAXNAME];
  char lc_two[wimp_MAXNAME];

  strncpy(lc_one, (*we_one)->identifier, wimp_MAXNAME);
  strncpy(lc_two, (*we_two)->identifier, wimp_MAXNAME);

  lc_one[wimp_MAXNAME - 1] = '\0';
  lc_two[wimp_MAXNAME - 1] = '\0';

  lower_case(lc_one);
  lower_case(lc_two);

  return strcmpcr(lc_one, lc_two);
}

char *lower_case(char *s)
{
//  Log(log_DEBUG, "lower_case");

  char *ret = s;
  if (s)
  {
    while (*s)
    {
      *s = tolower(*s);
      s++;
    }
  }
  return ret;
}

/**
 * Build an array of winentry pointers for the current file, optionally sorted
 * alphabetically by identifier.
 * 
 * The array is allocated with malloc(), and must be freed after use.
 *
 * \param *browser    Pointer to the browser instance to sort.
 * \param sort        TRUE to sort the entries; FALSE to leave in file order.
 * \return            Pointer to the resulting array, or NULL on failure.
 */
browser_winentry **browser_sortwindows(browser_fileinfo *browser, BOOL sort)
{
  browser_winentry **sorted, *winentry;
  int count = 0;

  Log(log_DEBUG, "browser_sortwindows");

  if (browser == NULL)
    return NULL;

  /* Allocate an array of window pointers, and fill it in file order. */

  sorted = malloc(sizeof(browser_winentry*) * browser->numwindows);
  if (sorted == NULL)
    return NULL;

  winentry = LinkList_FirstItem(&browser->winlist);

  while ((winentry != NULL) && (count < browser->numwindows))
  {
    sorted[count++] = winentry;
    winentry = LinkList_NextItem(&winentry->header);
  }

  Log(log_DEBUG, "Found %d windows, out of %d.", count, browser->numwindows);

  if (sort == TRUE)
    qsort(sorted, count, sizeof(browser_winentry*), alphacomp);

  return sorted;
}

/**
 * Sort and place the icons in a file browser window, creating any which don't
 * exist in the process.
 * 
 * \param *browser  The browser instance to be operated on.
 * \param force     TRUE to delete all icons, even if they don't need to move.
 * \param reopen    TRUE to call Wimp_OpenWindow on the window after update.
 * \param keepsel   TRUE to restore any selection after the update.
 */
void browser_sorticons(browser_fileinfo *browser, BOOL force, BOOL reopen, BOOL keepsel)
{
  int index;
  browser_winentry **winentries, *winentry;
  window_state wstate;
  int *selected;

  Log(log_DEBUG, "browser_sorticons");

  /* Get the winentry indexes. Note that "round" means "sort browser icons alphabetically" (!). */

  winentries = browser_sortwindows(browser, choices->browser_sort);
  if (winentries == NULL)
    return;

  /* If we're keeping the selections, record the selected icons. */

  if (keepsel == TRUE)
  {
    selected = malloc(browser->numwindows * sizeof(int));
    if (selected)
    {
      for (index = 0; index < browser->numwindows; index++)
        selected[index] = Icon_GetSelect(browser->window, index);
    }
  }
  else
  {
    selected = NULL;
  }

  /* Replace the icons in the window. */

  for (index = 0; index < browser->numwindows; index++)
  {
    winentry = winentries[index];

    /* Delete any existing icon. */

    if (winentry->icon != -1 && (winentry->icon != index || force))
    {
      Wimp_DeleteIcon(browser->window, winentry->icon);
      winentry->icon = -1;
    }

    /* Create a new icon. */

    if (winentry->icon == -1)
      winentry->icon = browser_newicon(browser, index, winentry, keepsel && selected != NULL && selected[index]);
  }

  if (keepsel == TRUE && selected != NULL)
    free(selected);

  if (winentries != NULL)
    free(winentries);

  browser_setextent(browser);
  Wimp_GetWindowState(browser->window,&wstate);
  if (choices->browtools)
    browtools_open(&wstate.openblock,browser,reopen);
  else if (reopen)
    Wimp_OpenWindow(&wstate.openblock);
  Wimp_GetWindowState(browser->window,&wstate);
  wstate.openblock.screenrect.max.x -=
  	(wstate.openblock.screenrect.min.x - wstate.openblock.scroll.x);
  wstate.openblock.screenrect.min.x = wstate.openblock.scroll.x;
  wstate.openblock.screenrect.min.y -=
  	(wstate.openblock.screenrect.max.y - wstate.openblock.scroll.y);
  wstate.openblock.screenrect.max.y = wstate.openblock.scroll.y;
  Wimp_ForceRedraw((window_redrawblock *) &wstate.openblock);
  if (choices->browtools)
    browtools_shadeapp(browser);
}

/* See title.h */
void               browser_settitle(browser_fileinfo *browser,char *title,BOOL altered)
{
  Log(log_DEBUG, "browser_settitle");

  if (altered != browser->altered || title)
  {
    char buffer[256];

    if (!title || !*title)
    {
      int len;

      /* Copy title to buffer and ensure zero termination */
      strncpy(buffer,browser->title,256);
      Str_MakeASCIIZ(buffer,256);

      /* Strip " *" if present */
      len = strlen(buffer);
      if (buffer[len - 1] == '*')
        buffer[len - 2] = 0;
    }
    else
    {
      strncpy(buffer,title,256);
      buffer[strlencr(buffer)] = 0;
    }

    if (altered)
      strncat(buffer," *",256);

    Window_SetTitle(browser->window,buffer);

    browser->altered = altered;

  }
  stats_update(browser);
}

BOOL               browser_prequit(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = LinkList_NextItem(&browser_list);
  BOOL unsaved = FALSE;

  Log(log_DEBUG, "browser_prequit");

  while (browser)
  {
    if (browser->altered)
    {
      unsaved = TRUE;
      break;
    }
    browser = LinkList_NextItem(browser);
  }

  if (unsaved == TRUE)
    unsaved_quit(event);

  Log(log_NOTICE, "WinEd shutting down (external request), \\t");

  return TRUE;
}

void               browser_shutdown()
{
  browser_fileinfo *browser;

  Log(log_DEBUG, "browser_shutdown");

  Hourglass_Smash();

  for (browser = LinkList_NextItem(&browser_list);
       browser;
       browser = LinkList_NextItem(browser))
  {
    browser_winentry *winentry;

    for (winentry = LinkList_NextItem(&browser->winlist);
    	 winentry;
    	 winentry = LinkList_NextItem(winentry))
    {
      if (winentry->fontarray)
        Font_LoseAllFonts(winentry->fontarray);
    }
  }
}

void               browser_preselfquit()
{
  browser_fileinfo *browser = LinkList_NextItem(&browser_list);
  BOOL unsaved = FALSE;

  Log(log_DEBUG, "browser_preselfquit");

  while (browser)
  {
    if (browser->altered)
    {
      unsaved = TRUE;
      break;
    }
    browser = LinkList_NextItem(browser);
  }

  if (unsaved == TRUE)
  {
    unsaved_quit(0);
    return;
  }

  Log(log_NOTICE, "WinEd shutting down (user request), \\t");

  exit(0);
}

/* Save file in strict order for speed (NB: what "strict order"? We can now sort files.) */
BOOL               browser_save(char *filename,void *reference,BOOL selection)
{
  browser_fileinfo *browser = reference;
  file_handle fp;
  BOOL result;

  Log(log_DEBUG, "browser_save");

  Hourglass_On();

  if (!browser->winlist.next)
  {
    WinEd_MsgTrans_Report(messages,"Empty",FALSE);
    return FALSE;
  }

  fp = File_Open(filename,file_WRITE);
  if (!fp)
  {
    if (file_lasterror)
      Error_Check(file_lasterror);
    else
      WinEd_MsgTrans_ReportPS(messages,"WontOpen",FALSE,filename,0,0,0);
    return FALSE;
  }

  result = browser_dosave(filename, browser, choices->file_sort, selection, fp);

  File_Close(fp);
  if (result)
    File_SetType(filename,filetype_TEMPLATE);
  else
    File_Delete(filename);

  Hourglass_Off();

  return TRUE;
}

BOOL               browser_save_check(char *filename, void *reference, BOOL selection)
{
  BOOL returnvalue;
  char current_title[256];
  int title_length;

  Log(log_DEBUG, "browser_save_check, filename: %s", filename);

  /* Save info to pass on to the overwrite handler */
  strncpy(overwrite_check_filename, filename, 1024);
  overwrite_check_browser = (browser_fileinfo *) reference;
  overwrite_check_selection = selection;

  /* Get existing title - strip trailing " *" if it's there */
  strncpycr(current_title, overwrite_check_browser->title, 256);
  title_length = strlen(current_title);
  if (!strcmp(&current_title[title_length-2], " *"))
    current_title[title_length-2] = '\0';

  /* Check whether filename has changed and exists already
     We don't check if the target is <Wimp$Scrap>, as we can assume that anything
     there shouldn't have been left after use and is therefore safe to delete. */
  if (strcmpcr(current_title, filename) && strcmpcr("<Wimp$Scrap>", filename) && File_Exists(filename))
  {
    /* Open the question window as a menu-come-window */
    transient_centre(overwrite_warn);
    overwrite_warn_open = TRUE;

    /* Register handlers */
    Event_Claim(event_OPEN,  overwrite_warn, event_ANY,           Handler_OpenWindow,    0);
    Event_Claim(event_CLOSE, overwrite_warn, event_ANY,           overwrite_checkanswer, 0);
    Event_Claim(event_CLICK, overwrite_warn, overwrite_OVERWRITE, overwrite_checkanswer, (void *)1);
    Event_Claim(event_CLICK, overwrite_warn, overwrite_CANCEL,    overwrite_checkanswer, 0);
    help_claim_window(overwrite_warn, "OVR");
    returnvalue = FALSE;
  }
  else
  {
    /* No need to hang about, just save it */
    returnvalue = browser_save(filename, reference, selection);
  }

  /* This determines whether the browser name is updated and the * removed */
  return returnvalue;
}

BOOL               overwrite_checkanswer(event_pollblock *event, void *reference)
{
  BOOL overwrite, returnvalue;
  void *ref;

  Log(log_DEBUG, "overwrite_checkanswer");

  /* Release handlers */
  Event_ReleaseWindow(overwrite_warn);

  /* Close window */
  overwrite_warn_open = FALSE;
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);

  overwrite = (BOOL) reference;

  if (overwrite)
  {
    /* Cast browser_info back to reference */
    ref = (void *) overwrite_check_browser;

    /* They say overwrite, so we overwrite! */
    if (browser_save(overwrite_check_filename, ref, overwrite_check_selection))
    {
      /* Sort out browser title bar */
      browser_savecomplete(ref, TRUE, TRUE, overwrite_check_filename, overwrite_check_selection);

      /* Close the browser window, if we've saved successfully and got here via a click on the close icon */
      if (overwrite_save_from_close) browser_forceclose(overwrite_check_browser);
    }

    returnvalue = TRUE;
  }
  else
  {
    /* Release handlers */
    Event_ReleaseWindow(overwrite_warn);

    /* Close window */
    overwrite_warn_open = FALSE;
    WinEd_CreateMenu((menu_ptr) -1, 0, 0);

    returnvalue = FALSE;
  }

  /* Reset global variable which tracks if we got here from a close-icon click */
  overwrite_save_from_close = FALSE;
  return returnvalue;
}

static void        browser_shademultisel()
{
  Log(log_DEBUG, "browser_shademultisel");

  Menu_SetFlags(browser_submenu,submenu_COPY,0,1);
  Menu_SetFlags(browser_submenu,submenu_RENAME,0,1);
  Menu_SetFlags(browser_submenu,submenu_PREVIEW,0,1);
  Menu_SetFlags(browser_submenu,submenu_EDIT,0,1);
}

static void        browser_selunshade()
{
  Log(log_DEBUG, "browser_selunshade");

  Menu_SetFlags(browser_parentmenu,parent_CLEAR,0,0);
  Menu_SetFlags(browser_parentmenu,parent_templat,0,0);
  Menu_SetFlags(browser_submenu,submenu_DELETE,0,0);
  Menu_SetFlags(browser_submenu,submenu_EXPORT,0,0);
}

void               browser_makemenus(browser_fileinfo *browser,int x,int y,
                       icon_handle icon)
{
  char buffer[64];
  int selections;

  Log(log_DEBUG, "browser_makemenus");

  /* Count selected icons */
  selections = count_selections(browser->window);

  /* Find selected icon or one under pointer */
  if (icon != -1 && browser_selection_withmenu)
  {
    browcom_clearsel();
    selections = 1;
    Icon_Select(browser->window,icon);
    selection_browser = browser;
  }

  /* Fill in templat '...'/Selection part of main menu */
  if (selections == 1)
  {
    /* Submenu title = 'templat' */
    MsgTrans_Lookup(messages,"BMTemp",buffer,64);
    strncpy(browser_submenu->title,buffer,12);

    /* Make "templat '...'" entry */
    sprintf(buffer+strlen(buffer)," \'%s",
    	    Icon_GetTextPtr(browser->window,
    	    		    Icon_WhichRadioInEsg(browser->window,1)));
    /* strcatcr(buffer,"\'"); */
    strcpy(buffer+strlencr(buffer),"\'");

    Menu_SetText(browser_parentmenu,parent_templat,buffer);
  }
  else
  {
    /* Submenu title/this entry = 'Selection' */
    MsgTrans_Lookup(messages,"BMSel",buffer,32);
    strncpy(browser_submenu->title,buffer,12);
    Menu_SetText(browser_parentmenu,parent_templat,buffer);
  }

  /* Shade appropriate menu items depending on selection */
  switch (selections)
  {
    case 0:
      Menu_SetFlags(browser_parentmenu,parent_CLEAR,0,1);
      Menu_SetFlags(browser_parentmenu,parent_templat,0,1);
      Menu_SetFlags(browser_submenu,submenu_DELETE,0,1);
      Menu_SetFlags(browser_submenu,submenu_EXPORT,0,1);
      browser_shademultisel();
      break;
    case 1:
      Menu_SetFlags(browser_submenu,submenu_COPY,0,0);
      Menu_SetFlags(browser_submenu,submenu_RENAME,0,0);
      Menu_SetFlags(browser_submenu,submenu_PREVIEW,0,0);
      Menu_SetFlags(browser_submenu,submenu_EDIT,0,0);
      browser_selunshade();
      break;
    default:
      browser_selunshade();
      browser_shademultisel();
      break;
  }

  WinEd_CreateMenu(browser_parentmenu,x,y);
  /*menu_destroy = */ browser_menuopen = TRUE;

  /* Register handlers */
  Event_Claim(event_MENU,event_ANY,event_ANY,browser_menuselect,browser);
  EventMsg_Claim(message_MENUSDELETED,event_ANY, browser_releasemenus,browser);
  EventMsg_Claim(message_MENUWARN,event_ANY,browser_sublink,browser);
  help_claim_menu("BRM");
}

BOOL               browser_menuselect(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  browser_winentry *winentry;
  int index;
  wimp_point lastmenupos;
  browser_fileinfo *browser = reference;

  Log(log_DEBUG, "browser_menuselect");

  Wimp_GetPointerInfo(&ptrinfo);

  switch (event->data.selection[0])
  {
    case parent_SAVE:
      /* We don't need to check for overwriting another file here as we're only saving to the location we loaded from */
      if (strchr(browser->title, ',') || strchr(browser->title, ':'))
        if (browser_save(browser->title,reference,FALSE))
          browser_settitle(browser,NULL,FALSE);
      break;
    case parent_templat:
      switch (event->data.selection[1])
      {
        case submenu_DELETE:
          browcom_delete(browser);
          break;
        case submenu_PREVIEW:
        case submenu_EDIT:
          index = -1;
          winentry = browser_findselection(browser,&index,browser->numwindows);
          if (winentry == NULL)
            break;
          if (winentry->status)
            viewer_close(winentry);
          viewer_open(winentry,event->data.selection[1] == submenu_EDIT);
          /* Refresh browser to reflect changed state of viewer */
          browser_sorticons(winentry->browser,TRUE,FALSE,TRUE);
          break;
      }
      break;
    case parent_SELALL:
      lastmenupos = menu_currentpos;
      browcom_selall(browser);
      if (ptrinfo.button.data.adjust)
      {
        rightclickonselectall = TRUE;
        /*browser_releasemenus(event,reference);*/
        browser_makemenus(browser,lastmenupos.x+64,lastmenupos.y,-1);
        return TRUE;
      }
      break;
    case parent_CLEAR:
      lastmenupos = menu_currentpos;
      browcom_clearsel();
      if (ptrinfo.button.data.adjust)
      {
        /*browser_releasemenus(event,reference);*/
        browser_makemenus(browser,lastmenupos.x+64,lastmenupos.y,-1);
        return TRUE;
      }
      break;
    case parent_STATS:
      stats_open(browser);
      break;
  }

  if (ptrinfo.button.data.adjust)
    Menu_ShowLast();
  /*else
    browser_releasemenus(event,reference);*/

  return TRUE;
}

BOOL               browser_releasemenus(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "browser_releasemenus");

  /*if (!menu_destroy)
  { */
    if (create_open && win_is_open(create_dbox))
      release_create();
    if (copy_open && win_is_open(copy_dbox))
      release_copy();
    if (rename_open && win_is_open(rename_dbox))
      release_rename();

    saveas_release();
  /*}
  menu_destroy = */ browser_menuopen = FALSE;

  /* This is a bodge to prevent the sublink items getting unattached from the
     main menu when you right-click on "Select All". I can't work out where
     this function is being called from, hence this bidge :( */
  if (rightclickonselectall)
  {
    rightclickonselectall = FALSE;
  }
  else
  {
    /* Release handlers */
    Event_Release(event_MENU,event_ANY,event_ANY,browser_menuselect,reference);
    EventMsg_Release(message_MENUWARN,event_ANY,browser_sublink);
    EventMsg_Release(message_MENUSDELETED,event_ANY,browser_releasemenus);
    help_release_menu();
  }

  return TRUE;
}

BOOL               browser_sublink(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;

  Log(log_DEBUG, "browser_sublink");

  switch (event->data.message.data.menuwarn.selection[0])
  {
    case parent_SAVE:
      browcom_save(browser,event->data.message.data.menuwarn.openpos.x,
                           event->data.message.data.menuwarn.openpos.y,TRUE,
                           FALSE);
      break;
    case parent_CREATE:
      browcom_create(TRUE, event->data.message.data.menuwarn.openpos.x,
      		event->data.message.data.menuwarn.openpos.y, reference);
      break;
    case parent_templat:
      switch (event->data.message.data.menuwarn.selection[1])
      {
        case submenu_COPY:
          browcom_copy(TRUE, event->data.message.data.menuwarn.openpos.x,
      	  	event->data.message.data.menuwarn.openpos.y, reference);
      	  break;
        case submenu_RENAME:
          browcom_rename(TRUE, event->data.message.data.menuwarn.openpos.x,
      	  	event->data.message.data.menuwarn.openpos.y, reference);
      	  break;
      	case submenu_EXPORT:
      	  browcom_export(browser,event->data.message.data.menuwarn.openpos.x,
                           event->data.message.data.menuwarn.openpos.y,TRUE);
      	  break;
      }
      break;
  }

  /* menu_destroy = FALSE; */

  return TRUE;
}

void               browser_savecomplete(void *ref,BOOL successful,BOOL safe,char *filename,
     			  BOOL selection)
{
  browser_fileinfo *browser = ref;

  Log(log_DEBUG, "browser_savecomplete");

  if (successful && safe && !selection)
    browser_settitle(browser,filename,FALSE);
}

void               browser_exportcomplete(void *ref,BOOL successful,BOOL safe,char *filename,
     			  BOOL selection)
{
/*Error_Debug_Printf(0, "Successful: %d, safe: %d, selection: %d",
successful,safe,selection);*/
  Log(log_DEBUG, "browser_exportcomplete");

  if (successful && safe)
  {
    browser_fileinfo *browser = ref;
    strcpy(browser->namesfile, filename);
  }
}

browser_winentry  *browser_copywindow(browser_fileinfo *browser,
		 		     char *identifier,
		 		     browser_winblock **windata)
{
  browser_winentry *winentry;
  int icon;

  Log(log_INFORMATION, "browser_copywindow - copying window %s", LogPreBuffer(identifier));

  if (!browser_overwrite(browser,identifier))
    return NULL;
  winentry = malloc(sizeof(browser_winentry));
  if (!winentry)
  {
    WinEd_MsgTrans_Report(messages,"WinMem",FALSE);
    return NULL;
  }

  if (!flex_alloc((flex_ptr) &winentry->window,
      		 flex_size((flex_ptr) windata)))
  {
    WinEd_MsgTrans_Report(messages,"WinMem",FALSE);
    free(winentry);
    return NULL;
  }

  memcpy(winentry->window,*windata,flex_size((flex_ptr) windata));

  strncpy(winentry->identifier, identifier, sizeof(winentry->identifier) - 1);
  winentry->identifier[sizeof(winentry->identifier) - 1] = '\0';

  LinkList_AddToTail(&browser->winlist,&winentry->header);

  winentry->status = status_CLOSED;
  winentry->iconised = FALSE;
  winentry->fontarray = 0;
  winentry->browser = browser;

  winentry->icon = browser_newicon(browser,browser->numwindows,winentry,FALSE);
  browser->numwindows++;
  browser_settitle(browser,NULL,TRUE);
  browser_sorticons(browser,FALSE,TRUE,FALSE);

  /* Font usage may be increased; this is simple enough, because all fonts
     in copied window already exist for this browser */
  for (icon = 0;icon < (*windata)->window.numicons;icon++)
    if ((*windata)->icon[icon].flags.data.font)
      browser->fontcount[(*windata)->icon[icon].flags.font.handle-1]++;

  return winentry;
}

BOOL               browser_overwrite(browser_fileinfo *browser,char *id)
{
  browser_winentry *winentry = browser_findnamedentry(browser,id);

  Log(log_DEBUG, "browser_overwrite");

  if (winentry)
  {
    os_error message;

    message.errnum = 0;
    MsgTrans_LookupPS(messages,"Overwrt",message.errmess,252,id,0,0,0);
    if (ok_report(&message))
    {
      browser_deletewindow(winentry);
      browser_settitle(browser,NULL,TRUE);
      return TRUE;
    }
    else
      return FALSE;
  }

  return TRUE;
}

browser_winentry  *browser_findselection(browser_fileinfo *browser,int *index,
		 			int max)
{
  Log(log_DEBUG, "browser_findselection");

  for ((*index)++;*index < max;(*index)++)
    if (Icon_GetSelect(browser->window,*index))
    {
      return browser_findwinentry(browser,*index);
    }

  *index = -1;
  return NULL;
}

browser_winentry  *browser_findwinentry(browser_fileinfo *browser,int icon)
{
  browser_winentry *winentry;

  Log(log_DEBUG, "browser_findwinentry");

  for (winentry = LinkList_NextItem(&browser->winlist);
  	 winentry && winentry->icon != icon;
  	 winentry = LinkList_NextItem(winentry));

  return winentry;
}

void               browser_clearselection()
{
  int index;
  icon_handle *selection;

  Log(log_DEBUG, "browser_clearselection");

  if (selection_browser)
  {
    selection = malloc((selection_browser->numwindows + 1)
    	* sizeof(icon_handle));
    if (!selection)
    {
      WinEd_MsgTrans_Report(messages,"BrowMem",FALSE);
      return;
    }
    Wimp_WhichIcon(selection_browser->window, selection,
    	icon_SELECTED, icon_SELECTED);
    for (index = 0;selection[index]!=-1;index++)
      Icon_Deselect(selection_browser->window,selection[index]);
    if (choices->browtools)
      browtools_shadenosel(selection_browser->pane);
  }
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  browser_selection_withmenu = TRUE;
  selection_browser = NULL;
}

void               browcom_clearsel()
{
  Log(log_DEBUG, "browcom_clearsel");

  if (selection_browser)
    browser_clearselection();
  if (selection_viewer)
    viewer_clearselection();
}

void               browcom_selall(browser_fileinfo *browser)
{
  int index;

  Log(log_DEBUG, "browcom_selall");

  browcom_clearsel();
  for (index = 0;index < browser->numwindows;index++)
    Icon_Select(browser->window,index);
  if (choices->browtools)
    browtools_shadeapp(browser);
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  browser_selection_withmenu = FALSE;
  selection_browser = browser;
}

void               browcom_save(browser_fileinfo *browser,int x,int y,BOOL leaf,BOOL seln)
{
  int selection;

  Log(log_DEBUG, "browcom_save");

  selection = (count_selections(browser->window) > 0);
  datatrans_saveas(browser->title,selection,
  		   x,y,leaf,
  		   browser_save_check,browser_savecomplete,browser,seln,FALSE);
}

void               browcom_export(browser_fileinfo *browser,int x,int y,BOOL leaf)
{
  Log(log_DEBUG, "browcom_export");

  datatrans_saveas(browser->namesfile,FALSE,
  		   x,y,leaf,
  		   browser_export,browser_exportcomplete,browser,FALSE,TRUE);
}

void               browcom_delete(browser_fileinfo *browser)
{
  browser_winentry *winentry;
  int index = -1;
  int orig_numwindows = browser->numwindows;

  Log(log_DEBUG, "browcom_delete");

  if (choices->confirm)
  {
    os_error err;
    err.errnum = 0;
    MsgTrans_Lookup(messages,"ConfirmDel",err.errmess,252);
    if (!ok_report(&err))
      return;
  }

  do
  {
    winentry = browser_findselection(browser,&index,orig_numwindows);
    if (winentry)
      browser_deletewindow(winentry);
  }
  while (index != -1);
  browser_sorticons(browser,TRUE,TRUE,FALSE);
  browser_settitle(browser,NULL,TRUE);
}

void               browcom_stats(browser_fileinfo *browser)
{
  Log(log_DEBUG, "browcom_stats");

  stats_open(browser);
}

BOOL               browser_hotkey(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  mouse_block ptrinfo;
  int selections;

  Log(log_DEBUG, "browser_hotkey");

  selections = count_selections(browser->window);
  switch (event->data.key.code)
  {
    case keycode_CTRL_F2:
      if (browser_menuopen)
      {
        /*browser_releasemenus(NULL, browser);*/
        WinEd_CreateMenu((menu_ptr) -1, 0, 0);
      }
      browser_close(browser);
      break;
    case keycode_F3:
      Wimp_GetPointerInfo(&ptrinfo);
      browcom_save(browser,ptrinfo.pos.x,ptrinfo.pos.y,FALSE,FALSE);
      break;
    case keycode_SHIFT_F3:
      if (selections)
      {
        Wimp_GetPointerInfo(&ptrinfo);
        browcom_save(browser,ptrinfo.pos.x,ptrinfo.pos.y,FALSE,TRUE);
      }
      break;
    case keycode_CTRL_F3:
      if (strchr(browser->title, ',') || strchr(browser->title, ':'))
        /* Don't need to check for overwriting another file here as we're only saving to the location we loaded from */
        if (browser_save(browser->title,reference,FALSE))
          browser_settitle(browser,NULL,FALSE);
      break;
      break;
    case 'Z' - 'A' + 1:
      if (selections)
        browcom_clearsel();
      break;
    case 'C' - 'A' + 1:
      if (selections == 1)
        browcom_copy(FALSE, 0, 0, reference);
      break;
    case 'K' - 'A' + 1:
      if (selections)
        browcom_delete(browser);
      break;
    case 'A' - 'A' + 1:
      browcom_selall(browser);
      break;
    case 'R' - 'A' + 1:
      browcom_create(FALSE, 0, 0, reference);
      break;
    case 'S' - 'A' + 1:
      browcom_stats(browser);
      break;
    case 'N' - 'A' + 1:
      if (selections == 1)
        browcom_rename(FALSE, 0, 0, reference);
      break;
    case 'E' - 'A' + 1:
    case 'P' - 'A' + 1:
      if (selections == 1)
        browcom_view(browser,event->data.key.code == 'E'-'A'+1);
      break;
    default:
      Wimp_ProcessKey(event->data.key.code);
      break;
  }
  return TRUE;
}

BOOL browser_scrollevent(event_pollblock *event,void *reference)
{
  scroll_window(event, MARGIN + WIDTH, MARGIN + HEIGHT, MARGIN + ((choices->browtools) ? browtools_paneheight : 0));
  return TRUE;
}

void               browser_claimcaret(browser_fileinfo *browser)
{
  caret_block caret;

  Log(log_DEBUG, "browser_claimcaret");

  caret.window = browser->window;
  caret.icon = -1;
  caret.offset.x = -1000;
  caret.offset.y = 1000;
  caret.height = 40 | (1 << 25);
  caret.index = -1;
  Wimp_SetCaretPosition(&caret);
}

void               browcom_view(browser_fileinfo *browser,BOOL editable)
{
  int index = -1;
  browser_winentry *winentry;

  Log(log_DEBUG, "browcom_view");

  winentry = browser_findselection(browser,&index,browser->numwindows);
  if (winentry == NULL)
    return;

  if (winentry->status)
    viewer_close(winentry);
  viewer_open(winentry,editable);
}

/**
 * Respond to changes in the application choices.
 * 
 * \param *old    The old choices, which had been in effect until the change.
 * \param *new_ch The new choices which will be in effect.
 */

void browser_responder(choices_str *old,choices_str *new_ch)
{
  browser_fileinfo *browser;
  browser_winentry *winentry;

  Log(log_DEBUG, "browser_responder");

  for (browser = LinkList_NextItem(&browser_list);
       browser;
       browser = LinkList_NextItem(browser))
  {
    if (old->browtools && !new_ch->browtools)
    {
      browtools_deletepane(browser);
      browser_sorticons(browser,TRUE,TRUE,FALSE);
    }
    else if (!old->browtools && new_ch->browtools)
    {
      browtools_newpane(browser);
      browser_sorticons(browser,TRUE,TRUE,FALSE);
    }
    else if ((old->browser_sort != new_ch->browser_sort) && Window_IsOpen(browser->window))
    {
      browser_sorticons(browser, TRUE, FALSE, TRUE);
    }

    if (old->hotkeys && !new_ch->hotkeys)
    {
      caret_block caret;

      Wimp_GetCaretPosition(&caret);
      if (caret.window == browser->window)
      {
        caret.window = -1;
        Wimp_SetCaretPosition(&caret);
      }
      browser_createmenus();
    }
    else if (!old->hotkeys && new_ch->hotkeys)
      browser_createmenus();


    for (winentry = LinkList_NextItem(&browser->winlist);
         winentry;
         winentry = LinkList_NextItem(winentry))
    {
      if (winentry->status == status_EDITING)
        viewer_responder(winentry,old,new_ch);
    }
  }
  icndiag_responder(old,new_ch);
  windiag_responder(old,new_ch);
}

BOOL               browser_modechange(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser;
  /* The -1s there are quite a nasty kludge really */
  static int old_xeig = -1;
  static int old_yeig = -1;
  Screen_CacheModeInfo();
  browser_maxnumcolumns = (screen_size.x-MARGIN-32) / (WIDTH + MARGIN);

  Log(log_DEBUG, "browser_modechange");

  for (browser = LinkList_NextItem(&browser_list); browser;
  	browser = LinkList_NextItem(browser))
  {
    if (browser->numcolumns > browser_maxnumcolumns)
    {
      browser->numcolumns = browser_maxnumcolumns;
      browser_sorticons(browser,TRUE,FALSE,TRUE);
    }
    else
      browser_setextent(browser);
    /* Change all on-screen fonts */
    if (screen_eig.y!=old_yeig || (screen_eig.x!=old_xeig && browser->fontinfo))
    {
      browser_winentry *winentry;
      for (winentry = LinkList_NextItem(&browser->winlist);
      	winentry;
      	winentry = LinkList_NextItem(winentry))
      {
        if (winentry->status && winentry->fontarray)
          viewer_changefonts(winentry);
      }
    }
  }
  return TRUE;
}

void               browser_changesparea(browser_winblock *win, void *sparea)
{
  int icon;
  Log(log_DEBUG, "browser_changesparea");

  win->window.spritearea = sparea;
  for (icon = 0;icon < win->window.numicons;icon++)
  {
    if (!win->icon[icon].flags.data.text &&
         win->icon[icon].flags.data.sprite &&
         win->icon[icon].flags.data.indirected)
    {
      win->icon[icon].data.indirectsprite.spritearea = sparea;
    }
  }
}


/**
 * Export the icon names from the selected windows in a browser.
 * 
 * \param *filename   The filename to export to.
 * \param *ref        Pointer to the browser instance of interest.
 * \param selection   Not used.
 * \return            TRUE if successful; otherwise FALSE.
 */
BOOL browser_export(char *filename, void *ref, BOOL selection)
{
  browser_fileinfo *browser = ref;
  int index = -1, type = 0;
  browser_winentry *winentry;
  struct export_handle *export;
  BOOL outcome;
  char *prefix;
  enum export_format format = EXPORT_FORMAT_NONE;
  enum export_flags flags = EXPORT_FLAGS_NONE;

  Log(log_DEBUG, "browser_export");

  type = Icon_WhichRadioInEsg(saveas_export, 1);
  switch(type)
  {
    case saveas_CENUM:
      format = EXPORT_FORMAT_CENUM;
      break;
    case saveas_CDEFINE:
      format = EXPORT_FORMAT_CDEFINE;
      break;
    case saveas_CTYPEDEF:
      format = EXPORT_FORMAT_CTYPEDEF;
      break;
    case saveas_MESSAGES:
      format = EXPORT_FORMAT_MESSAGES;
      break;
    case saveas_BASIC:
      format = EXPORT_FORMAT_BASIC;
      break;
  }

  type = Icon_WhichRadioInEsg(saveas_export, 2);
  switch(type)
  {
    case saveas_UNCHANGED:
      break;
    case saveas_UPPER:
      flags |= EXPORT_FLAGS_UPPERCASE;
      break;
    case saveas_LOWER:
      flags |= EXPORT_FLAGS_LOWERCASE;
      break;
  }

  if (Icon_GetSelect(saveas_export, saveas_SKIPIMPLIED) &&
      (format == EXPORT_FORMAT_CENUM || format == EXPORT_FORMAT_CDEFINE || format == EXPORT_FORMAT_CTYPEDEF))
    flags |= EXPORT_FLAGS_SKIPIMPLIED;

  if (Icon_GetSelect(saveas_export, saveas_USEREAL) && (format == EXPORT_FORMAT_BASIC))
    flags |= EXPORT_FLAGS_USEREAL;

  if (Icon_GetSelect(saveas_export, saveas_PREFIX))
    flags |= EXPORT_FLAGS_PREFIX;

  if (Icon_GetSelect(saveas_export, saveas_WINNAME))
    flags |= EXPORT_FLAGS_WINNAME;

  prefix = Icon_GetTextPtr(saveas_export, saveas_PREFIXFIELD);

  export = export_openfile(format, flags, prefix, filename);
  if (export == NULL)
  {
    WinEd_MsgTrans_ReportPS(messages, "NoExport", FALSE, filename, 0,0,0);
    return FALSE;
  }

  Hourglass_On();

  export_preamble(export);

  do
  {
    winentry = browser_findselection(browser, &index, browser->numwindows);

    if (winentry)
      export_winentry(export, winentry);
  }
  while (index != -1);

  export_postamble(export);

  outcome = export_closefile(export);

  Hourglass_Off();

  if (!outcome)
    WinEd_MsgTrans_ReportPS(messages, "NoExport", FALSE, filename, 0,0,0);

  return outcome;
}

int                browser_estsize(browser_fileinfo *browser)
{
  browser_winentry *winentry;
  int estsize = sizeof(template_header) + 4; /* 4 for terminating zero */

  Log(log_DEBUG, "browser_estsize");

  if (browser->fontinfo)
    estsize += flex_size((flex_ptr) & browser->fontinfo);
  for (winentry = LinkList_NextItem(&browser->winlist);
       winentry;
       winentry = LinkList_NextItem(winentry))
  {
    estsize += sizeof(template_index) +
    		     	flex_size((flex_ptr) &winentry->window);
  }
  return estsize;
}
