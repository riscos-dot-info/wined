/* shortcuts.c */

#include "shortcuts.h"
#include "common.h"
#include "browser.h"

/* This file constructs and responds to the shortcuts menus.
   Note current restrictions:
    - Max ten items
    - Max item value length shortcut_VALUELEN (128)
    - Max menu definition length 256
    - Max value list length 1024
*/

/* Respond to shortcuts menu select */
BOOL shortcuts_menuselect(event_pollblock *event, void *ref);

/* Shortcuts menu */
menu_ptr shortcuts_menu = NULL;
char shortcut_values[10][shortcut_VALUELEN];

BOOL shortcuts_createmenu(int mousex)
{
  char menudef[256], shortcutvals[1024];
  char shortcutsmenu_title[40];
  msgtrans_filedesc *shortcuts;
  os_error *error;
  int i;
  char *p;

  Log(log_DEBUG, "shortcuts_createmenu");

  /* Load messages-format shortcuts file */
  error = MsgTrans_LoadFile(&shortcuts,"Choices:WinEd.Shortcuts");
  if (error)
  {
    Log(log_INFORMATION, " Error opening shortcuts file"); /* Could just be it's not there */
    return FALSE;
  }

  /* Load tokens */
  error             = MsgTrans_Lookup(shortcuts, "Menu",   menudef,      sizeof(menudef));
  Log(log_DEBUG, " Menu definition: %s", menudef);
  if (!error) error = MsgTrans_Lookup(shortcuts, "Values", shortcutvals, sizeof(shortcutvals));
  Log(log_DEBUG, " Values: %s", shortcutvals);
  MsgTrans_LoseFile(shortcuts);

  if (error)
  {
    Log(log_WARNING, " Error looking up tokens"); /* File isn't of correct format */
    return FALSE;
  }

  /* Fill the values array */
  i = 0;
  p = strtok(shortcutvals, ",");
  while (p && (i<10))
  {
    Log(log_DEBUG, " ->%s", p);
    strncpy(shortcut_values[i], p, shortcut_VALUELEN);
    i++;
    p = strtok(NULL, ",");
  }
  /* Fill any remainder with nulls */
  if (i<9)
    for ( ; i<10; i++)
      strcpy(shortcut_values[i], "");

  if (shortcuts_menu)
  {
    /* A shortcuts menu already exists, dispose of it */
    Event_Release(event_MENU,event_ANY,event_ANY,shortcuts_menuselect,NULL);
    Menu_FullDispose(shortcuts_menu);
  }
  /* Look up menu title */
  MsgTrans_Lookup(messages,"Shortcuts",shortcutsmenu_title,sizeof(shortcutsmenu_title));

  /* Create menu */
  shortcuts_menu = Menu_New(shortcutsmenu_title,menudef);

  if (!shortcuts_menu)
  {
    Log(log_WARNING, " Error creating shortcuts menu");
    return FALSE;
  }

  Menu_Show(shortcuts_menu, mousex, -1);

  Event_Claim(event_MENU,event_ANY,event_ANY,shortcuts_menuselect,NULL);

  return TRUE;
}

BOOL shortcuts_menuselect(event_pollblock *event, void *ref)
{
  mouse_block ptrinfo;
  int size;

  Log(log_DEBUG, "shortcuts_menuselect, file=%s", shortcut_values[event->data.selection[0]]);

  if (menu_currentopen != shortcuts_menu)
    return FALSE;

  size = File_Size(shortcut_values[event->data.selection[0]]);
  if (size != -1)
    browser_load(shortcut_values[event->data.selection[0]],size,0);
  else
    Log(log_WARNING, "Problem loading file - doesn't exist?");

  Wimp_GetPointerInfo(&ptrinfo);

  if (ptrinfo.button.data.adjust)
    Menu_ShowLast();

  /* Note: menu and event handler will be released next time menu is created */

  return TRUE;
}
