/* globals.h*/
/* Global variables used by various parts of wined */

#ifndef __wined_globals_h
#define __wined_globals_h

#include "DeskLib:Event.h"
#include "DeskLib:LinkList.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Wimp.h"
#include <stdarg.h>

#include "common.h"
#include "tempdefs.h"

/* Application name for task manager and menus */
extern char APPNAME[];

/* An extra logging level */
#define log_TEDIOUS 240

/* The size of a standard window scroll lump, is OS units. */
#define globals_WINDOW_SCROLL 20

/* For control of Messages file */
extern msgtrans_filedesc *messages;

/* List of all browser entries known to task */
extern linklist_header browser_list;


/* Open a transient dialogue box centred on screen */
void transient_centre(window_handle window);

/* Count number of selected icons in a window */
int count_selections(window_handle window);

/* Report a message with OK and Cancel buttons, return TRUE if OK pressed,
   FALSE if Cancel pressed */
BOOL ok_report(os_error *message);

/* Find a winentry from its window handle */
browser_winentry *find_winentry_from_window(window_handle window);

/* If caret is in 'closing', put it in 'parent' if it's open */
void return_caret(window_handle closing, window_handle parent);

/* Kill menu tree from an event; if Click, checks it isn't Menu */
BOOL kill_menus(event_pollblock *,void *);

/* Returns TRUE if a window is open */
BOOL win_is_open(window_handle);

/* Makes a submenu open even if its parent entry is shaded */
extern void Menu_SetOpenShaded(menu_ptr parent_menu, int entry);

/* Opens a menu, generating fake message_MENUSDELETED first */
void WinEd_CreateMenu(menu_ptr, int x, int y);

/* Selection conditions */
extern browser_fileinfo *selection_browser;
extern browser_winentry *selection_viewer;

/* Output function for fortify */
void fort_out(const char *string);

/* Take possibly ctrl-terminated strings and copy them into a separate buffer
 * for use with the Log() function. The pre-buffer is reset when Log() is called.
 * 
 * \param *text   Pointer to a possibly ctrl-terminated string.
 * \return        Pointer to a zero-terminated string which will be valid until
 *                Log() has been called.
 */
char *LogPreBuffer(char *text);

/* Syslog (& debug) output */
void Log(int level, const char *format, ...);

/* The same as DeskLib, except syslogs the message */
#define WinEd_MsgTrans_Report(filedesc, token, fatal) \
  WinEd_MsgTrans_ReportPS((filedesc),(token),(fatal),0,0,0,0)

/* The same as DeskLib, except syslogs the message */
extern os_error *WinEd_MsgTrans_ReportPS(msgtrans_filedesc *filedesc, char *token,
                                   BOOL fatal,
                                   const char *p0,
                                   const char *p1,
                                   const char *p2,
                                   const char *p3);

/* Just logs the message, then passes it straight on to Wimp_ReportError */
int WinEd_Wimp_ReportErrorR(os_error *error, int flags, const char *name);

/* Extract icon name from icon definition. Returns length of string, or 0 if no name */
int extract_iconname(browser_winentry *winentry, int icon, char *buffer, int buflen);

/* Handle scroll events generically. */
BOOL globals_scrollevent(event_pollblock *event,void *reference);

/* Handle scrolling in a window. */
void scroll_window(event_pollblock *event, int col_size, int row_size, int pane_size);

/* For debugging */
void test_fn(void);

/* For changing pointer shape when dragging */
#define SWI_Wimp_SPRITEOP 0x400E9

#endif
