/* wined.c */
/* Main task handling bits */

/* And some add-on debugging stuff */

#include "common.h"

#include "kernel.h"

#include "DeskLib:Hourglass.h"
#include "DeskLib:Kbd.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:KeyCodes.h"
#include "DeskLib:Resource.h"
#include "DeskLib:SWI.h"
#include "DeskLib:Time.h"
#include "DeskLib:Debug.h"
#include "DeskLib:Environment.h"

#include "browser.h"
#include "choices.h"
#include "datatrans.h"
#include "deskfont.h"
#include "globals.h"
#include "monitor.h"
#include "picker.h"
#include "uri.h"
#include "usersprt.h"
#include "shortcuts.h"

#ifdef HAVE_MEMCHECK
#include "MemCheck:MemCheck.h"
#endif

#define app_VERSION "3.201ß (September 2008)"

extern void __heap_checking_on_all_allocates(int);
extern void __heap_checking_on_all_deallocates(int);

/* --------------------- Definitions ------------------------------ */

/* ProgInfo icons */
#define proginfo_VERSION 0
#define proginfo_USER 1
#define proginfo_EMAIL 8
#define proginfo_WEB 9

/* ------------------ Globals not used elsewhere ------------------ */

/* Icon bar menu */
static menu_ptr iconbar_menu;
#define ibmenu_INFO 0
#define ibmenu_HELP 1
#define ibmenu_PICKER 2
#define ibmenu_MONITOR 3
#define ibmenu_CHOICES 4
#define ibmenu_CLEARSPR 5
#define ibmenu_QUIT 6

/* Whether templates file is open */
static BOOL wined_templatesopen;

/* ProgInfo window */
window_handle proginfo_window;

/* -------------------- List of functions ------------------------- */

/* Initialise ProgInfo dialogue box and its handlers */
window_handle proginfo_init(void);

/* Initialise wined; maxmem is max memory size to use for DA in 4K blocks */
void wined_initialise(int maxmem);

/* Handlers for icon bar icon */
BOOL iconbar_click(event_pollblock *event,void *ref);
BOOL load_handler(event_pollblock *event,void *ref);
BOOL iconbar_dataopen(event_pollblock *event,void *ref);

/* Handler for icon bar menu */
BOOL iconbar_menuclick(event_pollblock *event,void *ref);

/* Remove iconbar menu handlers */
BOOL iconbar_closemenu(event_pollblock *event,void *ref);

/* Respond to save events (loading from another app) */
BOOL save_handler(event_pollblock *event,void *ref);

/* exit(0) when message_QUIT received */
BOOL wined_quitter(event_pollblock *event,void *reference);

/* Attempt to ensure iconise protocol works correctly */
BOOL wined_ignore_ic(event_pollblock *event,void *reference);

/* Title to use for errors */
extern char error_title[40];

/* Close Messages file */
void wined_closemessages(void);

/* ---------------------------------------------------------------- */


void wined_closemessages()
{
  MessageTrans_CloseFile(messages);
  if (wined_templatesopen)
    Wimp_CloseTemplate();
}

