/* WinEd IDE module */

#include "DeskLib:Wimp.h"
#include "DeskLib:WimpMsg.h"

#include "common.h"
#include "globals.h"
#include "ide.h"

void ide_broadcast(browser_fileinfo *browser)
{
  ide_handshake data;
  int myref, namelen;
  char onlyname[256];

  Log(log_DEBUG, "ide_broadcast");

  data.handle = browser->window; /* Use browser window handle as unique identifier */

  /* Strip off " *", if needed */
  snprintf(onlyname, sizeof(onlyname), "%s", browser->title);
  if (browser->altered)
    onlyname[strlencr(browser->title) - 2] = 0;

  /* Check filename isn't too long */
  namelen = strlencr(onlyname) + 1;

  if (namelen > sizeof(data.filename))
    /* If name is too long, remove characters from beginning */
    snprintf(data.filename, sizeof(data.filename), "%s", onlyname + namelen - sizeof(data.filename));
  else
    snprintf(data.filename, sizeof(data.filename), "%s", onlyname);

  myref = WimpMsg_Broadcast(event_SENDWANTACK, message_WinEd_IDE_BROADCAST, (void *)&data, sizeof(data));
  Log(log_INFORMATION, "Broadcast message sent. Reference:%d; Identifier:%d; Filename:%s",
                        myref, data.handle, data.filename);

  /* Claim acknowledgement. Note that we can only deal with one of these at a time as ide_receive
     releases all handlers attached to this message type when it gets a valid response. */
  EventMsg_Claim(message_WinEd_IDE_ACKNOWLEDGE, browser->window, ide_receive, (void *)myref);
}

BOOL ide_receive(event_pollblock *event, void *reference)
{
  int myref = (int) reference;
  ide_handshake *data = (ide_handshake *) &(event->data.message.data);

  Log(log_DEBUG, "ide_receive");

  Log(log_INFORMATION, "Message received from task %d. Reference:%d (sent ref was:%d); Identifier:%d; Filename:%s",
      event->data.message.header.sender, event->data.message.header.yourref, myref, data->handle, data->filename);

  if (myref == event->data.message.header.yourref)
  {
    /* If this was our message, release handler & register client for this browser */
    browser_fileinfo *browser;

    Log(log_INFORMATION, "Message was in response to our broadcast, searching for browser which sent it...");

    EventMsg_ReleaseWindow(message_WinEd_IDE_ACKNOWLEDGE);

    /* Find out which browser based on value of identifier (which was the browser window handle), and register client */
    for (browser = LinkList_NextItem(&browser_list); browser; browser = LinkList_NextItem(browser))
    {
      if (browser->window == data->handle)
      {
        Log(log_INFORMATION, "...found the broadcasting browser (%s)", browser->title);
        browser->idetask = event->data.message.header.sender;
        return TRUE;
      }
    }
  }

  Log(log_INFORMATION, "...failed to find browser which broadcast the message.");

  /* This event was for someone else (different WinEd or different window) */
  return FALSE;
}

void ide_sendinfo(browser_winentry *winentry, event_pollblock *event)
{
  ide_event fake_event;

  Log(log_DEBUG, "ide_sendinfo");

  if (winentry->browser->idetask)
  {
    Log(log_DEBUG, "Client task handle exists, sending event data...");

    if (event->type == event_CLICK)
    {
      char iconname[204];

      fake_event.handle = winentry->browser->window;
      fake_event.event = event_CLICK;
      fake_event.data.click.button = event->data.mouse.button;
      fake_event.data.click.icon = event->data.mouse.icon;
      snprintf(fake_event.data.click.identifier, sizeof(fake_event.data.click.identifier), "%s", winentry->identifier);

      extract_iconname(winentry, event->data.mouse.icon, iconname, sizeof(iconname));
      snprintf(fake_event.data.click.iconname, sizeof(fake_event.data.click.iconname), "%s", iconname);
    }
    else
    {
      Log(log_NOTICE, "Unknown event (%d) passed to ide_sendinfo", event->type);
    }
  }

  WimpMsg_SendTask(event_SEND, message_WinEd_IDE_EVENT, (void *)&fake_event, sizeof(fake_event), winentry->browser->idetask);
}
