/* globals.c */
/* Global variables used by various parts of wined */

#include "choices.h"
#include "common.h"
#include "DeskLib:BackTrace.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:SWI.h"
#include "DeskLib:Environment.h"

/* Set up in browser.c */
extern window_handle overwrite_warn;
extern BOOL overwrite_warn_open;

msgtrans_filedesc *messages;

event_handler menuhandler;

/**
 * The linked list of loaded template files/browser windows.
 */
linklist_header browser_list;

char APPNAME[] = "WinEd";

char log_buffer[1020]; /* A bit smaller than 1K to allow for \\r etc below */
char log_final[1024];
int log_prebuffer = 0;

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
  if (WinEd_Wimp_ReportErrorR(message,0x13,title) == 1)
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
  WinEd_CreateMenu((menu_ptr) -1,0,0);
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
  /* Belt & Braces, to avoid any problems with DeskLib. Ensure that all attempts to close
     the menu tree use "(menu_ptr) -1", and pass X and Y as 0,0. This will prevent
     DeskLib from trying to interpret the NULL pointer if Y is -1. */
  if (m == (menu_ptr) -1 || m == NULL)
  {
    m = (menu_ptr) -1;
    x = 0;
    y = 0;
  }
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

/* Take possibly ctrl-terminated strings and copy them into a separate buffer
 * for use with the Log() function. The pre-buffer is reset when Log() is called.
 * 
 * \param *text   Pointer to a possibly ctrl-terminated string.
 * \return        Pointer to a zero-terminated string which will be valid until
 *                Log() has been called.
 */
char *LogPreBuffer(char *text)
{
  char *start = log_final + log_prebuffer;

  if (text == NULL)
    return NULL;

  while (*text >= ' ' && log_prebuffer < (sizeof(log_final) - 1))
    log_final[log_prebuffer++] = *text++;

  log_final[log_prebuffer++] = '\0';

  return start;
}

void Log(int level, const char *format, ...)
{
  va_list argptr;
  int zero_len;
  BOOL bad_terminator = FALSE;

  va_start(argptr, format);
  vsnprintf(log_buffer, sizeof(log_buffer), format, argptr);
  va_end(argptr);

  /* If the output string runs over the end of the buffer, it will be
     truncated but not null-terminated, so have to account for that: */
  log_buffer[sizeof(log_buffer)-1] = '\0';

  /* Reset the prebuffer now that we've built the log string. */
  log_prebuffer = 0;

  /* Scan the buffer to see if there are any control characters before the terminator.
     Also convert " to ~, to sidestep Reporter's inability to handle " in log messages. */
  for (zero_len = 0; zero_len < sizeof(log_buffer) && log_buffer[zero_len] != '\0'; zero_len++)
  {
    if (log_buffer[zero_len] < ' ')
      bad_terminator = TRUE;
    if (log_buffer[zero_len] == '"')
      log_buffer[zero_len] = '~';
  }

  if (bad_terminator == TRUE)
  {
    char *message = "A bad string termintor was found in the following log message.";
    Debug_Printf("\\R%s", message);
    Environment_LogMessage(log_ERROR, message);
  }

  /* If debugging, output colour-coded reporter text */
  if      (level <= log_ERROR)       snprintf(log_final, sizeof(log_final), "\\r%s", log_buffer); /* red    */
  else if (level <= log_WARNING)     snprintf(log_final, sizeof(log_final), "\\o%s", log_buffer); /* orange */
  else if (level <= log_NOTICE)      snprintf(log_final, sizeof(log_final), "\\b%s", log_buffer); /* blue   */
  else if (level <= log_INFORMATION) snprintf(log_final, sizeof(log_final), "%s",    log_buffer); /* black  */
  else                               snprintf(log_final, sizeof(log_final), "\\f%s", log_buffer); /* grey   */

  Debug_Print(log_final);

  /* Log message (without Reporter colours) with syslog */
  Environment_LogMessage(level, "%s", log_buffer);
}

os_error *WinEd_MsgTrans_ReportPS(msgtrans_filedesc *filedesc, char *token,
                            BOOL fatal,
                            const char *p0,
                            const char *p1,
                            const char *p2,
                            const char *p3)
{
  os_error *swierr;
  char buffer[252];

  swierr = MsgTrans_LookupPS(filedesc, token, buffer, sizeof(buffer), p0, p1, p2, p3);

  if (swierr)
    return (swierr);

  if (fatal)
  {
    Log(log_CRITICAL, buffer);
    Error_ReportFatal(0, buffer);
  }
  else
  {
    Log(log_NOTICE, buffer);
    Error_Report(0, buffer);
  }

  return NULL;
}

