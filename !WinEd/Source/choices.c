/* choices.c */

#include "common.h"

#include "choices.h"


/* Icons for window Choices */
typedef enum {
  choices_PICKER = 4,
  choices_BROWTOOLS = 5,
  choices_VIEWTOOLS = 6,
  choices_HOTKEYS = 7,
  choices_HATCHREDRAW = 8,
  choices_FURNITURE = 9,
  choices_AUTOSPRITES = 10,
  choices_EDITPANES = 11,
  choices_MOUSELESSMOVE = 12,
  choices_BORDERS = 13,
  choices_CONFIRM = 14,
  choices_FORMED = 15,
  choices_STRICTPANES = 16,
  choices_MONITOR = 17,
  choices_SAVE = 19,
  choices_OK = 21,
  choices_CANCEL = 23,
  choices_ROUNDSORT = 26
} choices_icons;

#define WEdC 0x43644557
#define choices_VERSION 131

typedef struct {
  int id;
  int version;
  char creator[12];
  choices_str choices;
} choices_filestr;

/* Can't use Resource because it's a ...$Path and we need to save as well */
const char choices_pathname[] = "Choices:WinEd.Choices";
const char choices_winedsavename[] = "<WinEd$Dir>.Resources.Choices";
const char choices_savename[] = "<Choices$Write>.WinEd.Choices";
/*const char choices_existingsave[] = "Choices:Wined.Choices";*/
#define choices_existingsave choices_savename
const char choices_var[] = "Choices$Write";
const char choices_makedir[] = "<Choices$Write>.WinEd";
const char choices_existingdir[] = "Choices:WinEd";

void choices_seticons(void);
void choices_readicons(void); /* Also calls responder */
void choices_default(void);

BOOL choices_clickok(event_pollblock *event,void *reference);
BOOL choices_clickcancel(event_pollblock *event,void *reference);

static choices_filestr choices_buffer;
choices_str *choices = &choices_buffer.choices;

static choices_responder global_responder;
static window_handle choices_window;

void choices_init(choices_responder responder)
{
  window_block *templat;

  templat = templates_load("Choices",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&choices_window));
  free(templat);

  Event_Claim(event_OPEN,choices_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,choices_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,choices_window,choices_OK,choices_clickok,0);
  Event_Claim(event_CLICK,choices_window,choices_CANCEL,choices_clickcancel,0);
  Event_Claim(event_CLICK,choices_window,choices_SAVE,choices_clickok,0);
  help_claim_window(choices_window,"CHO");

  global_responder = responder;

  choices_default();

  if (File_Size((char *) choices_pathname) > sizeof(choices_filestr))
    return;
  if (File_LoadTo((char *) choices_pathname, &choices_buffer, 0))
    return;
  if (choices_buffer.id != WEdC ||
      choices_buffer.version > choices_VERSION)
    choices_default();
}

void choices_default()
{
  choices_buffer.id = WEdC;
  choices_buffer.version = choices_VERSION;
  strcpy(choices_buffer.creator,APPNAME);

  choices->autosprites =
  choices->monitor =
  choices->picker =
  choices->hotkeys =
  choices->hatchredraw =
  /* Note, round now means "display templates sorted in browser" */
  choices->round =
  choices->furniture =
  choices->editpanes =
  choices->formed =
  choices->mouseless_move =
  choices->strict_panes =
  choices->viewtools = TRUE;
  choices->confirm =
  choices->borders =
  choices->browtools = FALSE;
}

void choices_seticons()
{
  Icon_SetSelect(choices_window,choices_MONITOR,choices->monitor);
  Icon_SetSelect(choices_window,choices_PICKER,choices->picker);
  Icon_SetSelect(choices_window,choices_BROWTOOLS,choices->browtools);
  Icon_SetSelect(choices_window,choices_VIEWTOOLS,choices->viewtools);
  Icon_SetSelect(choices_window,choices_HATCHREDRAW,choices->hatchredraw);
  Icon_SetSelect(choices_window,choices_FURNITURE,choices->furniture);
  Icon_SetSelect(choices_window,choices_ROUNDSORT,choices->round);
  Icon_SetSelect(choices_window,choices_HOTKEYS,choices->hotkeys);
  Icon_SetSelect(choices_window,choices_AUTOSPRITES,choices->autosprites);
  Icon_SetSelect(choices_window,choices_EDITPANES,choices->editpanes);
  Icon_SetSelect(choices_window,choices_MOUSELESSMOVE,choices->mouseless_move);
  Icon_SetSelect(choices_window,choices_CONFIRM,choices->confirm);
  Icon_SetSelect(choices_window,choices_BORDERS,choices->borders);
  Icon_SetSelect(choices_window,choices_FORMED,choices->formed);
  Icon_SetSelect(choices_window,choices_STRICTPANES,choices->strict_panes);
}

