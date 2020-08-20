/* datatrans.c */
/* Deals with RISC OS data transfer */

#include "common.h"

#include "DeskLib:DragASpr.h"
#include "DeskLib:KeyCodes.h"

#include "browser.h"
#include "datatrans.h"
#include "diagutils.h"


window_handle saveas_window, saveas_export;
static datatrans_saver saveas_saver;
static datatrans_complete saveas_complete;
static browser_fileinfo *datatrans_browser;
static int datatrans_myref;
static BOOL datatrans_safe;
static BOOL saveas_dragging;


/* We need to be able to check against this function to avoid leaving
   save dbox open if browser is closing */
extern void unsaved_complete(void *browser,BOOL success,BOOL safe,
       	    		     char *filename);

/* Handlers for saveas window */
BOOL saveas_accclose(event_pollblock *event,void *ref); /* Shouldn't happen */
/* Set filename icon according to Selection button*/
BOOL saveas_selection(event_pollblock *event,void *ref);
BOOL saveas_clicksave(event_pollblock *event,void *ref);
BOOL saveas_key(event_pollblock *event,void *ref);
BOOL saveas_startdrag(event_pollblock *event,void *ref);

/* Start save protocol when drag completed */
BOOL saveas_dragcomplete(event_pollblock *event,void *ref);

/* Save when message_DATASAVE acknowleged */
BOOL datatrans_dosave(event_pollblock *event,void *ref);

/* Release above handler and itself if message_DATASAVE acknowleged or not */
BOOL datatrans_releasesaveack(event_pollblock *event,void *ref);

/* Call above and tell originator save has failed */
BOOL datatrans_nosaveack(event_pollblock *event,void *ref);

/* Respond to message_DATALOAD */
BOOL datatrans_loadack(event_pollblock *event,void *ref);

/* Release above handler and itself if message_DATALOAD acknowleged or not */
BOOL datatrans_releaseloadack(event_pollblock *event,void *ref);

/* Call above and tell originator save has failed */
BOOL datatrans_noloadack(event_pollblock *event,void *ref);


BOOL saveas_open = FALSE;
BOOL saveas_export_open = FALSE;

/* Calls a loader function then replies to the Message_DataLoad and deletes
   Scrap file if appropriate */
void        datatrans_load(event_pollblock *event,datatrans_loader loader,void *ref)
{
  int size;
  message_block message = event->data.message;
  Log(log_DEBUG, "datatrans_load");

  size = File_Size(message.data.dataload.filename);
  if (size && (*loader)(message.data.dataload.filename,size,ref))
  {
    message.header.yourref = message.header.myref;
    message.header.action = message_DATALOADACK;
    Error_Check(Wimp_SendMessage(event_SEND,
    				 &message,message.header.sender,0));
  }
  /*
  else
    WinEd_MsgTrans_ReportPS(messages,"NotFile",FALSE,
    		      message.data.dataload.filename,0,0,0);
  */

  if (!strcmp(message.data.dataload.filename,"<Wimp$Scrap>"))
    File_Delete(message.data.dataload.filename);
}

void        datatrans_saveack(event_pollblock *event)
{
  message_block message = event->data.message;
  Log(log_DEBUG, "datatrans_saveack");

  message.header.size = 64;
  message.header.yourref = message.header.myref;
  message.header.action = message_DATASAVEACK;
  message.data.datasaveack.estsize = -1;
  strcpy(message.data.datasaveack.filename,"<Wimp$Scrap>");
  Error_Check(Wimp_SendMessage(event_SEND,&message,message.header.sender,0));
}

void        saveas_init()
{
  window_block *templat;
  Log(log_DEBUG, "saveas_init");

  /* Main save box */
  templat = templates_load("SaveAs",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&saveas_window));
  free(templat);

  /* Export icon name/numbers save box */
  templat = templates_load("Export",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&saveas_export));
  free(templat);

  saveas_dragging = FALSE;
}

