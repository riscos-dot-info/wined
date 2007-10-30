/* browser.c */
/* Handles browser windows and management of templat files */

#include <ctype.h>

#include "common.h"

#include "DeskLib:Str.h"
#include "DeskLib:Font.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:KeyCodes.h"

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

/* Dimensions in browser */
#define DEFAULTNUMCOLUMNS 4
#define MAXNUMCOLUMNS browser_maxnumcolumns
#define MINNUMROWS 3
#define MARGIN 8
#define WIDTH 244
#define HEIGHT 44

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
char browser_dialogvalid[] = "Sdialogue";
char browser_windowvalid[] = "Swindow";

/* Menus */
static menu_ptr browser_parentmenu = 0;
static menu_ptr browser_submenu = 0;
static BOOL browser_menuopen = FALSE;

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


void browser_createmenus()
{
  char menutext[256];
  char subtitle[32];

  Debug_Printf("browser_createmenus");

  /* Main menu */
  if (browser_parentmenu)
    Menu_FullDispose(browser_parentmenu);
  if (choices->hotkeys)
    MsgTrans_Lookup(messages,"BrowMenK",menutext,256);
  else
    MsgTrans_Lookup(messages,"BrowMen",menutext,256);
  browser_parentmenu = Menu_New(APPNAME,menutext);
  Menu_SetOpenShaded(browser_parentmenu,parent_templat);

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

static BOOL create_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  Debug_Printf("create_clicked");

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
    MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }
  if (browser_copywindow(browser,buffer,&browser_default))
  {
    browser_settitle(browser,NULL,TRUE);
    browser_sorticons(browser,TRUE,TRUE,FALSE);
  }

  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu(0, -1, -1);
  return TRUE;
}

static void release_create()
{
  Debug_Printf("release_create");

  EventMsg_ReleaseWindow(create_dbox);
  Event_ReleaseWindow(create_dbox);
  help_release_window(create_dbox);
  create_open = FALSE;
}

static BOOL release_create_msg(event_pollblock *e, void *r)
{
  Debug_Printf("release_create_msg");

  release_create();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_create_msg);
  return TRUE;
}

void browcom_create(BOOL submenu, int x, int y,void *reference)
{
  Debug_Printf("browcom_create");

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

static BOOL copy_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  browser_winentry *winentry;
  int index = -1;

  Debug_Printf("copy_clicked");

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
    MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }

  winentry = browser_findselection(browser,&index,browser->numwindows);
  /* Ignore if giving it the same name */
  if (!strcmp(winentry->identifier,buffer))
    return TRUE;
  if (browser_copywindow(browser,buffer,&winentry->window))
  {
    browser_settitle(browser,NULL,TRUE);
    browser_sorticons(browser,TRUE,TRUE,FALSE);
  }

  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu(0, -1, -1);
  return TRUE;
}

static void release_copy()
{
  Debug_Printf("release_copy");

  EventMsg_ReleaseWindow(copy_dbox);
  Event_ReleaseWindow(copy_dbox);
  help_release_window(copy_dbox);
  copy_open = FALSE;
}

static BOOL release_copy_msg(event_pollblock *e, void *r)
{
  Debug_Printf("release_copy_msg");

  release_copy();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_copy_msg);
  return TRUE;
}

