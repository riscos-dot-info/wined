/* globals.c */
/* Global variables used by various parts of wined */

#include "choices.h"
#include "common.h"
#include "DeskLib:BackTrace.h"

/* Set up in browser.c */
extern window_handle overwrite_warn;
extern BOOL overwrite_warn_open;

msgtrans_filedesc *messages;

event_handler menuhandler;

linklist_header browser_list;

char APPNAME[] = "WinEd";

void transient_centre(window_handle window)
{
  window_state wstate;
  int x,y;

  Error_Check(Wimp_GetWindowState(window,&wstate));
  Screen_CacheModeInfo();
  x = (screen_size.x +
       wstate.openblock.screenrect.min.x-wstate.openblock.screenrect.max.x) /
      2;
  y = (screen_size.y -
       wstate.openblock.screenrect.min.y+wstate.openblock.screenrect.max.y) /
      2;
  WinEd_CreateMenu((menu_block *) window,x+64,y);
}

int count_selections(window_handle window)
{
  window_info winfo;
  icon_handle icon;
  int number = 0;

  Window_GetInfo3(window,&winfo);
  winfo.window = window;

  for (icon = 0;icon < winfo.block.numicons;icon++)
    if (Icon_GetSelect(window,icon))
      number++;

  return number;
}

BOOL ok_report(os_error *message)
{
  char title[64];

  MsgTrans_Lookup(messages,"MsgFrom",title,64);
  if (Wimp_ReportErrorR(message,0x13,title) == 1)
    return TRUE;

  return FALSE;
}

browser_winentry *find_winentry_from_window(window_handle window)
{
  browser_fileinfo *browser;
  browser_winentry *winentry;

  for (browser = LinkList_NextItem(&browser_list);
       browser;
       browser = LinkList_NextItem(browser))
  {
    for (winentry = LinkList_NextItem(&browser->winlist);
    	 winentry;
    	 winentry = LinkList_NextItem(winentry))
    {
      if (winentry->handle == window && winentry->status == status_EDITING)
        return winentry;
    }
  }

  return NULL;
}

void return_caret(window_handle closing, window_handle parent)
{
  caret_block caret;
  window_state wstate;
  if (!choices->hotkeys)
    return;
  Wimp_GetCaretPosition(&caret);
  if (caret.window != closing)
    return;
  Wimp_GetWindowState(parent, &wstate);
  if (!wstate.flags.data.open)
    return;
  caret.window = parent;
  caret.icon = -1;
  caret.offset.x = -1000;
  caret.offset.y = 1000;
  caret.height = 40 | (1 << 25);
  caret.index = -1;
  Wimp_SetCaretPosition(&caret);
}

BOOL kill_menus(event_pollblock *event, void *ref)
{
  if (event->type == event_CLICK &&
  	event->data.mouse.button.value == button_MENU)
    return FALSE;
  WinEd_CreateMenu(0,-1,-1);
  return TRUE;
}

BOOL win_is_open(window_handle wh)
{
  window_state wstate;
  Wimp_GetWindowState(wh, &wstate);
  return (wstate.flags.data.open);
}

void Menu_SetOpenShaded(menu_ptr menu, int entry)
{
  menu_item *item = (menu_item *) (((int) menu) + sizeof(menu_block));

  item = &item[entry];
  item->menuflags.data.openshaded = 1;
}

browser_fileinfo *selection_browser = NULL;
browser_winentry *selection_viewer = NULL;

void WinEd_CreateMenu(menu_ptr m, int x, int y)
{
  /* This is a bodge to avoid mysterious problems with receiving menusdeleted messages which
     result in the overwrite-warning dialogue getting closed but other problems arising when
     the overwrite_warn transient is closed but the close-handler (overwrite_checkanswer) isn't
     called - e.g. when the window closes as a result of an external mouse click */
  if ((!overwrite_warn_open) || !Window_IsOpen(overwrite_warn))
  {
    event_pollblock event;

    event.type = event_USERMESSAGE;
    event.data.message.header.size = sizeof(message_header);
    event.data.message.header.sender = event_taskhandle;
    event.data.message.header.myref = event.data.message.header.yourref = 0;
    event.data.message.header.action = message_MENUSDELETED;
    Event_Process(&event);
    Menu_Show(m, x, y);
  }
  overwrite_warn_open = FALSE;
}

void fort_out(const char *string)
{
  Debug_Printf("\\O%s",string+1);
}

void test_fn(void)
{
  BackTrace_functionlist *list;
  Debug_Printf("Backtrace");
  BackTrace_OutputToStdErr();

//  list = BackTrace_GetCurrentFunctions();
//  Debug_Printf("1) %s",list->functions[1]);
}