BOOL        saveas_selection(event_pollblock *event,void *ref)
{
  char *filename = ref;
  char buffer[256];
  int buflen;
  char untitled[24];
  Log(log_DEBUG, "saveas_selection");

  if (Icon_GetSelect(saveas_window,saveas_SELECTION))
  {
    MsgTrans_Lookup(messages,"Selection",buffer,256);
  }
  else
  {
    /* Strip " *" from filename and change "<Untitled>" to "Templates" */
    strcpy(buffer,filename);
    buflen = strlen(buffer);
    if (buffer[buflen - 1] == '*')
      buffer[buflen - 2] = 0;
    MsgTrans_Lookup(messages,"Untitled",untitled,24);
    if (!strcmp(buffer,untitled))
      MsgTrans_Lookup(messages,"Templates",buffer,256);
  }
  Icon_SetText(saveas_window,saveas_FILE,buffer);
  return TRUE;
}

static BOOL saveas_esccancel(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "saveas_esccancel");
  if (event->data.key.code == keycode_ESCAPE)
  {
    Wimp_DragBox(NULL);
    Event_Release(event_USERDRAG,event_ANY,event_ANY,
                  saveas_dragcomplete,reference);
    Event_Release(event_KEY,event_ANY,event_ANY,saveas_esccancel,reference);
    saveas_dragging = FALSE;
    return TRUE;
  }
  return FALSE;
}

static void saveas_release_specific(void *ref)
{
  Log(log_DEBUG, "saveas_release_specific");

  if (saveas_dragging)
  {
    event_pollblock event;
    event.data.key.code = keycode_ESCAPE;
    saveas_esccancel(&event, ref);
  }
}

static BOOL saveas_release_msg(event_pollblock *e, void *ref)
{
  Log(log_DEBUG, "saveas_release_msg");

  saveas_release_specific(ref);
  return FALSE;
}

void        datatrans_saveas(char *filename,BOOL allow_selection,
                             int x,int y,
                             BOOL leaf,
		             datatrans_saver saver,
		             datatrans_complete complete,
		             void *ref,/* <- unsaved_browser */
		             BOOL seln,
		             BOOL export)
{
  window_handle win = export ? saveas_export : saveas_window;
  Log(log_DEBUG, "datatrans_saveas");

  /* Ensure event handlers for saveas not already registered */
  saveas_release();
  saveas_saver = saver;
  saveas_complete = complete;
  datatrans_browser = ref;
  Event_Claim(event_OPEN,win,event_ANY,Handler_OpenWindow,ref);
  Event_Claim(event_CLOSE,win,event_ANY,saveas_accclose,ref);
  Event_Claim(event_CLICK,win,saveas_SAVE,saveas_clicksave,ref);
  Event_Claim(event_CLICK,win,saveas_CANCEL,kill_menus,ref);
  Event_Claim(event_CLICK,win,saveas_ICON,saveas_startdrag,ref);
  Event_Claim(event_KEY,win,saveas_FILE,saveas_key,ref);
  if (export) help_claim_window(win, "EXP"); else help_claim_window(win,"SAVE");
  if (!export)
  {
    Event_Claim(event_CLICK,win,saveas_SELECTION,
    	saveas_selection,filename);
    Icon_SetSelect(saveas_window,saveas_SELECTION,seln);
    Icon_SetShade(saveas_window,saveas_SELECTION,!allow_selection);
    saveas_selection(0,filename);
  }
  else
  {
    Icon_SetText(win,saveas_FILE,filename);
    Event_Claim(event_CLICK,win,saveas_CENUM,export_setfiletype,ref);
    Event_Claim(event_CLICK,win,saveas_CDEFINE,export_setfiletype,ref);
    Event_Claim(event_CLICK,win,saveas_CTYPEDEF,export_setfiletype,ref);
    Event_Claim(event_CLICK,win,saveas_MESSAGES,export_setfiletype,ref);
    Event_Claim(event_CLICK,win,saveas_BASIC,export_setfiletype,ref);
    Event_Claim(event_CLICK,win,saveas_PREFIX,export_setfiletype,ref);

    Event_Claim(event_CLICK,win, saveas_CENUM, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_CDEFINE, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_CTYPEDEF, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_MESSAGES, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_BASIC, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_UNCHANGED, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_LOWER, diagutils_adjust_radio, ref);
    Event_Claim(event_CLICK,win, saveas_UPPER, diagutils_adjust_radio, ref);

    export_setfiletype(0, 0);
  }

  if (leaf)
    Error_Check(Wimp_CreateSubMenu((menu_ptr) win,x,y));
  else
    WinEd_CreateMenu((menu_ptr) win,x+64,y);

  /* The save box is created as a menu (see lines above) so closes when the user
     presses escape and a menusdeleted message is generated. This claim deals with that
     occurance. However, care is needed as the preceding dialogue box (if we've
     arrived here as a result of a click on the "save" icon of the "are you sure you want
     to discard window") is also a menu so will close immediately before the save box opens
     and generate a menusdeleted message... */
  EventMsg_Claim(message_MENUSDELETED, event_ANY, saveas_release_msg, ref);

  if (export)
    saveas_export_open = TRUE;
  else
    saveas_open = TRUE;
  Icon_SetCaret(win,saveas_FILE);
}

