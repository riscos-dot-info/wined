/* unsaved.c */
/* Warns user of attempts to quit/close with unsaved data */

#include "common.h"
#include "browser.h"

#include "DeskLib:KeyCodes.h"

#include "unsaved.h"

/* Icon numbers */
typedef enum {
  close_SAVE,
  close_MSG,
  close_DISCARD,
  close_CANCEL
} close_icon;

typedef enum {
  quit_DISCARD,
  quit_MSG,
  quit_CANCEL
} quit_icon;

/* typedef struct {
  union {
    event_handler closehandler;
    unsaved_savehandler savehandler;
  } handler;
  void *browser;
} unsaved_closeref; */

static unsaved_closehandler close_handler;
static datatrans_saver save_handler;
static browser_fileinfo *unsaved_browser;

/* Handling of shutdown */
static BOOL unsaved_shutdown; /* Whether to re-initiate shutdown */
static task_handle shutdown_sender;
static BOOL unsaved_discardanyway; /* Allows unsaved_quit() to be re-entered
       	    			      if this task has re-initiated shutdown
       	    			    */

static window_handle close_window;
static window_handle quit_window;


BOOL close_savehandler(event_pollblock *event,void *ref);
BOOL close_discardhandler(event_pollblock *event,void *ref);
BOOL close_cancelhandler(event_pollblock *event,void *ref);
BOOL quit_cancelhandler(event_pollblock *event,void *ref);
BOOL quit_discardhandler(event_pollblock *event,void *ref);
/* Closes browser if save was successful */
void unsaved_complete(void *browser,BOOL success,BOOL safe,
     		      char *filename,BOOL selection);

/* Stops message_MENUSDELETED cocking up saveas box */
BOOL close_menudel(event_pollblock *event,void *ref);



void unsaved_init()
{
  window_block *templat;
  char buffer[128];

  templat = templates_load("Close",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&close_window));
  free(templat);
  templat = templates_load("Quit",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&quit_window));
  free(templat);
  MsgTrans_Lookup(messages,"CloseWarn",buffer,128);
  Icon_SetText(close_window,close_MSG,buffer);
  MsgTrans_Lookup(messages,"QuitWarn",buffer,128);
  Icon_SetText(quit_window,quit_MSG,buffer);

  Event_Claim(event_OPEN,close_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,close_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,close_window,close_SAVE,close_savehandler,0);
  Event_Claim(event_CLICK,close_window,close_DISCARD,close_discardhandler,0);
  Event_Claim(event_CLICK,close_window,close_CANCEL,close_cancelhandler,0);
  help_claim_window(close_window,"CLO");

  Event_Claim(event_OPEN,quit_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,quit_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,quit_window,quit_DISCARD,quit_discardhandler,0);
  Event_Claim(event_CLICK,quit_window,quit_CANCEL,quit_cancelhandler,0);
  help_claim_window(quit_window,"PRQ");

  unsaved_discardanyway = FALSE;
}

/* Calls handler if Discard chosen,
   whether to shutdown worked out from event */
void unsaved_quit(event_pollblock *event)
{
  if (unsaved_discardanyway)
  {
    browser_userquit = TRUE;
    return;
  }

  if (event)
  {
    message_block *message = &event->data.message;

    shutdown_sender = message->header.sender;

    /* Reply to prequit message */
    message->header.yourref = message->header.myref;
    Error_Check(Wimp_SendMessage(event_ACK,message,shutdown_sender,0));

    /* Detect shutdown */
    if (message->data.words[0] & 1)
      unsaved_shutdown = FALSE;
    else
      unsaved_shutdown = TRUE;
  }
  else
    unsaved_shutdown = FALSE;

  transient_centre(quit_window);
}

/* Calls closehandler if Discard chosen, savehandler if Save chosen.
   ref should point to relevant browser */
void unsaved_close(unsaved_closehandler closehandler,datatrans_saver saver,
                   browser_fileinfo *browser)
{
  transient_centre(close_window);
  close_handler = closehandler;
  save_handler = saver;
  unsaved_browser = browser;
}

BOOL close_cancelhandler(event_pollblock *event,void *ref)
{
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  /*Error_Check(Wimp_CloseWindow(close_window));*/
  return TRUE;
}

BOOL quit_cancelhandler(event_pollblock *event,void *ref)
{
  WinEd_CreateMenu((menu_ptr) -1, 0, 0);
  /*Error_Check(Wimp_CloseWindow(quit_window));*/
  return TRUE;
}

BOOL quit_discardhandler(event_pollblock *event,void *ref)
{
  if (unsaved_shutdown)
  {
    key_block shutkey;

    Error_Check(Wimp_GetCaretPosition(&shutkey.caret));
    shutkey.code = keycode_CTRL_SHIFT_F12;
    Error_Check(Wimp_SendMessage(event_KEY,(message_block *) &shutkey,
    				 shutdown_sender,0));
    /* Apparently can't exit() yet or shutdown won't work, therefore indicate
       that data can be discarded to avoid infinite loop of unsaved_quit() */
    unsaved_discardanyway = TRUE;
  }
  else
  {
    browser_userquit = TRUE;
    exit(0);
  }

  return TRUE;
}

BOOL close_discardhandler(event_pollblock *event,void *ref)
{
  close_cancelhandler(NULL,0);
  (*close_handler)(unsaved_browser);
  return TRUE;
}

BOOL close_savehandler(event_pollblock *event,void *ref)
{
  mouse_block ptrinfo;

  Wimp_GetPointerInfo(&ptrinfo);
  close_cancelhandler(NULL,0);
  /* Claim message_MENUSDELETED to stop it cocking up saveas box */
  EventMsg_Claim(message_MENUSDELETED,event_ANY,close_menudel,0);
  datatrans_saveas(unsaved_browser->title,FALSE,
                   ptrinfo.pos.x-64,ptrinfo.pos.y+64,
                   FALSE,
  		   save_handler,
  		   unsaved_complete,
  		   unsaved_browser,
  		   FALSE,
  		   FALSE);
  return TRUE;
}

void unsaved_complete(void *browser,BOOL success,BOOL safe,
     		      char *filename,BOOL selection)
{
  if (safe)
    (*close_handler)(browser);
}

BOOL close_menudel(event_pollblock *event,void *ref)
{
  EventMsg_Release(message_MENUSDELETED,event_ANY,close_menudel);
  return TRUE;
}
