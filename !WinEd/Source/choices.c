/* choices.c */

#include "common.h"

#include "choices.h"
#include "IniConfig.h"


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
  choices_BROWSERSORT = 26,
  choices_SAFEICONS = 27,
  choices_DEFAULT = 28,
  choices_ROUNDCOORDS = 29,
  choices_FILESORT = 30
} choices_icons;

/* The magic number which appears in legacy binary config files. */
#define WEdC 0x43644557

/* The current version number for legacy binary config files. */
#define choices_VERSION 234

/* The size of buffer for constructing filenames. */
#define CHOICES_MAX_FILENAME 256

/* The Choices filename. */
const char choices_filename[] = "Choices";

/* The Choices folder for use within Choices$Path. */
const char choices_foldername[] = "WinEd";

/* The local choices folder for non-uni-boot systems. */
const char choices_winedname[] = "<WinEd$Dir>.Resources";

/* The INI file used to store the choices. */
iniconfig_file *choices_file;

/* The header block for legacy binary config files. */
typedef struct {
  int id;
  int version;
  char creator[12];
} choices_filestr;


void choices_seticons(void);
void choices_readicons(void); /* Also calls responder */
void choices_default(void);

static BOOL choices_load(void);
static BOOL choices_save(void);

BOOL choices_clickok(event_pollblock *event,void *reference);
BOOL choices_clickcancel(event_pollblock *event,void *reference);
BOOL choices_clickdefault(event_pollblock *event,void *reference);

/* The choices data structure. */
static choices_str choices_data;

/* Set the public reference to the choices data structure. */
choices_str *choices = &choices_data;

static choices_responder global_responder;

/* The handle of the Choices window. */
static window_handle choices_window;

/**
 * Initialise the choices system.
 *
 * \param responder   Pointer to a function to be called when the choices change.
 */
void choices_init(choices_responder responder)
{
  window_block *templat;

  /* Load the Choices window template. */
  templat = templates_load("Choices",0,0,0,0);
  Error_CheckFatal(Wimp_CreateWindow(templat,&choices_window));
  free(templat);

  /* Set up event handlers for the Choices window. */
  Event_Claim(event_OPEN,choices_window,event_ANY,Handler_OpenWindow,0);
  Event_Claim(event_CLOSE,choices_window,event_ANY,Handler_CloseWindow,0);
  Event_Claim(event_CLICK,choices_window,choices_OK,choices_clickok,0);
  Event_Claim(event_CLICK,choices_window,choices_CANCEL,choices_clickcancel,0);
  Event_Claim(event_CLICK,choices_window,choices_SAVE,choices_clickok,0);
  Event_Claim(event_CLICK,choices_window,choices_DEFAULT,choices_clickdefault,0);
  help_claim_window(choices_window,"CHO");

  /* Store the function to be called when choices change. */
  global_responder = responder;

  /* Initialise the choices file handler. */
  choices_file = IniConfig_Init();

  /* Initialise the choices, and set their default values. */
  IniConfig_AddSection(choices_file, "Windows");
  IniConfig_AddBoolean(choices_file, "AutomaticMonitor", &(choices->monitor), TRUE);
  IniConfig_AddBoolean(choices_file, "AutomaticPicker", &(choices->picker), TRUE);
  IniConfig_AddBoolean(choices_file, "BrowserToolbar", &(choices->browtools), FALSE);
  IniConfig_AddBoolean(choices_file, "EditorPanes", &(choices->editpanes), TRUE);
  IniConfig_AddBoolean(choices_file, "EditableToolbar", &(choices->viewtools), TRUE);
  IniConfig_AddBoolean(choices_file, "EditableFurniture", &(choices->furniture), TRUE);
  IniConfig_AddSection(choices_file, "Editing");
  IniConfig_AddBoolean(choices_file, "KeyboardShortcuts", &(choices->hotkeys), TRUE);
  IniConfig_AddBoolean(choices_file, "MoveSelWithoutMouse", &(choices->mouseless_move), TRUE);
  IniConfig_AddBoolean(choices_file, "RoundCoordinates", &(choices->round_coords), TRUE);
  IniConfig_AddBoolean(choices_file, "ConfirmDelete", &(choices->confirm), FALSE);
  IniConfig_AddBoolean(choices_file, "SafeIconUpdate", &(choices->safe_icons), FALSE);
  IniConfig_AddSection(choices_file, "Display");
  IniConfig_AddBoolean(choices_file, "AlwaysShowBorders", &(choices->borders), FALSE);
  IniConfig_AddBoolean(choices_file, "HatchUserRedraw", &(choices->hatchredraw), TRUE);
  IniConfig_AddBoolean(choices_file, "DisplayTemplatesSorted", &(choices->browser_sort), TRUE);
  IniConfig_AddSection(choices_file, "Misc");
  IniConfig_AddBoolean(choices_file, "AutoLoadSprites", &(choices->autosprites), TRUE);
  IniConfig_AddBoolean(choices_file, "FormEdCompatible", &(choices->formed), TRUE);
  IniConfig_AddBoolean(choices_file, "SaveFilesSorted", &(choices->file_sort), FALSE);
  IniConfig_AddBoolean(choices_file, "StrictPanes", &(choices->strict_panes), TRUE);

  /* Load any user-saved choices from disc. */
  choices_load();
}