BOOL        saveas_accclose(event_pollblock *event,void *ref)
{
  Log(log_DEBUG, "savea_acclse");
  Wimp_CloseWindow(saveas_window);
  saveas_release_specific(ref);
  saveas_release();
  return TRUE;
}

static void saveas_immediate_save(window_handle window, BOOL close, void *ref)
{
  char buffer[256];
  Log(log_DEBUG, "saveas_immedite_sve");

  Icon_GetText(window,saveas_FILE,buffer);
  if (!strchr(buffer,'.') && !strchr(buffer,':'))
  {
    WinEd_MsgTrans_Report(messages,"ToSave",FALSE);
    return;
  }

  if ((*saveas_saver)(buffer,ref,
  	saveas_export_open ?
  		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION)))
  {
    if (close || saveas_complete == (datatrans_complete) unsaved_complete)
      WinEd_CreateMenu((menu_ptr) -1,0,0);
    (*saveas_complete)(ref,TRUE,TRUE,buffer,
    	saveas_export_open ?
  		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));
  }
  else
    (*saveas_complete)(ref,FALSE,FALSE,NULL,
  	saveas_export_open ?
  		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));
}

BOOL        saveas_clicksave(event_pollblock *event,void *ref)
{
  Log(log_DEBUG, "saveas_clicksave");
  if (!event->data.mouse.button.data.select &&
      !event->data.mouse.button.data.adjust)
    return FALSE;

  saveas_immediate_save(event->data.mouse.window,
  	event->data.mouse.button.data.select, ref);

  return TRUE;
}

/**
 * Process clicks in the export icons dialogue, which can
 * change the behaviour of the rest of the icons.
 */
BOOL export_setfiletype(event_pollblock *event,void *ref)
{
  int icon;

  Log(log_DEBUG, "export_setfiletype");

  icon = Icon_WhichRadioInEsg(saveas_export, 1);

  switch(icon)
  {
    case saveas_BASIC:
      Icon_SetText(saveas_export, saveas_ICON, "file_ffb");
      Log(log_DEBUG, "Basic icon revealed");
      break;
    default:
      Icon_SetText(saveas_export, saveas_ICON, "file_fff");
      Log(log_DEBUG, "Text icon revealed");
      break;
  }

  Icon_SetShade(saveas_export, saveas_SKIPIMPLIED,
      icon != saveas_CENUM && icon != saveas_CTYPEDEF);
  Icon_SetShade(saveas_export, saveas_USEREAL, icon != saveas_BASIC);

  Icon_SetShade(saveas_export, saveas_PREFIXFIELD, !Icon_GetSelect(saveas_export, saveas_PREFIX));

  return TRUE;
}

BOOL        saveas_key(event_pollblock *event,void *ref)
{
  Log(log_DEBUG, "saveas_key");
  switch (event->data.key.code)
  {
    case keycode_RETURN:
      saveas_immediate_save(event->data.key.caret.window, TRUE, ref);
      return TRUE;
    case keycode_ESCAPE:
      if (saveas_dragging)
        saveas_esccancel(event,ref);
      return TRUE;
  }
  return FALSE;
}

