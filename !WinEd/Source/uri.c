/* URI handling */

#include <stdlib.h>
#include <string.h>

#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:MsgTrans.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Debug.h"

#include "common.h"
#include "globals.h"
#include "uri.h"

static BOOL uri_noack(event_pollblock *event, void *handle)
{
  char workstr[256];

  if (event->data.message.header.action != message_URI)
    return FALSE;

  strcpy(workstr, "Alias$URLOpen_");
  strncat(workstr, event->data.message.data.bytes,
  	strchr(event->data.message.data.bytes, ':') -
  		event->data.message.data.bytes);
  if (!getenv(workstr))
  {
    WinEd_MsgTrans_Report(messages, "NoURI", FALSE);
    return TRUE;
  }
  strcat(workstr, " ");
  strcat(workstr, event->data.message.data.bytes);
  Error_Check(Wimp_StartTask(workstr + 6));
  return TRUE;
}

void uri_init(void)
{
  Log(log_DEBUG, "uri_init");
  Event_Claim(event_ACK, event_ANY, event_ANY, uri_noack, 0);
}

extern void uri_send(const char *uri)
{
  message_block m;

  m.header.size = sizeof(message_header) + ((strlen(uri) + 4) & ~3);
  	/* 4 added instead of 3 to allow for terminator */
  m.header.yourref = 0;
  m.header.action = message_URI;
  strcpy(m.data.bytes, uri);
  Error_Check(Wimp_SendMessage(event_USERMESSAGERECORDED, &m, 0, 0));
}