/**
 * Set the icons in the Choices window.
 */
void choices_seticons()
{
  Icon_SetSelect(choices_window,choices_MONITOR,choices->monitor);
  Icon_SetSelect(choices_window,choices_PICKER,choices->picker);
  Icon_SetSelect(choices_window,choices_BROWTOOLS,choices->browtools);
  Icon_SetSelect(choices_window,choices_VIEWTOOLS,choices->viewtools);
  Icon_SetSelect(choices_window,choices_HATCHREDRAW,choices->hatchredraw);
  Icon_SetSelect(choices_window,choices_FURNITURE,choices->furniture);
  Icon_SetSelect(choices_window,choices_BROWSERSORT,choices->browser_sort);
  Icon_SetSelect(choices_window,choices_HOTKEYS,choices->hotkeys);
  Icon_SetSelect(choices_window,choices_AUTOSPRITES,choices->autosprites);
  Icon_SetSelect(choices_window,choices_EDITPANES,choices->editpanes);
  Icon_SetSelect(choices_window,choices_MOUSELESSMOVE,choices->mouseless_move);
  Icon_SetSelect(choices_window,choices_CONFIRM,choices->confirm);
  Icon_SetSelect(choices_window,choices_BORDERS,choices->borders);
  Icon_SetSelect(choices_window,choices_FORMED,choices->formed);
  Icon_SetSelect(choices_window,choices_STRICTPANES,choices->strict_panes);
  Icon_SetSelect(choices_window,choices_SAFEICONS,choices->safe_icons);
  Icon_SetSelect(choices_window,choices_ROUNDCOORDS,choices->round_coords);
  Icon_SetSelect(choices_window,choices_FILESORT,choices->file_sort);
}

/**
 * Open the Choices window.
 */
void choices_open()
{
  choices_seticons();
  Window_Show(choices_window,open_UNDERPOINTER);
}

/**
 * Read the icon settings from the Choices window.
 */
void choices_readicons()
{
  choices_str old;

  old = *choices;

  choices->monitor = Icon_GetSelect(choices_window,choices_MONITOR);
  choices->picker = Icon_GetSelect(choices_window,choices_PICKER);
  choices->browtools = Icon_GetSelect(choices_window,choices_BROWTOOLS);
  choices->viewtools = Icon_GetSelect(choices_window,choices_VIEWTOOLS);
  choices->hatchredraw = Icon_GetSelect(choices_window,choices_HATCHREDRAW);
  choices->browser_sort = Icon_GetSelect(choices_window,choices_BROWSERSORT);
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
  choices->safe_icons = Icon_GetSelect(choices_window,choices_SAFEICONS);
  choices->round_coords = Icon_GetSelect(choices_window,choices_ROUNDCOORDS);
  choices->file_sort = Icon_GetSelect(choices_window,choices_FILESORT);

  (*global_responder)(&old,choices);
}