BOOL        saveas_startdrag(event_pollblock *event,void *ref)
{
  Log(log_DEBUG, "saveas_startdrag");
  if (!event->data.mouse.button.data.dragselect &&
      !event->data.mouse.button.data.dragadjust)
  {
    Log(log_DEBUG, " exiting - not a drag");
    return FALSE;
  }
  Error_Check(DragASprite_DragIcon(event->data.mouse.window,saveas_ICON));
  Event_Claim(event_USERDRAG,event_ANY,event_ANY,saveas_dragcomplete,ref);
  Event_Claim(event_KEY,event_ANY,event_ANY,saveas_esccancel,ref);
  saveas_dragging = TRUE;
  return TRUE;
}

void        saveas_release()
{
  Log(log_DEBUG, "saveas_release");
  if (saveas_open || saveas_export_open)
    EventMsg_Release(message_MENUSDELETED, event_ANY, saveas_release_msg);
  if (saveas_open)
  {
    EventMsg_ReleaseWindow(saveas_window);
    Event_ReleaseWindow(saveas_window);
    saveas_open = FALSE;
  }
  if (saveas_export_open)
  {
    EventMsg_ReleaseWindow(saveas_export);
    Event_ReleaseWindow(saveas_export);
    saveas_export_open = FALSE;
  }
}

BOOL        saveas_dragcomplete(event_pollblock *event,void *ref)
{
  mouse_block ptrinfo;
  message_block message;
  char filename[256];
  char *leaf;
  Log(log_DEBUG, "saveas_dragcomplete");

  Event_Release(event_USERDRAG,event_ANY,event_ANY,saveas_dragcomplete,ref);
  Event_Release(event_KEY,event_ANY,event_ANY,saveas_esccancel,ref);
  saveas_dragging = FALSE;
  DragASprite_Stop();
  Wimp_GetPointerInfo(&ptrinfo);
  if (ptrinfo.window == ((browser_fileinfo *) ref)->window)
  {
    return TRUE;
  }

  message.header.size = sizeof(message_header) + sizeof(message_datasave);
  message.header.yourref = 0;
  message.header.action = message_DATASAVE;
  message.data.datasave.window = ptrinfo.window;
  message.data.datasave.icon = ptrinfo.icon;
  message.data.datasave.pos = ptrinfo.pos;
  message.data.datasave.estsize = browser_estsize((browser_fileinfo *) ref);
  message.data.datasave.filetype = filetype_TEMPLATE;
  Icon_GetText(saveas_export_open ? saveas_export : saveas_window,
  	saveas_FILE,filename);
  Str_MakeASCIIZ(filename,256);

  leaf = strrchr(filename,'.');
  if (!leaf)
  {
    leaf = strrchr(filename,':');
    if (!leaf)
      leaf = filename;
    else
      leaf++;
  }
  else
    leaf++;

  strncpy(message.data.datasave.leafname,leaf,212);
  Error_Check(Wimp_SendMessage(event_SENDWANTACK,&message,
  	      		       ptrinfo.window,ptrinfo.icon));
  datatrans_myref = message.header.myref;
  EventMsg_Claim(message_DATASAVEACK,event_ANY,datatrans_dosave,
                 (void *) datatrans_myref);
  Event_Claim(event_ACK,event_ANY,event_ANY,datatrans_nosaveack,
              (void *) datatrans_myref);
  return TRUE;
}

BOOL        datatrans_dosave(event_pollblock *event,void *ref)
{
  message_block message;
  Log(log_DEBUG, "datatrans_dosave");

  /* Release handlers */
  if (!datatrans_releasesaveack(event,ref))
    return FALSE;

  if (!(*saveas_saver)(event->data.message.data.datasaveack.filename,
                     datatrans_browser,
                     saveas_export_open ?
    			FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION)))
  {
    (*saveas_complete)(datatrans_browser,FALSE,FALSE,NULL,
    	saveas_export_open ?
    		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));
    return TRUE;
  }

  if (event->data.message.data.datasaveack.estsize == -1)
    datatrans_safe = FALSE;
  else
    datatrans_safe = TRUE;

  message = event->data.message;
  message.header.yourref = message.header.myref;
  message.header.action = message_DATALOAD;
  message.data.dataload.size =
    File_Size(event->data.message.data.datasaveack.filename);
  Error_Check(Wimp_SendMessage(event_SENDWANTACK,&message,
                               message.data.dataload.window,
                               message.data.dataload.icon));
  datatrans_myref = message.header.myref;
  EventMsg_Claim(message_DATALOADACK,event_ANY,datatrans_loadack,
                 (void *) datatrans_myref);
  Event_Claim(event_ACK,event_ANY,event_ANY,datatrans_noloadack,
              (void *) datatrans_myref);
  return TRUE;
}

