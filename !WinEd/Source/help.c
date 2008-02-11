/* help.c */

#include <stdio.h>
#include <string.h>

#include "DeskLib:SWI.h"
#include "DeskLib:WimpSWIs.h"

#include "globals.h"
#include "common.h"
#include "help.h"
#include "monitor.h"

#ifndef SWI_Wimp_GetMenuState
#define SWI_Wimp_GetMenuState 0x400f4
#endif

/* This actually sends the HelpReply message given the HelpRequest message
   (needed for sender/ref details) and MessageTrans tag. If fails to find
   message, any trailing G is stripped, then icon numbers */
void help_reply(message_block *originator,char *tag)
{
  message_block send;

  if (MsgTrans_Lookup(messages,tag,send.data.helpreply.text,200))
  {
    BOOL success = FALSE;

    /* Strip trailing G */
    if (tag[strlen(tag) - 1] == 'G')
    {
      tag[strlen(tag) - 1] = 0;
      if (!MsgTrans_Lookup(messages,tag,send.data.helpreply.text,200))
        success = TRUE;
    }

    if (!success)
    {
      char *rchr = strrchr(tag,'_');
      /* Strip icon numbers */
      if (rchr == strchr(tag,'_')) /* There were no numbers */
        return;
      *rchr = 0;
      if (MsgTrans_Lookup(messages,tag,send.data.helpreply.text,200))
        return;
    }
  }

  /* Complete and send message */
  send.header.size = sizeof(message_block);
  send.header.yourref = originator->header.myref;
  send.header.action = message_HELPREPLY;
  Wimp_SendMessage(event_SEND,&send,originator->header.sender,0);

  /* Help messages come more often than we're poll idling and prevent
     null events so give monitor a chance */
  if (monitor_isactive)
    monitor_update(0, monitor_winentry);
}

BOOL help_on_window(event_pollblock *event,void *reference)
{
  char tag[20];

  sprintf(tag,"H_%s",(char *) reference);

  if (event->data.message.data.helprequest.where.icon != -1 &&
      event->data.message.data.helprequest.where.window != window_ICONBAR)
  {
    sprintf(tag + strlen(tag),"_%02d",
    	    event->data.message.data.helprequest.where.icon);
  }

  help_reply(&event->data.message,tag);

  return TRUE;
}

BOOL help_on_menu(event_pollblock *event,void *reference)
{
  char tag[32];
  int selection[10];
  int index;

  sprintf(tag,"H_%s",(char *) reference);
  SWI(2,0,SWI_Wimp_GetMenuState,0,selection);

  if (selection[0] != -1)
  {
    icon_block istate;

    strcat(tag,"_");

    for (index = 0; selection[index] != -1;index++)
      sprintf(tag + strlen(tag),"%02d",selection[index]);

    Wimp_GetIconState(event->data.message.data.helprequest.where.window,
    		      event->data.message.data.helprequest.where.icon,&istate);
    if (istate.flags.data.shaded)
      strcat(tag,"G");

    help_reply(&event->data.message,tag);
  }

  return TRUE;
}
