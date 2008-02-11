/* deskfont.c */

#include <stdlib.h>

#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:SWI.h"

#include "common.h"
#include "deskfont.h"

#define DeskFont_Name "Homerton.Medium"
#define DeskFont_Size (12*16)

#define Wimp_ReadSysInfo	0x400f2

font_handle deskfont_handle = 0;
static font_handle tolose = 0;

static void deskfont_losefont()
{
  if (tolose)
    Font_LoseFont(tolose);
}

static void deskfont_newfont(font_handle font)
{
  font_info finfo;

  if (tolose)
    Font_LoseFont(tolose);
  /* Second condition checks for System font bug */
  if (font && !Font_ReadInfo(font, &finfo))
  {
    deskfont_handle = font;
    tolose = 0;
  }
  else
  {
    Error_CheckFatal(Font_FindFont(&deskfont_handle,
    	DeskFont_Name, DeskFont_Size, DeskFont_Size,
    	0, 0));
    tolose = deskfont_handle;
  }
}

static BOOL deskfont_handler(event_pollblock *event, void *handle)
{
  if (event->data.message.header.size >= 24)
    deskfont_newfont(event->data.message.data.bytes[0]);
  return TRUE;
}

void deskfont_init()
{
  if (event_wimpversion >= 350)
  {
    font_handle df;
    Error_CheckFatal(SWI(1, 1, Wimp_ReadSysInfo, 8, &df));
    deskfont_newfont(df);
    EventMsg_Claim((message_action) Message_FontChanged, event_ANY,
    	deskfont_handler, 0);
  }
  else
    deskfont_newfont(0);
  atexit(deskfont_losefont);
}