BOOL        datatrans_nosaveack(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "datatrans_nosaveack");
  if (event->data.message.header.action != message_DATASAVE)
    return FALSE;
  if (datatrans_releasesaveack(event,reference))
  {
    (*saveas_complete)(datatrans_browser,FALSE,FALSE,NULL,
    	saveas_export_open ?
    		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));
    return TRUE;
  }
  return FALSE;
}

BOOL        datatrans_releasesaveack(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "datatrans_releasesaveack");
  if ((int) reference != datatrans_myref)
    return FALSE;

  EventMsg_Release(message_DATASAVEACK,event_ANY,datatrans_dosave);
  Event_Release(event_ACK,event_ANY,event_ANY,datatrans_nosaveack,reference);
  return TRUE;
}

BOOL        datatrans_loadack(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "datatrans_loadack");
  if (!datatrans_releaseloadack(event,reference))
    return FALSE;

  /* Remove Save as window from screen (inc menu) */
  WinEd_CreateMenu((menu_ptr) -1,0,0);
  (*saveas_complete)(datatrans_browser,TRUE,datatrans_safe,
                     event->data.message.data.dataload.filename,
    		     saveas_export_open ?
    			FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));

  return TRUE;
}

BOOL        datatrans_noloadack(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "datatrans_noloadack");
  if (event->data.message.header.action != message_DATALOAD)
    return FALSE;
  if (datatrans_releaseloadack(event,reference))
  {
    WinEd_MsgTrans_Report(messages,"LoadAck",FALSE);
    (*saveas_complete)(datatrans_browser,FALSE,FALSE,NULL,
    	saveas_export_open ?
    		FALSE : Icon_GetSelect(saveas_window,saveas_SELECTION));
    return TRUE;
  }
  return FALSE;
}

BOOL        datatrans_releaseloadack(event_pollblock *event,void *reference)
{
  Log(log_DEBUG, "datatrans_releaseloadack");
  if ((int) reference != datatrans_myref)
    return FALSE;

  EventMsg_Release(message_DATALOADACK,event_ANY,datatrans_loadack);
  Event_Release(event_ACK,event_ANY,event_ANY,datatrans_noloadack,reference);
  return TRUE;
}

void        datatrans_dragndrop(datatrans_saver saver, datatrans_complete complete,
                	        browser_fileinfo *browser)
{
  char buffer[256];
  wimp_rect /*srect,*/ prect;
  mouse_block ptr;

  Log(log_DEBUG, "datatrans_dragndrop");
  saveas_saver = saver;
  saveas_complete = complete;
  datatrans_browser = browser;
  saveas_open = saveas_export_open = FALSE;
  Icon_Select(saveas_window, saveas_SELECTION);
  MsgTrans_Lookup(messages, "Selection", buffer, 256);
  Icon_SetText(saveas_window, saveas_FILE, buffer);
  Wimp_GetPointerInfo(&ptr);
  prect.min.x = ptr.pos.x - 34;
  prect.min.y = ptr.pos.y - 34;
  prect.max.x = ptr.pos.x + 34;
  prect.max.y = ptr.pos.y + 34;
  /*
  Screen_CacheModeInfo();
  srect.min.x = srect.min.y = 0;
  srect.max = screen_size;
  */
  Error_Check(DragASprite_Start(
  	1 + (1<<2) + (1<<6) + (1<<7),
  		/* Centred, pointer screen bounded, shadow */
  	(void *) 1, "file_fec", &prect, NULL));
  Event_Claim(event_USERDRAG,event_ANY,event_ANY,saveas_dragcomplete,browser);
  Event_Claim(event_KEY,event_ANY,event_ANY,saveas_esccancel,browser);
  saveas_dragging = TRUE;
}