/* Just logs the error as well as calling Wimp_ReportError */
int WinEd_Wimp_ReportErrorR(os_error *error, int flags, const char *name)
{
  Log(log_NOTICE, error->errmess);
  return Wimp_ReportErrorR(error, flags, name);
}

/**
 * Extract an icon name from icon definition (The N validation command),
 * copying it into the supplied buffer.
 * 
 * \param *winentry     The window entry containing the icon.
 * \param icon          The icon from which to extract the name.
 * \param *buffer       Pointer to a buffer in which to return the name.
 * \param buflen        The length of the buffer.
 * \return              The length of the name, or 0 if none returned.
 */
int extract_iconname(browser_winentry *winentry, int icon, char *buffer, int buflen)
{
  int valix, n = 0;

  if (buffer == NULL || buflen == 0)
    return 0;

  /* Ensure that the buffer is always returned terminated. */

  buffer[0] = '\0';

  if (  (winentry->window->icon[icon].flags.data.indirected)
     && (winentry->window->icon[icon].flags.data.text) )
  /* Only worth trying if the icon is indirected & text */
  {
    char *validstring = winentry->window->icon[icon].data.indirecttext.validstring +
  	               (int) winentry->window;

    valix = Validation_ScanString(validstring, 'N');
    if (valix)
    {

      for (n = valix; (validstring[n] > 31) && (validstring[n] != ';') && (n-valix < buflen-1); n++)
        buffer[n-valix] = validstring[n];

      buffer[n-valix] = 0;
    }
    else
      return 0;
  }
  else
    return 0;

  /* Return length of name copied over */
  return n - valix;
}

BOOL globals_scrollevent(event_pollblock *event,void *reference)
{
  scroll_window(event, globals_WINDOW_SCROLL, globals_WINDOW_SCROLL, 0);
  return TRUE;
}

/**
 * Handle scrolling in a window.
 *
 * \param *event      Pointer to the event_pollblock for the scroll.
 * \param col_size    The size of a horizontal scroll, in OS units.
 * \param row_size    The size of a vertical scroll, in OS units.
 * \param top_margin  The size of the pane/margin at the top of the window, in OS units.
 */
void scroll_window(event_pollblock *event, int col_size, int row_size, int top_margin)
{
  scroll_rq *scroll = &(event->data.scroll);
  int width, height, error, distance;

  /* Add in the X scroll offset. */

  distance = 0;
  width = scroll->openblock.screenrect.max.x - scroll->openblock.screenrect.min.x;

  switch (scroll->direction.x)
  {
    case 1: /* One Column Right */
      distance = +col_size;
      break;

    case -1: /* One Column Left */
      distance = -col_size;
      break;

    case 2: /* One Page Right */
      distance = +width;
      break;

    case -2: /* One Page Left */
      distance = -width;
      break;

    case 3:
    case -3:
      /* We don't support Auto Scroll. */
      break;

    default: /* Extended Scroll */
      if (scroll->direction.x > 0)
        distance = (scroll->direction.x >> 2) * col_size;
      else if (scroll->direction.x < 0)
        distance = -((-scroll->direction.x) >> 2) * col_size;
      break;
  }

  scroll->openblock.scroll.x += distance;

  /* Add in the Y scroll offset. */

  distance = 0;
  height = (scroll->openblock.screenrect.max.y - scroll->openblock.screenrect.min.y) - top_margin;

  switch (scroll->direction.y)
  {
  case 1: /* One Rown Down */
    distance = +row_size;
    break;

  case -1: /* One Row Up. */
    distance = -row_size;
    break;

  case 2: /* One Page Down */
    distance = +height;
    break;

  case -2: /* One Page Up */
    distance = -height;
    break;

  case 3:
  case -3:
    /* We don't support Auto Scroll. */
    break;

  default: /* Extended Scroll */
    if (scroll->direction.y > 0)
      distance = (scroll->direction.y >> 2) * row_size;
    else if (scroll->direction.y < 0)
      distance = -((-scroll->direction.y) >> 2) * row_size;
    break;
  }

  scroll->openblock.scroll.y += distance;
  if ((error = ((scroll->openblock.scroll.y) % row_size)))
    scroll->openblock.scroll.y -= ((distance > 0) ? row_size : 0) + error;

  Wimp_OpenWindow(&(scroll->openblock));
}

#include "DeskLib:WimpMsg.h"
#include "ide.h"

void test_fn(void)
{
  char bling[50] = "this is a message";
  int ref;

  ref = WimpMsg_Broadcast(event_SENDWANTACK, message_WinEd_IDE_BROADCAST, (void *)bling, sizeof(bling));
}