BOOL iconbar_click(event_pollblock *event,void *ref)
{
  switch (event->data.mouse.button.value)
  {
    case button_MENU:
      WinEd_CreateMenu(iconbar_menu,event->data.mouse.pos.x,-1);
      Event_Claim(event_MENU,event_ANY,event_ANY,iconbar_menuclick,0);
      EventMsg_Claim(message_MENUSDELETED,event_ANY,iconbar_closemenu,0);
      help_claim_menu("IBM");
      return TRUE;
    case button_ADJUST:
      if (File_Exists("Choices:WinEd.Shortcuts"))
      {
        /* If shortcuts_createmenu succeeds it'll display a menu. Otherwise, open new browser */
        if (!shortcuts_createmenu(event->data.mouse.pos.x))
          browser_newbrowser();
      }
      else
        browser_newbrowser();
      return TRUE;
    case button_SELECT:
      #ifdef DeskLib_DEBUG
        browser_preselfquit();
       #else
        browser_newbrowser();
      #endif
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

BOOL iconbar_menuclick(event_pollblock *event,void *ref)
{
  mouse_block ptrinfo;

  Wimp_GetPointerInfo(&ptrinfo);

  switch (event->data.selection[0])
  {
    case ibmenu_INFO:
      #ifdef DeskLib_DEBUG
        test_fn();
      #endif
      break;
    case ibmenu_HELP:
      Wimp_StartTask("Run <WinEd$Dir>.!Help");
      break;
    case ibmenu_PICKER:
      picker_open();
      break;
    case ibmenu_MONITOR:
      monitor_open();
      break;
    case ibmenu_CHOICES:
      choices_open();
      break;
    case ibmenu_CLEARSPR:
      usersprt_clear();
      break;
    case ibmenu_QUIT:
      browser_preselfquit();
      break;
  }

  if (ptrinfo.button.data.adjust)
    Menu_ShowLast();

  return TRUE;
}

BOOL iconbar_closemenu(event_pollblock *event,void *reference)
{
  Event_Release(event_MENU,event_ANY,event_ANY,iconbar_menuclick,0);
  EventMsg_Release(message_MENUSDELETED,event_ANY,iconbar_closemenu);
  help_release_menu();
  return TRUE;
}

BOOL load_handler(event_pollblock *event,void *ref)
{
  if (event->data.message.data.dataload.filetype == filetype_TEMPLATE &&
  	event->data.message.data.dataload.window == window_ICONBAR)
    datatrans_load(event,browser_load,NULL);
  else if (event->data.message.data.dataload.filetype == FILETYPE_Sprites)
  {
    if (Kbd_KeyDown(inkey_SHIFT))
    {
      /* Merge file with Wimp pool and redraw whole screen */
      int r[10];
      window_redrawblock redraw;

      r[0] = 11;
      r[2] = (int) event->data.message.data.dataload.filename;
      if (!Error_Check(Wimp_SpriteOp(r)))
      {
        Screen_CacheModeInfo();
        redraw.window = -1;
        redraw.rect.min.x = redraw.rect.min.y = 0;
        redraw.rect.max = screen_size;
        Wimp_ForceRedraw(&redraw);
      }
    }
    else
    {
      datatrans_load(event,usersprt_merge,NULL);
    }
  }
  return TRUE;
}

BOOL iconbar_dataopen(event_pollblock *event,void *reference)
{
  /* DataOpen is equivalent to DataLoad */
  if (event->data.message.data.dataload.filetype == filetype_TEMPLATE)
  {
    int size;

    message_block message = event->data.message;

    message.header.yourref = message.header.myref;
    message.header.action = message_DATALOADACK;
    Error_Check(Wimp_SendMessage(event_SEND,&message,
    				 event->data.message.header.sender,0));
    size = File_Size(event->data.message.data.dataload.filename);
    if (size)
      browser_load(event->data.message.data.dataload.filename,size,0);
    else
      WinEd_MsgTrans_ReportPS(messages,"NotFile",FALSE,
      			event->data.message.data.dataload.filename,0,0,0);

    if (!strcmp(message.data.dataload.filename,"<Wimp$Scrap>"))
      File_Delete(message.data.dataload.filename);

    return TRUE;
  }
  return FALSE;
}

BOOL save_handler(event_pollblock *event,void *ref)
{
  if ((event->data.message.data.datasave.filetype == filetype_TEMPLATE &&
  	event->data.message.data.datasave.window == window_ICONBAR) ||
  	event->data.message.data.datasave.filetype == FILETYPE_Sprites)
    datatrans_saveack(event);
  return TRUE;
}

static BOOL proginfo_email(event_pollblock *event,void *ref)
{
  uri_send("mailto:riscos@snowstone.org.uk");
  return TRUE;
}

static BOOL proginfo_web(event_pollblock *event,void *ref)
{
  uri_send("http://www.riscos.info/index.php?title=WinEd");
  return TRUE;
}

window_handle proginfo_init()
{
  window_handle proginfo;
  window_block *windef;

  Log(log_DEBUG, "proginfo_init");

  windef = templates_load("ProgInfo",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(windef,&proginfo));
  free(windef);

  Icon_SetText(proginfo, proginfo_VERSION, app_VERSION);

  Event_Claim(event_OPEN,proginfo,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,proginfo,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,proginfo,proginfo_EMAIL,proginfo_email,0);
  Event_Claim(event_CLICK,proginfo,proginfo_WEB,proginfo_web,0);
  help_claim_window(proginfo,"ATP");

  return proginfo;
}

void wined_initialise(int maxmem)
{
  char buffer[256], buffy[256];
  int taskmanager_call;
  icon_handle baricon;
  os_error *error;
  int wined_wimpmessages[] = {message_DATALOAD,message_DATALOADACK,
                              message_DATASAVE,message_DATASAVEACK,
                              message_DATAOPEN,
                              message_PREQUIT,
                              /*message_MENUSDELETED,*/message_MENUWARN,
                              message_WINDOWINFO,
                              message_HELPREQUEST,
                              message_MODECHANGE,
                              Message_FontChanged,
                              message_URI,
                              0};
  /* message_MENUSDELETED now only generated internally to avoid confusion
     over which menu tree it's actually for */



  /* Work out whether to support nested wimp */
  SWI(1, 1, 0x400f2, 7, &taskmanager_call);
  if (taskmanager_call >= 380)
    taskmanager_call = 380;
  else
    taskmanager_call = 310;

  /* Start task and trap unhandled events */
  Event_Initialise3(APPNAME,taskmanager_call,wined_wimpmessages);
  EventMsg_Initialise();

  Debug_Initialise(debug_REPORTER);
  Environment_LogInitialise(APPNAME);

  Debug_Printf("\\b");
  Log(log_NOTICE, "WinEd starting up, \\t, %s.", app_VERSION);
  Log(log_NOTICE, "  OS:%d, DeskLib:%d, Wimp:%d", Environment_OSVersion(), DESKLIB_VERSION, event_wimpversion);
  Log(log_TEDIOUS, "...detailed debugging");

Log(log_TEDIOUS, "LOGGING TEST.4567890123456789012345678501234567890123456789012345678901234567890123456710012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345671001234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567100123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456710012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345671001234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567100123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456710012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345671001234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567100123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456100012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234561200123456789012345678901234561230");

  #ifdef FORTIFY
    Fortify_SetOutputFunc(fort_out);
  #endif

  strncpy(resource_pathname, "WinEdRes:", 30);
  resource_pathname[30] = '\0';               /* Ensure string is terminated */

  /* Load Messages file */
  sprintf(buffer,"%sMessages",resource_pathname);
  Error_CheckFatal(MsgTrans_LoadFile(&messages,buffer));
  atexit(wined_closemessages);

  /* Set up error title for subsequent non-fatal errors */
  MsgTrans_Lookup(messages,"MsgFrom:Message from WinEd",error_title,40);

  /* Check we're not already loaded */
  if (Environment_TaskIsActive(APPNAME))
  {
    MsgTrans_Lookup(messages, "Already", buffer, sizeof(buffer));
    /* Launch multitasking error box if we're already active */
    snprintf(buffy, sizeof(buffy), "WinEdRes:MultiError -t WinEdRes:Templates -e %s", buffer);
    Wimp_StartTask(buffy);
    Hourglass_Off();
    exit(EXIT_FAILURE);
  }

  flex_init("WinEd",NULL,maxmem); /* Old WinEd used Castle flex. This version uses ROL flex */

  /* Set up user sprite area */
  usersprt_init();

  deskfont_init();

  /* Open templates */
  sprintf(buffer,"%sTemplates",resource_pathname);
  Error_CheckFatal(Wimp_OpenTemplate(buffer));
  wined_templatesopen = TRUE;

  /* Load all windows from templates */
  proginfo_window = proginfo_init();
  browser_init();
  saveas_init();
  picker_init();
  monitor_init();
  uri_init();

  /* Close templates */
  Wimp_CloseTemplate();
  wined_templatesopen = FALSE;

  /* Icon bar menu */
  error = MsgTrans_Lookup(messages,"IBMenu",buffer,256);
  iconbar_menu = Menu_New(APPNAME,buffer);
  if (!iconbar_menu || error)
    WinEd_MsgTrans_Report(messages,"NoMenu",TRUE);
  Menu_AddSubMenu(iconbar_menu,ibmenu_INFO,(menu_ptr) proginfo_window);

  /* Put icon on icon bar */
  baricon = Icon_BarIcon("!wined",iconbar_RIGHT);
  Event_Claim(event_CLICK,window_ICONBAR,baricon,iconbar_click,0);
  EventMsg_Claim(message_DATALOAD,event_ANY,load_handler,0);
  EventMsg_Claim(message_DATAOPEN,window_ICONBAR,iconbar_dataopen,0);
  EventMsg_Claim(message_DATASAVE,event_ANY,save_handler,0);
  help_claim_window(window_ICONBAR,"IBI");

  EventMsg_Claim(message_QUIT,event_ANY,wined_quitter,0);
  EventMsg_Claim(message_WINDOWINFO,event_ANY,wined_ignore_ic,0);
}

BOOL wined_quitter(event_pollblock *event,void *reference)
{
  browser_userquit = TRUE;
  exit(0);
  return TRUE;
}

BOOL wined_ignore_ic(event_pollblock *event,void *reference)
{
  return TRUE;
}

int main(int argc,char **argv)
{
/*
  __heap_checking_on_all_allocates(1);
  __heap_checking_on_all_deallocates(1);
*/
#ifdef HAVE_MEMCHECK
  fprintf(stderr, "Enabling MemCheck\n");
  MemCheck_Init();
  MemCheck_RegisterArgs(argc, argv);
  MemCheck_InterceptSCLStringFunctions();
#endif
  Hourglass_On();

  /* Disable MemCheck for initialisation to prevent stderr clutter
     caused by badly written but harmless DeskLib/Resource code */
#ifdef HAVE_MEMCHECK
  MemCheck_SetReadChecking(0);
#endif

  wined_initialise(atoi(argv[1])*1024);

#ifdef HAVE_MEMCHECK
  MemCheck_SetReadChecking(1);
#endif

  /* Check whether to load file straight away */
  if (argc == 3)
    browser_load(argv[2],File_Size(argv[2]),0);

  Hourglass_Off();

  Log(log_INFORMATION, "WinEd initialised...");

  while (TRUE)
  {
    if (monitor_isactive)
    {
      Wimp_PollIdle(event_mask, &event_lastevent, Time_Monotonic()+25);
    }
    else
    {
      Wimp_Poll(event_mask, &event_lastevent);
    }
    if (event_lastevent.type <= 19 && event_lastevent.type >= 0)
      Event_Process(&event_lastevent);
  }
}


