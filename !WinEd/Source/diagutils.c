/* diagutils.c */

#include "common.h"

#include "DeskLib:KeyCodes.h"
#include "DeskLib:Kbd.h"

#include "diagutils.h"

/* Function to encode a value into a field */
typedef void (*diagutils_encoder)(window_handle window,icon_handle field,
	      			  int value);

/* Function to decode a value from a field */
typedef int (*diagutils_decoder)(window_handle window,icon_handle field);

/* Info needed for handlers */
typedef struct {
  diagutils_encoder encoder;
  diagutils_decoder decoder;
  window_handle window;
  icon_handle field;
  int min,max;
  int step;
  BOOL none; /* Whether to allow 'none' for colours */
} diagutils_block;

/* Register handlers for bumper icons next to any field */
void _diagutils_bumpers(diagutils_block *block);

/* General purpose down bumper handler */
BOOL diagutils_dechandler(event_pollblock *event,diagutils_block *block);

/* General purpose up bumper handler */
BOOL diagutils_inchandler(event_pollblock *event,diagutils_block *block);

/* Handler for button-type menu button */
BOOL diagutils_popupbtype(event_pollblock *event,diagutils_block *block);

/* Handler for colour menu button */
BOOL diagutils_popupcolour(event_pollblock *event,diagutils_block *block);

/* Colour dbox handlers */
BOOL colour_clicknone(event_pollblock *event,void *reference);
BOOL colour_clickcolour(event_pollblock *event,void *reference);

/* Button-type menu handlers */
BOOL diagutils_menuselect(event_pollblock *event,diagutils_block *block);
BOOL diagutils_menurelease(event_pollblock *event,diagutils_block *block);

/* Tick an entry in diagutils_menu */
void diagutils_tickmenu(int entry);

/* Colour dbox icons */
typedef enum {
  colour_NONE,
  colour_COLOUR = 2
} colour_icons;

/* Button type menu */
static menu_ptr diagutils_menu;

/* Colour dbox */
static window_handle colour_window;

/* Which field is being edited */
static diagutils_block diagutils_whichfield;


void diagutils_init()
{
  window_block *templat;
  char buffer[256];
  char tag[32];
  int index;

  /* Create dialogue box */
  templat = templates_load("Colour",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&colour_window));
  free(templat);
  Event_Claim(event_OPEN,colour_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLICK,colour_window,colour_NONE,colour_clicknone,0);
  for (index = colour_COLOUR;index < colour_COLOUR + 16;index++)
    Event_Claim(event_CLICK,colour_window,index,colour_clickcolour,0);
  help_claim_window(colour_window,"COL");

  /* Create menu */
  *buffer = 0;
  for (index = 0;index < 16;index++)
  {
    if (index > 0)
      strcat(buffer,",");
    sprintf(tag,"BType%d",index);
    Error_CheckFatal(MsgTrans_Lookup(messages,tag,buffer + strlen(buffer),
    		     		     256 - strlen(buffer)));
  }
  Error_CheckFatal(MsgTrans_Lookup(messages,"BType",tag,32));
  diagutils_menu = Menu_New(tag,buffer);
  if (!diagutils_menu)
    WinEd_MsgTrans_Report(messages,"NoMenu",TRUE);
}

void diagutils_bumpers(window_handle window,icon_handle field,
     		       int min,int max /* Limits */,int step)
{
  /* Note: step=-1 is used to mean: step=4 normally, 2 with shift, 1 with ctrl. See dechandler, inchandler */
  diagutils_block *block;

  block = malloc(sizeof(diagutils_block));
  if (!block)
    WinEd_MsgTrans_Report(0,"NoStore",TRUE);
  block->encoder = Icon_SetInteger;
  block->decoder = Icon_GetInteger;
  block->window = window;
  block->field = field;
  block->min = min;
  block->max = max;
  block->step = step;
  block->none = FALSE;
  _diagutils_bumpers(block);
}