/**
 * Handle clicks on the OK and Save buttons.
 *
 * \param *event      The Wimp event block.
 * \param *reference  Not used, so NULL.
 * \return            TRUE if handled; otherwise FALSE.
 */
BOOL choices_clickok(event_pollblock *event,void *reference)
{
  BOOL success = TRUE;

  if (event->data.mouse.button.data.menu)
    return FALSE;

  choices_readicons();

  if (event->data.mouse.icon == choices_SAVE)
    success = choices_save();

  if (success && event->data.mouse.button.data.select)
    Wimp_CloseWindow(choices_window);

  return TRUE;
}

/**
 * Handle clicks on the Cancel button.
 *
 * \param *event      The Wimp event block.
 * \param *reference  Not used, so NULL.
 * \return            TRUE if handled; otherwise FALSE.
 */
BOOL choices_clickcancel(event_pollblock *event,void *reference)
{
  if (event->data.mouse.button.data.menu)
    return FALSE;

  if (event->data.mouse.button.data.select)
    Wimp_CloseWindow(choices_window);
  else if (event->data.mouse.button.data.adjust)
    choices_seticons();

  return TRUE;
}

/**
 * Handle clicks on the Default button.
 *
 * \param *event      The Wimp event block.
 * \param *reference  Not used, so NULL.
 * \return            TRUE if handled; otherwise FALSE.
 */
BOOL choices_clickdefault(event_pollblock *event,void *reference)
{
  choices_str saved;

  if (!event->data.mouse.button.data.select)
    return FALSE;

  saved = *choices;

  IniConfig_ResetDefaults(choices_file);
  choices_seticons();

  *choices = saved;

  return TRUE;
}

/**
 * Load a choices file from disc.
 * 
 * \return TRUE on success; FALSE on failure.
 */