void browcom_copy(BOOL submenu, int x, int y,void *reference)
{
  Debug_Printf("browcom_copy(");

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
  Icon_SetText(copy_dbox, 2,
  	browser_findselection(browser,&index,browser->numwindows)->identifier);
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

static BOOL rename_clicked(event_pollblock *event,void *reference)
{
  char buffer[16];
  browser_fileinfo *browser = reference;
  int index;
  browser_winentry *winentry;

  Debug_Printf("rename_clicked");

  if (event->type == event_KEY && event->data.key.code != 13)
  {
    Wimp_ProcessKey(event->data.key.code);
    return TRUE;
  }
  if (event->type == event_CLICK && event->data.mouse.button.data.menu)
    return FALSE;
  if (event->type == event_KEY || event->data.mouse.button.data.select)
    WinEd_CreateMenu(0, -1, -1);
  Icon_GetText(rename_dbox, 2, buffer);
  if (!buffer[0])
  {
    MsgTrans_Report(messages,"NoID",FALSE);
    return TRUE;
  }
  if (!browser_overwrite(browser,buffer))
    return TRUE;
  index = -1;
  winentry = browser_findselection(browser,&index,browser->numwindows);

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

static void release_rename()
{
  Debug_Printf("release_rename");

  EventMsg_ReleaseWindow(rename_dbox);
  Event_ReleaseWindow(rename_dbox);
  help_release_window(rename_dbox);
  rename_open = FALSE;
}

static BOOL release_rename_msg(event_pollblock *e, void *r)
{
  Debug_Printf("release_rename_msg");

  release_rename();
  EventMsg_Release(message_MENUSDELETED, event_ANY, release_rename_msg);
  return TRUE;
}

void browcom_rename(BOOL submenu, int x, int y,void *reference)
{
  int index = -1;
  browser_fileinfo *browser = reference;
  Debug_Printf("browcom_rename");

  if (rename_open)
    release_rename();
  if (!submenu)
  {
    mouse_block ptrinfo;
    Wimp_GetPointerInfo(&ptrinfo);
    x = ptrinfo.pos.x - 64;
    y = ptrinfo.pos.y + 64;
  }
  Icon_SetText(rename_dbox, 2,
  	browser_findselection(browser,&index,browser->numwindows)->identifier);
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

void create_minidboxes()
{
  window_block *templat;

  Debug_Printf("create_minidboxes");

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
static void browser_cr(browser_fileinfo *browser)
{
  browser_winentry *winentry;

  Debug_Printf("\\b browser_cr");

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

      for (i = 0;
      	i < wimp_MAXNAME && winentry->window->window.title.text[i] > 31;
      	++i);
      if (i < wimp_MAXNAME)
        winentry->window->window.title.text[i] = choices->formed ? 13 : 0;
    }
    /* Check each icon */
    for (icon = 0; icon < winentry->window->window.numicons; ++icon)
    {
      if (!winentry->window->icon[icon].flags.data.indirected)
      {
        int i;

        for (i = 0;
        	i < wimp_MAXNAME &&
        		winentry->window->icon[icon].data.text[i] > 31;
        	++i);
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

static BOOL browser_dosave(char *filename,browser_fileinfo *browser,
	BOOL selection,file_handle fp)
{
  template_header header;
  browser_winentry *winentry;
  int offset;

  Debug_Printf("browser_dosave");

  browser_cr(browser);
  Debug_Printf("cr checked");
  /* Save header */
  if (browser->fontinfo)
  {
    /* Calculate eventual offset of font data */
    offset = sizeof(template_header);
    for (winentry = LinkList_NextItem(&browser->winlist); winentry;
         winentry = LinkList_NextItem(winentry))
      if (!selection || Icon_GetSelect(browser->window,winentry->icon))
        offset += sizeof(template_index) +
                  flex_size((flex_ptr) &winentry->window);
    header.fontoffset = offset + 4; /* Skip terminating zero */
  }
  else
    header.fontoffset = -1;
  header.dummy[0] = header.dummy[1] = header.dummy[2] = 0;
  if (File_WriteBytes(fp,&header,sizeof(template_header)))
  {
    if (file_lasterror)
      Error_Check(file_lasterror);
    else
      MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
    return FALSE;
  }

  Debug_Printf("header written");

  /* Work out eventual offset for start off window data */
  offset = sizeof(template_header);
  for (winentry = LinkList_NextItem(&browser->winlist); winentry;
       winentry = LinkList_NextItem(winentry))
    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
      offset += sizeof(template_index);
  offset += 4; /* Skip terminating zero */

  /* Save index entry for each window */
  for (winentry = LinkList_NextItem(&browser->winlist); winentry;
       winentry = LinkList_NextItem(winentry))
  {
    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
    {
      template_index index;
      int i;

      index.offset = offset;
      index.size = flex_size((flex_ptr) &winentry->window);
      index.type = 1;
      /* Don't use strcpycr, we need to keep original terminator */
      for (i = 0; i < 12; i++) /* Titles can only be 12 chars long */
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
          MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        return FALSE;
      }
      offset += index.size;
    }
  }

  /* Save index-terminating zero */
  if (Error_Check(File_Write32(fp,0)))
  {
    return FALSE;
  }

  Debug_Printf("Index written");

  /* Save actual data for each window */
  for (winentry = LinkList_NextItem(&browser->winlist); winentry;
       winentry = LinkList_NextItem(winentry))
    if (!selection || Icon_GetSelect(browser->window,winentry->icon))
    {
      browser_changesparea(winentry->window, (void *) 1);
      if (File_WriteBytes(fp,winentry->window,
                          flex_size((flex_ptr) &winentry->window)))
      {
        if (file_lasterror)
          Error_Check(file_lasterror);
        else
          MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        browser_changesparea(winentry->window, user_sprites);
        return FALSE;
      }
      browser_changesparea(winentry->window, user_sprites);
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
          MsgTrans_ReportPS(messages,"CantSave",FALSE,filename,0,0,0);
        return FALSE;
      }
  }

  Debug_Printf("End of do_save");

  return TRUE;
}

/* Saves all altered files to scrap if it can */
static void browser_safetynet()
{
  BOOL reported = FALSE;
  browser_fileinfo *browser;
  Debug_Printf("browser_safetynet");

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
        MsgTrans_Report(messages,"Safety",FALSE);
        Wimp_CommandWindow(-1);
        /* Create directory */
        if (SWI(5,0,SWI_OS_File,8,browser->title,0,0,0))
          return;
        reported = TRUE;
      }
      strcat(browser->title,".");
      strcat(browser->title,strrchr(tmpnam(NULL),'.')+1);
      fp = File_Open(browser->title,file_WRITE);
      if (fp)
      {
        browser_dosave(browser->title, browser, FALSE, fp);
        browser->altered = FALSE;
        File_Close(fp);
        File_SetType(browser->title, filetype_TEMPLATE);
      }
    }
  }
}

void browser_init(void)
{
  template_block templat;
  char filename[64] = "WinEdRes:Sprites";
  char *tempind = NULL;
  int indsize;

  Debug_Printf("browser_init");

  atexit(browser_safetynet);
  Screen_CacheModeInfo();
  browser_maxnumcolumns = (screen_size.x-MARGIN-32) / (WIDTH + MARGIN);
  if (screen_eig.y == 1)
    strcat(filename,"22");
  browser_sprites = Sprite_LoadFile(filename);

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
    MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
  }
  if (!flex_alloc((flex_ptr) &browser_default,
      		 (int) templat.buffer))
  {
    Wimp_CloseTemplate();
    MsgTrans_ReportPS(messages,"TempMem",TRUE,"Default",0,0,0);
  }
  /* Create temporary buffer for indirected data */
  indsize = (int) templat.workfree;
  if (templat.workfree)
  {
    tempind = malloc(indsize);
    if (!tempind)
    {
      Wimp_CloseTemplate();
      MsgTrans_ReportPS(messages,"TempMem",TRUE,"Default",0,0,0);
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
    MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
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
        MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
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
        MsgTrans_ReportPS(messages,"NoTemp",TRUE,"Default",0,0,0);
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
/*
  int width,height;
*/
  int index;

  Debug_Printf("browser_newbrowser");

  /* Create new info block */
  newfile = malloc(sizeof(browser_fileinfo));
  if (!newfile)
  {
    MsgTrans_Report(messages,"BrowMem",FALSE);
    return NULL;
  }
  newfile->fontinfo = NULL;
  newfile->numfonts = 0;
  for (index = 1;index < 256;index++)
    newfile->fontcount[index] = 0;
  newfile->numwindows = 0;
  newfile->numcolumns = DEFAULTNUMCOLUMNS;
  newfile->altered = FALSE;
  newfile->stats = 0;
  LinkList_AddToTail(&browser_list,&newfile->header);
  LinkList_Init(&newfile->winlist);

  /* Create new browser window */
  MsgTrans_Lookup(messages,"Untitled",newfile->title,256);
  MsgTrans_Lookup(messages,"ExportLeaf",newfile->namesfile,256);
  browser_templat->title.indirecttext.buffer = newfile->title;
  browser_templat->workarearect.min.x = 0;
  browser_templat->workarearect.min.y = - MARGIN -
                 MINNUMROWS * (MARGIN + HEIGHT);
  browser_templat->workarearect.max.x = MARGIN +
  	MAXNUMCOLUMNS * (MARGIN + WIDTH);
  browser_templat->workarearect.max.y = 0;
  if (choices->browtools)
    browser_templat->workarearect.min.y -= browtools_paneheight;
  if (Error_Check(Wimp_CreateWindow(browser_templat,&newfile->window)))
  {
    free(newfile);
    return 0;
  }

  /* Handlers, using browser_fileinfo block as reference */
  Event_Claim(event_OPEN,newfile->window,event_ANY,browser_OpenWindow,
              newfile);
  Event_Claim(event_CLOSE,newfile->window,event_ANY,
  	      browser_closeevent,newfile);
  Event_Claim(event_CLICK,newfile->window,event_ANY,browser_click,newfile);
  Event_Claim(event_KEY,newfile->window,event_ANY,browser_hotkey,newfile);
  EventMsg_Claim(message_DATALOAD,newfile->window,browser_loadhandler,
                 newfile);
  EventMsg_Claim(message_DATASAVE,newfile->window,browser_savehandler,
                 newfile);
  EventMsg_Claim(message_WINDOWINFO,newfile->window,browser_iconise,newfile);
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

BOOL browser_OpenWindow(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  Debug_Printf("browser_OpenWindow");

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

  return TRUE;
}

BOOL browser_iconise(event_pollblock *event,void *reference)
{
  message_block message = event->data.message;
  char *leaf;
  browser_fileinfo *browser = reference;
  browser_winentry *winentry;

  Debug_Printf("browser_iconise");

  /* Ignore iconise messages for other windows
  if (event->data.message.data.windowinfo.window != browser->window)
    return FALSE;*/

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
  Error_Check(Wimp_SendMessage(event_SEND,&message,
                               event->data.message.header.sender,0));

  /* Close all 'children' */
  winentry = LinkList_NextItem(&browser->winlist);
  while (winentry)
  {
    if (winentry->status)
      viewer_close(winentry);
    winentry = LinkList_NextItem(winentry);
  }
  stats_close(browser);

  return TRUE;
}

void browser_close(browser_fileinfo *browser)
{
  browser_winentry *entry;

  Debug_Printf("browser_close");

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

void browser_forceclose(browser_fileinfo *browser)
{
  Debug_Printf("browser_forceclose");

  /* Reset the global variable which tracks if the user clicked on the close icon */
  overwrite_save_from_close = FALSE;

  browser->altered = FALSE;
  browser_close(browser);
}

BOOL browser_closeevent(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  browser_fileinfo *browser = reference;
  BOOL shift;

  Debug_Printf("browser_closeevent");

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

BOOL browser_click(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  BOOL tools;

  Debug_Printf("browser_click");

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
          viewer_close(winentry);
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

BOOL browser_loadhandler(event_pollblock *event,void *reference)
{
  Debug_Printf("browser_loadhandler");

  if (event->data.message.data.dataload.filetype == filetype_TEMPLATE)
  {
    datatrans_load(event,browser_merge,reference);
    return TRUE;
  }
  return FALSE;
}

BOOL browser_savehandler(event_pollblock *event,void *reference)
{
  Debug_Printf("browser_savehandler");

  if (event->data.message.data.datasave.filetype == filetype_TEMPLATE)
  {
    datatrans_saveack(event);
    return TRUE;
  }
  return FALSE;
}

BOOL browser_load(char *filename,int filesize,void *reference)
{
  browser_fileinfo *browser;

  Debug_Printf(" browser_load");

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

      strcpy(spritefile, filename);
      leaf = strrchr(spritefile,'.');
      if (leaf)
      {
        strcpy(leaf+1,"Sprites");
        usersprt_modfilename(spritefile);
        size = File_Size(spritefile);
        if (size > 0)
          usersprt_merge(spritefile,size,0);
      }
    }
    return TRUE;
  }
  else
  {
    browser_close(browser);
    return FALSE;
  }
  return TRUE;
}

BOOL browser_merge(char *filename,int filesize,void *reference)
{
  browser_fileinfo *browser = reference;

  Debug_Printf("browser_merge");

  if (browser_getfile(filename,filesize,browser))
  {
    browser_sorticons(browser,TRUE,TRUE,FALSE);
    browser_settitle(browser,0,TRUE);
    return TRUE;
  }
  return FALSE;
}

typedef enum {
  load_OK,
  load_MemoryError,
  load_FileError,
  load_ReportedError,
  load_IconError
} load_result;

/* Lengths exclude terminators */
typedef struct {
  int buf;
  int buflen;
  int valid;
  int validlen;
} ind_table;


/* icon = real icon # +1, 0 = title because of indtable indexing */
static BOOL browser_resolve_shared(browser_winentry *winentry,
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
  os_error err;
  BOOL broken = FALSE;

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
    err.errnum = 0;
    if (Error_Check(MsgTrans_Lookup(messages,"CorruptInd",err.errmess,252)))
      return load_ReportedError;
    return (Wimp_ReportErrorR(&err, 11, event_taskname) == 2)
           ? load_ReportedError : load_OK;
  }
  return load_OK;
}

/* Check header and read font array */
static load_result browser_load_header(browser_fileinfo *browser,
                                       int *fontoffset,
                                       file_handle fp)
{
  template_header header;

  Debug_Printf("browser_load_header");

  if (File_ReadBytes(fp,&header,sizeof(template_header)))
  {
    return load_FileError;
  }
  if (header.dummy[0] || header.dummy[1] || header.dummy[2])
  {
    os_error err;
    if (Error_Check(MsgTrans_Lookup(messages,"BadHeader",err.errmess,252)))
    {
      return load_ReportedError;
    }
    err.errnum = 0;
    if (Wimp_ReportErrorR(&err, 11 /* OK, Cancel, no prompt */, event_taskname)
    	== 2)
    {
      return load_ReportedError;
    }
  }
  *fontoffset = header.fontoffset;
  return load_OK;
}

static load_result browser_load_fonts(browser_fileinfo *browser,
                                      template_fontinfo **newfontinfo,
                                      int fontoffset, int filesize,
                                      file_handle fp, char *filename)
{
  Debug_Printf("browser_load_fonts");

  if (fontoffset != -1 && fontoffset < filesize)
  {
    if (!flex_alloc((flex_ptr) newfontinfo,filesize - fontoffset))
    {
      MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
      return load_ReportedError;
    }
    if (Error_Check(File_Seek(fp,fontoffset)))
    {
      flex_free((flex_ptr) newfontinfo);
      return load_ReportedError;
    }
    if (File_ReadBytes(fp,*newfontinfo,filesize - fontoffset))
    {
      if (file_lasterror)
        Error_Check(file_lasterror);
      else
        MsgTrans_ReportPS(messages,"BadFileFull",FALSE,filename,0,0,0);
      flex_free((flex_ptr) newfontinfo);
      return load_ReportedError;
    }
  }
  return load_OK;
}

static load_result browser_load_index_entry(template_index *entry, int index,
                                            file_handle fp, char *filename)
{
  Debug_Printf("browser_load_index_entry");

  /* Find index'th index entry in file */
  if (Error_Check(File_Seek(fp,sizeof(template_header) +
                            index * sizeof(template_index))))
  {
    return load_ReportedError;
  }

  /* Read index entry */
  if (File_ReadBytes(fp,entry,sizeof(template_index)) >=
      sizeof(template_index))
  {
    return load_FileError;
  }
  return load_OK;
}

static void browser_make_new_toggle(browser_winentry *winentry)
{
  Debug_Printf("browser_make_new_toggle");

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

static void browser_make_flags_new(browser_winentry *winentry)
{
  /* Conditions handled rather oddly due to Norcroft bug:
     unable to spill registers when debugging enabled... */

  Debug_Printf("browser_make_flags_new");

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

  Debug_Printf("browser_all_indirected");

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
      free(indtable);
      return result;
    }
    for (icon = 0; icon < winentry->window->window.numicons; icon++)
    {
//      Debug_Printf("In loop, icon:%d", icon);
      result = browser_check_indirected(winentry,
                                        &winentry->window->icon[icon].flags,
                                        &winentry->window->icon[icon].data,
                                        indtable, icon, pass);
      if (result)
      {
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
static void browser_process_fonts(browser_fileinfo *browser,
                                  browser_winentry *winentry,
                                  template_fontinfo **newfontinfo)
{
  BOOL reported = FALSE;
  int icon;

  Debug_Printf("browser_process_fonts");

  if (winentry->window->window.titleflags.data.font)
  {
    if (winentry->window->window.titleflags.font.handle *
        sizeof(template_fontinfo) >
        flex_size((flex_ptr) newfontinfo))
    {
      if (!reported)
      {
        MsgTrans_Report(messages,"BadFont",FALSE);
        reported = TRUE;
      }
      winentry->window->window.titleflags.font.handle = 0xef;
      winentry->window->window.titleflags.data.font = 0;
    }
    else
    {
      winentry->window->window.titleflags.font.handle =
        tempfont_findfont(browser,
        &(*newfontinfo)[winentry->window->window.titleflags.font.handle-1]);
    }
  }
  for (icon = 0;icon < winentry->window->window.numicons;icon++)
  {
    if (winentry->window->icon[icon].flags.data.font)
    {
      if (winentry->window->icon[icon].flags.font.handle *
          sizeof(template_fontinfo) >
          flex_size((flex_ptr) newfontinfo))
      {
        if (!reported)
        {
          MsgTrans_Report(messages,"BadFont",FALSE);
          reported = TRUE;
        }
        winentry->window->icon[icon].flags.font.handle = 0xef;
        winentry->window->icon[icon].flags.data.font = 0;
      }
      else
      {
        winentry->window->icon[icon].flags.font.handle =
        tempfont_findfont(browser,
        &(*newfontinfo)[winentry->window->icon[icon].flags.font.handle-1]);
      }
    }
  }
}

BOOL validate_icon_data(icon_block icon)
{
  /* Return FALSE if there's some problem */
  BOOL result = TRUE;

  #ifdef DeskLib_DEBUG
  if (!icon.flags.data.indirected)
    Debug_Printf(" Icon data: %s", icon.data.text);
  #endif

  if (  (icon.workarearect.min.x > icon.workarearect.max.x)
     || (icon.workarearect.min.y > icon.workarearect.max.y) )
  {
    Debug_Printf("\\r  Icon dimensions topsy turvy");
    result = FALSE;
  }

  return result;
}

static load_result browser_verify_data(browser_winentry *winentry)
{
  load_result result = load_OK;
  int icon;

  for (icon = 0; icon < winentry->window->window.numicons; icon++)
  {
    Debug_Printf("Validating icon %d", icon);
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
  #ifdef DeskLib_DEBUG
  char reportertext[wimp_MAXNAME+1];
  #endif

  Debug_Printf("browser_load_templat");

  /* Delete this entry if it already exists */
  winentry = browser_findnamedentry(browser,entry->identifier);
  if (winentry)
  {
    browser_deletewindow(winentry);
  }

  /* Create new window entry block for it */
  winentry = malloc(sizeof(browser_winentry));
  if (!winentry)
  {
    return load_MemoryError;
  }
  LinkList_AddToTail(&browser->winlist,&winentry->header);

  if (strlencr(entry->identifier) >= wimp_MAXNAME)
    MsgTrans_ReportPS(messages,"LongIdent",FALSE,entry->identifier,0,0,0);

  strncpycr(winentry->identifier, entry->identifier, sizeof(winentry->identifier) - 1);
  winentry->identifier[sizeof(winentry->identifier) - 1] = '\0';

  winentry->icon = -1;

  /* Allocate memory and load window/icon data */
  if (!flex_alloc((flex_ptr) &winentry->window,entry->size))
  {
    LinkList_Unlink(&browser->header,&winentry->header);
    free(winentry);
    return load_MemoryError;
  }
  if (File_Seek(fp,entry->offset) ||
      File_ReadBytes(fp,winentry->window,entry->size))
  {
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->header,&winentry->header);
    free(winentry);
    return load_FileError;
  }



  winentry->status = status_CLOSED;
  winentry->fontarray = 0;
  winentry->browser = browser;

  /* Check for old format flags */
  if (!winentry->window->window.flags.data.newflags)
  {
    Debug_Printf("\\B Old format flags found");
    browser_make_flags_new(winentry);
  }

  /* Check indirected data */
  result = browser_all_indirected(browser, winentry, entry);
  if (result)
  {
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->winlist,&winentry->header);
    free(winentry);
    return result;
  }

  /* Verify template data */
  result = browser_verify_data(winentry);
  if (result)
  {
    flex_free((flex_ptr) &winentry->window);
    LinkList_Unlink(&browser->winlist,&winentry->header);
    free(winentry);
    return result;
  }

  browser_changesparea(winentry->window, user_sprites);

  browser_process_fonts(browser, winentry, newfontinfo);

  browser->numwindows++;

  #ifdef DeskLib_DEBUG
  strncpycr(reportertext, winentry->identifier, wimp_MAXNAME-1); /* it's CR terminated... */
//  reportertext[wimp_MAXNAME] = '\0'; /* Or not terminated at all if 12 chars long */
  Debug_Printf("\\d browser '%s' loaded. %d icons.", reportertext, winentry->window->window.numicons);
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

  Debug_Printf("browser_load_from_fp");

  browser_badind_reported = FALSE;
  result = browser_load_header(browser, &fontoffset, fp);
  if (result)
    return result;

  result = browser_load_fonts(browser, &newfontinfo, fontoffset, filesize,
                              fp, filename);
  if (result)
    return result;

  for (index = 0; ; ++index)
  {
    result = browser_load_index_entry(&entry, index, fp, filename);
    if (result)
      break;

    if (entry.offset == 0) /* Finished */
      break;

    if (entry.type != 1)
    {
      char num[8];

      sprintf(num,"%d",entry.type);
      MsgTrans_ReportPS(messages,"NotWindow",FALSE,filename,num,0,0);
    }
    else
    {
      result = browser_load_templat(browser, &entry, &newfontinfo,
                                     fp, filename);
      if (result)
      {
        if (result == load_IconError)
          /* This error might just be this window so carry on loading... */
          MsgTrans_ReportPS(messages,"ContIcon",FALSE,entry.identifier,0,0,0);
        else
          break; /* Stop loading windows */
      }
    }
  }

  if (newfontinfo)
    flex_free((flex_ptr) &newfontinfo);

  if (result)
  {
    /* Allows partial loading */
    if (index)
    {
      if ((result == load_FileError))
        MsgTrans_ReportPS(messages,"BadFilePart",FALSE,filename,0,0,0);
      else if (result == load_MemoryError)
        MsgTrans_ReportPS(messages,"ContMem",FALSE,filename,0,0,0);
      return load_OK;
    }
  }
  return result;
}

BOOL browser_getfile(char *filename,int filesize,browser_fileinfo *browser)
{
  file_handle fp;
  load_result result;
  void *checkanc;

  Debug_Printf("browser_getfile");

  /* Do a quick check to see if there is apparently enough memory */
  if (!flex_alloc(&checkanc,filesize))
  {
     MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
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
      MsgTrans_ReportPS(messages,"WontOpen",FALSE,filename,0,0,0);
    return FALSE;
  }

  result = browser_load_from_fp(browser, fp, filesize, filename);

  File_Close(fp);
  Hourglass_Off();

  switch (result)
  {
    case load_FileError:
      if (file_lasterror)
        Error_Check(file_lasterror);
      else
        MsgTrans_ReportPS(messages,"BadFileFull",FALSE,filename,0,0,0);
      break;
    case load_MemoryError:
      MsgTrans_ReportPS(messages,"LoadMem",FALSE,filename,0,0,0);
      break;
    default:
      break; /* Here to suppress compiler warning about not using load_OK and load_ReportedError */
  }

  return result ? FALSE : TRUE;
}

icon_handle browser_newicon(browser_fileinfo *browser,int index,
                            browser_winentry *winentry,int selected)
{
  icon_createblock newicdata;
  icon_handle newichandle;
  int column,row;
  /* window_redrawblock redraw; */

  Debug_Printf("browser_newicon");

  column = index % browser->numcolumns;
  row = index / browser->numcolumns;
  newicdata.window = browser->window;
  newicdata.icondata.workarearect.min.x = MARGIN + column * (MARGIN + WIDTH);
  newicdata.icondata.workarearect.max.x =
    newicdata.icondata.workarearect.min.x + WIDTH;
  newicdata.icondata.workarearect.max.y = - MARGIN - row * (MARGIN + HEIGHT);
  if (choices->browtools)
    newicdata.icondata.workarearect.max.y -= browtools_paneheight;
  newicdata.icondata.workarearect.min.y =
    newicdata.icondata.workarearect.max.y - HEIGHT;
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
  if (winentry->window->window.flags.data.hscroll ||
      winentry->window->window.flags.data.vscroll)
    newicdata.icondata.data.indirecttext.validstring = browser_windowvalid;
  else
    newicdata.icondata.data.indirecttext.validstring = browser_dialogvalid;

  if (Error_Check(Wimp_CreateIcon(&newicdata,&newichandle)))
    return -1;

  return newichandle;
}

void browser_delwindow(browser_winentry *winentry)
{
  Debug_Printf("browser_delwindow");

  if (winentry->status)
    viewer_close(winentry);
  tempfont_losewindow(winentry->browser,winentry);
  winentry->browser->numwindows--;
  flex_free((flex_ptr) &winentry->window);
  LinkList_Unlink(&winentry->browser->winlist,&winentry->header);
  free(winentry);
}

void browser_deletewindow(browser_winentry *winentry)
{
  Debug_Printf("browser_deletewindow");

  if (winentry->icon != -1)
    Wimp_DeleteIcon(winentry->browser->window,winentry->icon);

  browser_delwindow(winentry);
}

browser_winentry *browser_findnamedentry(browser_fileinfo *browser,char *id)
{
  browser_winentry *winentry;

  Debug_Printf("browser_findnamedentry");

  winentry = LinkList_NextItem(&browser->winlist);
  while (winentry && strcmpcr(winentry->identifier,id))
    winentry = LinkList_NextItem(winentry);
  return winentry;
}

void browser_setextent(browser_fileinfo *browser)
{
  wimp_rect extent;
  Debug_Printf("browser_setextent");

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
}

int alphacomp(const void *one, const void *two)
{
  char lc_one[wimp_MAXNAME + 1];
  char lc_two[wimp_MAXNAME + 1];

  strncpycr(lc_one, (char *)one, wimp_MAXNAME);
  strncpycr(lc_two, (char *)two, wimp_MAXNAME);

  /* If the names are 12 chars long, won't be terminated at all by wimp */
  lc_one[wimp_MAXNAME] = '\0';
  lc_two[wimp_MAXNAME] = '\0';

  Debug_Printf("alphacomp   1:%s, 2:%s", lc_one, lc_two);

  /* make comparison case-insensitive */
  lower_case(lc_one);
  lower_case(lc_two);

  return strcmpcr(lc_one, lc_two);
}

char *lower_case(char *s)
{
//  Debug_Printf("lower_case");

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


void browser_sorticons(browser_fileinfo *browser, BOOL force, BOOL reopen, BOOL keepsel)
{
  int index = 0, ordered;
  browser_winentry *winentry;
  window_state wstate;
  char *selected;
  char names[browser->numwindows][wimp_MAXNAME];

  Debug_Printf("browser_sorticons");

  if (choices->round)
  {
    /* Find list of window names */
    winentry = LinkList_NextItem(&browser->winlist);
    while (winentry)
    {
      strncpycr(names[index], winentry->identifier, wimp_MAXNAME);
      names[index][wimp_MAXNAME - 1] = '\0'; /* Ensure null termination */
      index++;
      winentry = LinkList_NextItem(&winentry->header);
    }
    /* Order list alphabetically */
    qsort(names, browser->numwindows, wimp_MAXNAME, alphacomp);
  }

  if (keepsel)
  {
    selected = malloc(browser->numwindows);
    if (selected)
      for (index=0; index<browser->numwindows; index++)
        selected[index] = Icon_GetSelect(browser->window,index);
  }
  else
    selected = NULL;

  index = 0;
  winentry = LinkList_NextItem(&browser->winlist);
  while (winentry)
  {
    if (winentry->icon != -1 && (winentry->icon != index || force))
    {
      Wimp_DeleteIcon(browser->window,winentry->icon);
      winentry->icon = -1;
    }
    if (winentry->icon == -1)
    {
      if (choices->round)
      { /* Find order for "index" to pass to browser_newicon */
        ordered = 0;
        while ((strcmpcr(names[ordered], winentry->identifier)) && (ordered < browser->numwindows))
          ordered++;
      }
      else
      {
        ordered = index;
      }

      winentry->icon = browser_newicon(browser, ordered, winentry, keepsel&&selected&&selected[index]);
    }

    index++;
    winentry = LinkList_NextItem(&winentry->header);
  }

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
  if (keepsel&&selected)
    free(selected);
}

/* See title.h */
void browser_settitle(browser_fileinfo *browser,char *title,BOOL altered)
{
  Debug_Printf("browser_settitle");

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

BOOL browser_prequit(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = LinkList_NextItem(&browser_list);

  Debug_Printf("browser_prequit");

  while (browser)
  {
    if (browser->altered)
    {
      unsaved_quit(event);
      break;
    }
    browser = LinkList_NextItem(browser);
  }

  return TRUE;
}

void browser_shutdown()
{
  browser_fileinfo *browser;

  Debug_Printf("browser_shutdown");

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

void browser_preselfquit()
{
  browser_fileinfo *browser = LinkList_NextItem(&browser_list);

  Debug_Printf("browser_preselfquit");

  while (browser)
  {
    if (browser->altered)
    {
      unsaved_quit(0);
      return;
    }
    browser = LinkList_NextItem(browser);
  }

  exit(0);
}

/* Save file in strict order for speed */
BOOL browser_save(char *filename,void *reference,BOOL selection)
{
  browser_fileinfo *browser = reference;
  file_handle fp;
  BOOL result;

  Debug_Printf("browser_save");

  Hourglass_On();

  if (!browser->winlist.next)
  {
    MsgTrans_Report(messages,"Empty",FALSE);
    return FALSE;
  }

  fp = File_Open(filename,file_WRITE);
  if (!fp)
  {
    if (file_lasterror)
      Error_Check(file_lasterror);
    else
      MsgTrans_ReportPS(messages,"WontOpen",FALSE,filename,0,0,0);
    return FALSE;
  }

  result = browser_dosave(filename, browser, selection, fp);

  File_Close(fp);
  if (result)
    File_SetType(filename,filetype_TEMPLATE);
  else
    File_Delete(filename);

  Hourglass_Off();

  return TRUE;
}

BOOL browser_save_check(char *filename, void *reference, BOOL selection)
{
  BOOL returnvalue;
  char current_title[256];
  int title_length;

  Debug_Printf("browser_save_check");

  /* Save info to pass on to the overwrite handler */
  strncpy(overwrite_check_filename, filename, 1024);
  overwrite_check_browser = (browser_fileinfo *) reference;
  overwrite_check_selection = selection;

  /* Get existing title - strip trailing " *" if it's there */
  strncpycr(current_title, overwrite_check_browser->title, 256);
  title_length = strlen(current_title);
  if (!strcmp(&current_title[title_length-2], " *"))
    current_title[title_length-2] = '\0';

  /* Check whether filename has changed and exists already */
  if (strcmpcr(current_title, filename) && File_Exists(filename))
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

BOOL overwrite_checkanswer(event_pollblock *event, void *reference)
{
  BOOL overwrite, returnvalue;
  void *ref;

  Debug_Printf("overwrite_checkanswer");

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

static void browser_shademultisel()
{
  Debug_Printf("browser_shademultisel");

  Menu_SetFlags(browser_submenu,submenu_COPY,0,1);
  Menu_SetFlags(browser_submenu,submenu_RENAME,0,1);
  Menu_SetFlags(browser_submenu,submenu_PREVIEW,0,1);
  Menu_SetFlags(browser_submenu,submenu_EDIT,0,1);
}

static void browser_selunshade()
{
  Debug_Printf("browser_selunshade");

  Menu_SetFlags(browser_parentmenu,parent_CLEAR,0,0);
  Menu_SetFlags(browser_parentmenu,parent_templat,0,0);
  Menu_SetFlags(browser_submenu,submenu_DELETE,0,0);
  Menu_SetFlags(browser_submenu,submenu_EXPORT,0,0);
}

void browser_makemenus(browser_fileinfo *browser,int x,int y,
                       icon_handle icon)
{
  char buffer[64];
  int selections;

  Debug_Printf("browser_makemenus");

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
  EventMsg_Claim(message_MENUSDELETED,event_ANY,
  		 browser_releasemenus,browser);
  EventMsg_Claim(message_MENUWARN,event_ANY,browser_sublink,browser);
  help_claim_menu("BRM");
}

BOOL browser_menuselect(event_pollblock *event,void *reference)
{
  mouse_block ptrinfo;
  browser_winentry *winentry;
  int index;
  wimp_point lastmenupos;
  browser_fileinfo *browser = reference;

  Debug_Printf("browser_menuselect");

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
          if (winentry->status)
            viewer_close(winentry);
          viewer_open(winentry,event->data.selection[1] == submenu_EDIT);
          break;
      }
      break;
    case parent_SELALL:
      lastmenupos = menu_currentpos;
      browcom_selall(browser);
      if (ptrinfo.button.data.adjust)
      {
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

BOOL browser_releasemenus(event_pollblock *event,void *reference)
{
  Debug_Printf("browser_releasemenus");

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

  /* Release handlers */
  Event_Release(event_MENU,event_ANY,event_ANY,browser_menuselect,reference);
  EventMsg_Release(message_MENUSDELETED,event_ANY,browser_releasemenus);
  EventMsg_Release(message_MENUWARN,event_ANY,browser_sublink);
  help_release_menu();

  return TRUE;
}

BOOL browser_sublink(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;

  Debug_Printf("browser_sublink");

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

void browser_savecomplete(void *ref,BOOL successful,BOOL safe,char *filename,
     			  BOOL selection)
{
  browser_fileinfo *browser = ref;

  Debug_Printf("browser_savecomplete");

  if (successful && safe && !selection)
    browser_settitle(browser,filename,FALSE);
}

void browser_exportcomplete(void *ref,BOOL successful,BOOL safe,char *filename,
     			  BOOL selection)
{
/*Error_Debug_Printf(0, "Successful: %d, safe: %d, selection: %d",
successful,safe,selection);*/
  Debug_Printf("browser_exportcomplete");

  if (successful && safe)
  {
    browser_fileinfo *browser = ref;
    strcpy(browser->namesfile, filename);
  }
}

browser_winentry *browser_copywindow(browser_fileinfo *browser,
		 		     char *identifier,
		 		     browser_winblock **windata)
{
  browser_winentry *winentry;
  int icon;

  Debug_Printf("browser_copywindow");

  if (!browser_overwrite(browser,identifier))
    return NULL;
  winentry = malloc(sizeof(browser_winentry));
  if (!winentry)
  {
    MsgTrans_Report(messages,"WinMem",FALSE);
    return NULL;
  }

  if (!flex_alloc((flex_ptr) &winentry->window,
      		 flex_size((flex_ptr) windata)))
  {
    MsgTrans_Report(messages,"WinMem",FALSE);
    free(winentry);
    return NULL;
  }

  memcpy(winentry->window,*windata,flex_size((flex_ptr) windata));

  strncpy(winentry->identifier, identifier, sizeof(winentry->identifier) - 1);
  winentry->identifier[sizeof(winentry->identifier) - 1] = '\0';

  LinkList_AddToTail(&browser->winlist,&winentry->header);

  winentry->status = status_CLOSED;
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

BOOL browser_overwrite(browser_fileinfo *browser,char *id)
{
  browser_winentry *winentry = browser_findnamedentry(browser,id);

  Debug_Printf("browser_overwrite");

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

browser_winentry *browser_findselection(browser_fileinfo *browser,int *index,
		 			int max)
{
  Debug_Printf("browser_findselection");

  for ((*index)++;*index < max;(*index)++)
    if (Icon_GetSelect(browser->window,*index))
    {
      return browser_findwinentry(browser,*index);
    }

  *index = -1;
  return NULL;
}

browser_winentry *browser_findwinentry(browser_fileinfo *browser,int icon)
{
  browser_winentry *winentry;

  Debug_Printf("browser_findwinentry");

  for (winentry = LinkList_NextItem(&browser->winlist);
  	 winentry && winentry->icon != icon;
  	 winentry = LinkList_NextItem(winentry));
  return winentry;
}

void browser_clearselection()
{
  int index;
  icon_handle *selection;

  Debug_Printf("browser_clearselection");

  if (selection_browser)
  {
    selection = malloc((selection_browser->numwindows + 1)
    	* sizeof(icon_handle));
    if (!selection)
    {
      MsgTrans_Report(messages,"BrowMem",FALSE);
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

void browcom_clearsel()
{
  Debug_Printf("browcom_clearsel");

  if (selection_browser)
    browser_clearselection();
  if (selection_viewer)
    viewer_clearselection();
}

void browcom_selall(browser_fileinfo *browser)
{
  int index;

  Debug_Printf("browcom_selall");

  browcom_clearsel();
  for (index = 0;index < browser->numwindows;index++)
    Icon_Select(browser->window,index);
  if (choices->browtools)
    browtools_shadeapp(browser);
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  browser_selection_withmenu = FALSE;
  selection_browser = browser;
}

void browcom_save(browser_fileinfo *browser,int x,int y,BOOL leaf,BOOL seln)
{
  int selection;

  Debug_Printf("browcom_save");

  selection = (count_selections(browser->window) > 0);
  datatrans_saveas(browser->title,selection,
  		   x,y,leaf,
  		   browser_save_check,browser_savecomplete,browser,seln,FALSE);
}

void browcom_export(browser_fileinfo *browser,int x,int y,BOOL leaf)
{
  Debug_Printf("browcom_export");

  datatrans_saveas(browser->namesfile,FALSE,
  		   x,y,leaf,
  		   browser_export,browser_exportcomplete,browser,FALSE,TRUE);
}

void browcom_delete(browser_fileinfo *browser)
{
  browser_winentry *winentry;
  int index = -1;
  int orig_numwindows = browser->numwindows;

  Debug_Printf("browcom_delete");

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

void browcom_stats(browser_fileinfo *browser)
{
  Debug_Printf("browcom_stats");

  stats_open(browser);
}

BOOL browser_hotkey(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser = reference;
  mouse_block ptrinfo;
  int selections;

  Debug_Printf("browser_hotkey");

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

void browser_claimcaret(browser_fileinfo *browser)
{
  caret_block caret;

  Debug_Printf("browser_claimcaret");

  caret.window = browser->window;
  caret.icon = -1;
  caret.offset.x = -1000;
  caret.offset.y = 1000;
  caret.height = 40 | (1 << 25);
  caret.index = -1;
  Wimp_SetCaretPosition(&caret);
}

void browcom_view(browser_fileinfo *browser,BOOL editable)
{
  int index = -1;
  browser_winentry *winentry;

  Debug_Printf("browcom_view");

  winentry = browser_findselection(browser,&index,browser->numwindows);
  if (winentry->status)
    viewer_close(winentry);
  viewer_open(winentry,editable);
}

void browser_responder(choices_str *old,choices_str *new_ch)
{
  browser_fileinfo *browser;
  browser_winentry *winentry;

  Debug_Printf("browser_responder");

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

BOOL browser_modechange(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser;
  /* The -1s there are quite a nasty kludge really */
  static int old_xeig = -1;
  static int old_yeig = -1;
  Screen_CacheModeInfo();
  browser_maxnumcolumns = (screen_size.x-MARGIN-32) / (WIDTH + MARGIN);

  Debug_Printf("browser_modechange");

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

void browser_changesparea(browser_winblock *win, void *sparea)
{
  int icon;
  Debug_Printf("browser_changesparea");

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

BOOL browser_export(char *filename, void *ref, BOOL selection)
{
  browser_fileinfo *browser = ref;
  int index = -1;
  browser_winentry *winentry;
  FILE *fp;
  char buffer[256], pre[3], buffy[256];

  Debug_Printf("browser_export");

  if (Icon_GetSelect(saveas_export, 4))
    strcpy(pre, "E_");
  else
    strcpy(pre, "M_");

  fp = fopen(filename, "w");
  if (!fp)
  {
    MsgTrans_ReportPS(messages, "NoExport", FALSE, filename, 0,0,0);
    return FALSE;
  }

  Hourglass_On();

  snprintf(buffy, 256, "%sPreamble", pre);
  MsgTrans_Lookup(messages, buffy, buffer, sizeof(buffer));
  export_puts(fp, buffer);

  do
  {
    winentry = browser_findselection(browser,&index,browser->numwindows);

    if (winentry)
      export_winentry(fp, winentry, pre);
  }
  while (index != -1);

  snprintf(buffy, 256, "%sPostamble", pre);
  MsgTrans_Lookup(messages, buffy, buffer, sizeof(buffer));
  export_puts(fp, buffer);

  index = fclose(fp);

  Hourglass_Off();

  if (index != EOF)
  {
    File_SetType(filename, 0xfff);
    return TRUE;
  }
  else
  {
    File_Delete(filename);
    MsgTrans_ReportPS(messages, "NoExport", FALSE, filename, 0,0,0);
    return FALSE;
  }
}

int browser_estsize(browser_fileinfo *browser)
{
  browser_winentry *winentry;
  int estsize = sizeof(template_header) + 4; /* 4 for terminating zero */

  Debug_Printf("browser_estsize");

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