void diagutils_colour(window_handle window,icon_handle field,BOOL none)
{
  diagutils_block *block;

  block = malloc(sizeof(diagutils_block));
  if (!block)
    WinEd_MsgTrans_Report(0,"NoStore",TRUE);
  block->decoder = diagutils_readcolour;
  block->encoder = diagutils_writecolour;
  block->window = window;
  block->field = field;
  if (none)
    block->min = -1;
  else
    block->min = 0;
  block->max = 15;
  block->step = 1;
  block->none = none;
  _diagutils_bumpers(block);
  Event_Claim(event_CLICK,window,field + 3,
  	      (event_handler) diagutils_popupcolour,block);
}

void diagutils_btype(window_handle window,icon_handle field)
{
  diagutils_block *block;

  block = malloc(sizeof(diagutils_block));
  if (!block)
    WinEd_MsgTrans_Report(0,"NoStore",TRUE);
  block->decoder = diagutils_readbtype;
  block->encoder = diagutils_writebtype;
  block->window = window;
  block->field = field;
  block->min = 0;
  block->max = 15;
  block->step = 1;
  block->none = FALSE;
  Event_Claim(event_CLICK,window,field + 1,
  	      (event_handler) diagutils_popupbtype,block);
}

void _diagutils_bumpers(diagutils_block *block)
{
  Event_Claim(event_CLICK,block->window,block->field + 1,
  	      (event_handler) diagutils_dechandler,block);
  Event_Claim(event_CLICK,block->window,block->field + 2,
  	      (event_handler) diagutils_inchandler,block);
}

BOOL diagutils_dechandler(event_pollblock *event,diagutils_block *block)
{
  int value, step;

  step = block->step;
  if (step == -1)
  {
    step = 4;
    if (Kbd_KeyDown(inkey_SHIFT)) step = 2;
    if (Kbd_KeyDown(inkey_CTRL)) step = 1;
  }

  value = (*block->decoder)(block->window,block->field);
  switch (event->data.mouse.button.value)
  {
    case button_ADJUST:
      if (value < block->max)
        value = value - (value % step) + step;
      break;
    case button_SELECT:
      if (value > block->min)
        value = value - (value % step) - step;
      break;
  }
  (*block->encoder)(block->window,block->field,value);

  return TRUE;
}

BOOL diagutils_inchandler(event_pollblock *event,diagutils_block *block)
{
  int value, step;

  step = block->step;
  if (step == -1)
  {
    step = 4;
    if (Kbd_KeyDown(inkey_SHIFT)) step = 2;
    if (Kbd_KeyDown(inkey_CTRL)) step = 1;
  }

  value = (*block->decoder)(block->window,block->field);
  switch (event->data.mouse.button.value)
  {
    case button_SELECT:
      if (value < block->max)
        value = value - (value % step) + step;
      break;
    case button_ADJUST:
      if (value > block->min)
        value = value - (value % step) - step;
      break;
  }
  (*block->encoder)(block->window,block->field,value);

  return TRUE;
}

BOOL diagutils_popupbtype(event_pollblock *event,diagutils_block *block)
{
  diagutils_tickmenu((*block->decoder)(block->window,block->field));
  Menu_PopUpAuto(diagutils_menu);
  diagutils_whichfield.window = block->window;
  diagutils_whichfield.field = block->field;
  Event_Claim(event_MENU,event_ANY,event_ANY,
  	      (event_handler) diagutils_menuselect,&diagutils_whichfield);
  EventMsg_Claim(message_MENUSDELETED,event_ANY,
  	      	 (event_handler) diagutils_menurelease,
  	      	 &diagutils_whichfield);
  help_claim_menu("BTM");
  return TRUE;
}

BOOL diagutils_popupcolour(event_pollblock *event,diagutils_block *block)
{
  diagutils_whichfield.window = block->window;
  diagutils_whichfield.field = block->field;
  Icon_SetShade(colour_window,colour_NONE,!block->none);
  Menu_PopUpAuto((menu_ptr) colour_window);
  return TRUE;
}

void diagutils_writebtype(window_handle window,icon_handle field,int value)
{
  char tag[12];
  char buffer[24];

  sprintf(tag,"BType%d",value);
  MsgTrans_Lookup(messages,tag,buffer,24);
  Icon_SetText(window,field,buffer);
}