static BOOL choices_load(void)
{
  FILE *in;
  BOOL global_choices = FALSE, legacy_file = FALSE, result = FALSE;
  int object_type = 0;
  char filename[CHOICES_MAX_FILENAME];
  choices_filestr header;
  long filesize;

  /* Check if there is a global choices setup available. */

  global_choices = Environment_SysVarRead("Choices$Path", NULL, 0);

  /* Attempt to find a file somewhere. */

  snprintf(filename, CHOICES_MAX_FILENAME, "Choices:%s.%s", choices_foldername, choices_filename);
  filename[CHOICES_MAX_FILENAME - 1] = '\0';

  SWI(2, 1, SWI_OS_File, 17, filename, &object_type);

  if (object_type == 0)
  {
    snprintf(filename, CHOICES_MAX_FILENAME, "%s.%s", choices_winedname, choices_filename);
    filename[CHOICES_MAX_FILENAME - 1] = '\0';

    SWI(2, 1, SWI_OS_File, 17, filename, &object_type);
    if (object_type == 1)
    {
      if (global_choices)
        WinEd_MsgTrans_ReportPS(messages, "LocalChoice", FALSE, 0, 0, 0, 0);
    }
    else
    {
      if (object_type != 0)
      {
        WinEd_MsgTrans_ReportPS(messages, "BadLoadChoice", FALSE, filename, 0, 0, 0);
        return FALSE;
      }
    }
  }
  else if (object_type != 1)
  {
    WinEd_MsgTrans_ReportPS(messages, "BadLoadChoice", FALSE, filename, 0, 0, 0);
    return FALSE;
  }

  /* Load the file. */

  in = fopen(filename, "rb");
  if (in == NULL)
    return FALSE;

  /* Get the filesize and check that it is sane for a legacy file. */

  fseek(in, 0, SEEK_END);
  filesize = ftell(in);
  rewind(in);

  if ((filesize > sizeof(choices_filestr)) && (filesize <= (sizeof(choices_filestr) + sizeof(choices_str))))
    legacy_file = TRUE;

  /* If it does, load the file header in and check if it looks like a legacy file. */

  if (legacy_file)
  {
    Log(log_INFORMATION, "File size is %d bytes, so trying for a legacy file.", filesize);

    if (fread(&header, sizeof(choices_filestr), 1, in) != 1)
    {
      Log(log_ERROR, "Failed to read in file header.");
      fclose(in);
      return FALSE;
    }

    /* Are the file ID and version number OK? */

    if (header.id != WEdC)
    {
      Log(log_INFORMATION, "The file ID block isn't WEdC, so try as an INI file.");
      legacy_file = FALSE;
      rewind(in);
    }
    else if (header.version > choices_VERSION)
    {
      Log(log_INFORMATION, "The version isn't one we can handle, so skip the load.");
      fclose(in);
      return FALSE;
    }
  }

  /* Load the file, either as a legacy file or as an INI file. */

  if (legacy_file)
  {
    Log(log_INFORMATION, "Loading as a legacy file...");
    if (fread(&choices_data, filesize - sizeof(choices_filestr), 1, in) == 1)
    {
      result = TRUE;
    }
    else
    {
      IniConfig_ResetDefaults(choices_file);
      Log(log_WARNING, "Failed to load OK");
    }
  }
  else
  {
    Log(log_INFORMATION, "Loading as an INI file...");
    result = IniConfig_ReadFileRaw(choices_file, in);
  }

  fclose(in);

  if (!result)
    WinEd_MsgTrans_ReportPS(messages, "BadChoices", FALSE, filename, 0, 0, 0);

  return result;
}

/**
 * Save a choices file to disc.
 * 
 * \return TRUE on success; FALSE on failure.
 */
static BOOL choices_save(void)
{
  BOOL global_choices = FALSE;
  int object_type = 0;
  char filename[CHOICES_MAX_FILENAME];

  /* Check if there is a global choices setup available. */

  global_choices = Environment_SysVarRead("Choices$Write", NULL, 0);

  if (global_choices)
  {
    /* If there are global choices, make sure that our folder is created. */

    snprintf(filename, CHOICES_MAX_FILENAME, "<Choices$Write>.%s", choices_foldername);
    filename[CHOICES_MAX_FILENAME - 1] = '\0';

    SWI(2, 1, SWI_OS_File, 17, filename, &object_type);

    if (object_type == 0)
    {
      File_CreateDir(filename);
    }
    else if (object_type != 2 && object_type != 3)
    {
      WinEd_MsgTrans_ReportPS(messages, "BadSaveChoice", FALSE, filename, 0, 0, 0);
      return FALSE;
    }

    snprintf(filename, CHOICES_MAX_FILENAME, "<Choices$Write>.%s.%s", choices_foldername, choices_filename);
    filename[CHOICES_MAX_FILENAME - 1] = '\0';
  }
  else
  {
    /* If there are not global choices, save into our application. */

    snprintf(filename, CHOICES_MAX_FILENAME, "%s.%s", choices_winedname, choices_filename);
    filename[CHOICES_MAX_FILENAME - 1] = '\0';
  }

  /* Make sure that the target file isn't a folder. */

  SWI(2, 1, SWI_OS_File, 17, filename, &object_type);

  if (object_type != 0 && object_type != 1)
  {
    WinEd_MsgTrans_ReportPS(messages, "BadSaveChoice", FALSE, filename, 0, 0, 0);
    return FALSE;
  }

  /* Save the file. */

  return IniConfig_WriteFile(choices_file, filename);
}