void choices_open()
{
  choices_seticons();
  Window_Show(choices_window,open_UNDERPOINTER);
}

void choices_readicons()
{
  choices_str old;

  old = *choices;

  choices->monitor = Icon_GetSelect(choices_window,choices_MONITOR);
  choices->picker = Icon_GetSelect(choices_window,choices_PICKER);
  choices->browtools = Icon_GetSelect(choices_window,choices_BROWTOOLS);
  choices->viewtools = Icon_GetSelect(choices_window,choices_VIEWTOOLS);
  choices->hatchredraw = Icon_GetSelect(choices_window,choices_HATCHREDRAW);
  /* Note: this isn't "round". The flag is re-used to determine whether templates are sorted in browser view */
  choices->round = Icon_GetSelect(choices_window,choices_ROUNDSORT);
  choices->furniture = Icon_GetSelect(choices_window,choices_FURNITURE);
  choices->hotkeys = Icon_GetSelect(choices_window,choices_HOTKEYS);
  choices->autosprites = Icon_GetSelect(choices_window,choices_AUTOSPRITES);
  choices->editpanes = Icon_GetSelect(choices_window,choices_EDITPANES);
  choices->mouseless_move =
  	Icon_GetSelect(choices_window,choices_MOUSELESSMOVE);
  choices->confirm = Icon_GetSelect(choices_window,choices_CONFIRM);
  choices->borders = Icon_GetSelect(choices_window,choices_BORDERS);
  choices->formed = Icon_GetSelect(choices_window,choices_FORMED);
  choices->strict_panes = Icon_GetSelect(choices_window,choices_STRICTPANES);

  (*global_responder)(&old,choices);
}

BOOL choices_clickok(event_pollblock *event,void *reference)
{
  browser_fileinfo *browser;

  if (event->data.mouse.button.data.menu)
    return FALSE;

  choices_readicons();

  if (event->data.mouse.icon == choices_SAVE)
  {
    const char *choices_file = choices_pathname;
    	/* Any initialiser will do, just suppresses warning */
    /* Check for <Choices$Path> */
    int rvresult;
    SWI(5, 3, SWI_OS_ReadVarVal, choices_var, NULL, -1, 0, 0,
    	NULL, NULL, &rvresult);
    if (rvresult < 0)
    {
      /* Check for existence of WinEd directory in Choices$Path */
      int objtype;
      SWI(2,1,SWI_OS_File,5,choices_existingdir,&objtype);
      switch (objtype)
      {
        case 1:
          choices_file = choices_winedsavename;
          break;
        case 2: case 3:
          /* Try to save in existing Choices:WinEd and if that fails
             try next clause */
          if (!SWI(6,0,SWI_OS_File,10,
                (int) choices_existingsave,filetype_DATA,0,
                (int) &choices_buffer,
                (int) &choices_buffer + sizeof(choices_filestr)))
          {
            if (event->data.mouse.button.data.select)
              Wimp_CloseWindow(choices_window);
            return TRUE;
          }
        case 0:
          /* Make directory in Choices$Write */
          if (SWI(5,0,SWI_OS_File,8,choices_makedir,0,0,0))
            choices_file = choices_winedsavename;
          else
            choices_file = choices_savename;
          break;
      }
    }
    else
      choices_file = choices_winedsavename;

    Error_Check(SWI(6,0,SWI_OS_File,10,(int) choices_file,filetype_DATA,0,
                (int) &choices_buffer,
                (int) &choices_buffer + sizeof(choices_filestr)));
  }

  if (event->data.mouse.button.data.select)
    Wimp_CloseWindow(choices_window);

  /* Need to check if any browser windows are open in case "sort" has changed */
  for (browser = LinkList_NextItem(&browser_list); browser; browser = LinkList_NextItem(browser))
  {
    if (Window_IsOpen(browser->window)) browser_sorticons(browser, TRUE, FALSE, TRUE);
  }

  return TRUE;
}

BOOL choices_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;

  Wimp_CloseWindow(choices_window);

  return TRUE;
}