int diagutils_readbtype(window_handle window,icon_handle field)
{
  int result;
  char tag[12];
  char buffer[24];

  for (result = 0;result < 16;result++)
  {
    sprintf(tag,"BType%d",result);
    MsgTrans_Lookup(messages,tag,buffer,24);
    if (!strcmpcr(buffer,(char *) Icon_GetTextPtr(window,field)))
      return result;
  }

  return 0;
}

int diagutils_readcolour(window_handle window,icon_handle field)
{
  icon_block istate;
  int result;

  Error_Check(Wimp_GetIconState(window,field,&istate));
  result = istate.flags.data.background;
  /* Check for 'none' */
  if (result == 1 && istate.data.indirecttext.buffer[0] != '1')
    return -1;
  else
    return result;
}

void diagutils_writecolour(window_handle window,icon_handle field,int value)
{
  icon_createblock iblock;
  icon_handle trash;

  Error_Check(Wimp_GetIconState(window,field,&iblock.icondata));
  Error_Check(Wimp_DeleteIcon(window,field));

  if (value == 255 || value == -1)
  {
    MsgTrans_Lookup(messages,"ShortNone",
    		    iblock.icondata.data.indirecttext.buffer,12);
    iblock.icondata.flags.data.background = 1;
  }
  else
  {
    sprintf(iblock.icondata.data.indirecttext.buffer,"%d",value);
    iblock.icondata.flags.data.background = value;
  }
  if (value > 3 && value < 9)
    iblock.icondata.flags.data.foreground = 0;
  else
    iblock.icondata.flags.data.foreground = 7;

  iblock.window = window;
  Error_Check(Wimp_CreateIcon(&iblock,&trash));
  Icon_ForceRedraw(window,field);
}

/**
 * General purpose event handler to check the state of radio icons
 * after an Adjust click and ensure that they have not been deselected.
 * 
 * \param *event      The event poll block.
 * \param *reference  Unused.
 * \returns           FALSE, as this never claims the event.
 */
BOOL diagutils_adjust_radio(event_pollblock *event, void *reference)
{
  mouse_block *mouse;

  if (event == NULL || event->type != event_BUTTON)
    return FALSE;

  mouse = &(event->data.mouse);

  if (mouse->button.data.adjust)
    Icon_SetSelect(mouse->window, mouse->icon, 1);

  return FALSE;
}

BOOL colour_clicknone(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;

  diagutils_writecolour(diagutils_whichfield.window,
  			diagutils_whichfield.field,
  			-1);

  if (event->data.mouse.button.data.select)
    WinEd_CreateMenu((menu_ptr) -1,0,0);
  return TRUE;
}

BOOL colour_clickcolour(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;

  diagutils_writecolour(diagutils_whichfield.window,
  			diagutils_whichfield.field,
  			event->data.mouse.icon - colour_COLOUR);

  if (event->data.mouse.button.data.select)
    WinEd_CreateMenu((menu_ptr) -1,0,0);
  return TRUE;
}

BOOL diagutils_menuselect(event_pollblock *event,diagutils_block *block)
{
  mouse_block ptrinfo;

  Wimp_GetPointerInfo(&ptrinfo);

  diagutils_writebtype(block->window,block->field,event->data.selection[0]);
  diagutils_tickmenu(event->data.selection[0]);

  if (ptrinfo.button.data.adjust)
    Menu_ShowLast();

  return TRUE;
}

BOOL diagutils_menurelease(event_pollblock *event,diagutils_block *block)
{
  Event_Release(event_MENU,event_ANY,event_ANY,
  		(event_handler) diagutils_menuselect,block);
  EventMsg_Release(message_MENUSDELETED,event_ANY,
  		   (event_handler) diagutils_menurelease);
  help_release_menu();
  return TRUE;
}

void diagutils_tickmenu(int entry)
{
  int index;

  for (index = 0;index < 16;index++)
    Menu_SetFlags(diagutils_menu,index,0,0);
  Menu_SetFlags(diagutils_menu,entry,1,0);
}
