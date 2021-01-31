/* IniConfig.c */

/**
 * Handle the loading and saving of choices structures in a textual
 * INI file format.
 * 
 * This code has been structured in its own file, with a view to
 * adding it to DeskLib as a stand-alone module.
 * 
 * (c) Stephen Fryatt, 2021.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "DeskLib:Core.h"
#include "DeskLib:File.h"
#include "DeskLib:LinkList.h"

#include "IniConfig.h"

#include "globals.h"

/* The maximum length of a line in the choices file. */
#define INICONFIG_MAX_LINE 256


typedef enum {
//  iniconfig_type_STRING,
//  iniconfig_type_INT,
  iniconfig_type_BOOLEAN
} iniconfig_type;

typedef struct {
  BOOL *variable;
  BOOL initial;
} iniconfig_value_bool;

typedef union {
  iniconfig_value_bool boolean;
} iniconfig_values;

/* An entry in the Choices file list. */
typedef struct {
  linklist_header header;
  const char *name;
  iniconfig_type type;
  iniconfig_values value;
} iniconfig_entry;

typedef struct {
  linklist_header header;
  const char *name;
  linklist_header entries;
} iniconfig_section;

typedef struct iniconfig_file {
  linklist_header sections;
} iniconfig_file;

static BOOL IniConfig_UpdateEntry(iniconfig_entry *entry, char *value);
static iniconfig_section *IniConfig_FindSection(iniconfig_file *file, char *name);
static iniconfig_entry *IniConfig_FindEntry(iniconfig_section *section, char *name);
static BOOL IniConfig_StringCompare(char *lower, const char *mixed, BOOL allow_short);
static void IniConfig_StringToLower(char *str);



/**
 * Construct a new IniConfig file instance.
 * 
 * \return            Pointer to a new instance, or NULL.
 */
iniconfig_file *IniConfig_Init(void)
{
  iniconfig_file *file = NULL;

  file = malloc(sizeof(iniconfig_file));
  if (file == NULL)
    return NULL;

  LinkList_Init(&file->sections);

  IniConfig_AddSection(file, NULL);

  return file;
}

/**
 * Add a section to an IniConfig file.
 *
 * \param *file       Pointer to the IniFile instance to update.
 * \param *name       Pointer to the name of the section.
 */
void IniConfig_AddSection(iniconfig_file *file, const char *name)
{
  iniconfig_section *section = NULL;

  if (file == NULL)
    return;

  section = malloc(sizeof(iniconfig_section));
  if (section == NULL)
    return;

  Log(log_INFORMATION, "Creating section: name='%s', addr=0x%x", (name == NULL) ? "{NULL}" : name, section);

  section->name = name;

  LinkList_Init(&section->entries);

  LinkList_AddToTail(&file->sections, &section->header);
}

/**
 * Add a boolean value to an IniConfig file.
 *
 * \param *file       Pointer to the IniFile instance to update.
 * \param *name       Pointer to the name of the value.
 * \param *variable   Pointer to the associated variable.
 * \param initial     The initial value to set the variable to.
 */
void IniConfig_AddBoolean(iniconfig_file *file, const char *name, BOOL *variable, BOOL initial)
{
  iniconfig_entry *entry = NULL;
  iniconfig_section *section = NULL;

  if (variable != NULL)
    *variable = initial;

  if (file == NULL)
    return;

  section = LinkList_LastItem(&file->sections);
  if (section == NULL)
    return;

  entry = malloc(sizeof(iniconfig_entry));
  if (entry == NULL)
    return;

  Log(log_INFORMATION, "Creating entry: name='%s', addr=0x%x", (name == NULL) ? "{NULL}" : name, entry);

  entry->type = iniconfig_type_BOOLEAN;
  entry->name = name;
  entry->value.boolean.variable = variable;
  entry->value.boolean.initial = initial;

  LinkList_AddToTail(&section->entries, &entry->header);
}

/**
 * Reset the variables in an IniConfig file to their default
 * values.
 * 
 * \param *file       Pointer to the IniFile instance to update.
 */
void IniConfig_ResetDefaults(iniconfig_file *file)
{
  iniconfig_section *section;
  iniconfig_entry *entry;

  if (file == NULL)
    return;

  section = LinkList_FirstItem(&file->sections);

  while (section != NULL) {
    entry = LinkList_FirstItem(&section->entries);

    while (entry != NULL) {
      switch (entry->type) {
        case iniconfig_type_BOOLEAN:
          if (entry->value.boolean.variable != NULL)
            *entry->value.boolean.variable = entry->value.boolean.initial;
          break;
      }

      entry = LinkList_NextItem(entry);
    }

    section = LinkList_NextItem(section);
  }
}

/**
 * Write an IniConfig file to disc, storing the current values
 * of the choices variables within it.
 *
 * \param *file       Pointer to the IniFile instance to write.
 * \param *filename   Pointer to the filename to write to.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_WriteFile(iniconfig_file *file, char *filename)
{
  FILE *out = NULL;
  iniconfig_section *section;
  iniconfig_entry *entry;

  if (file == NULL)
    return FALSE;

  out = fopen(filename, "w");
	if (out == NULL)
		return FALSE;

  fprintf(out, "# >Choices\n");
  fprintf(out, "# WinEd Choices File\n");

  section = LinkList_FirstItem(&file->sections);
  while (section != NULL) {
    entry = LinkList_FirstItem(&section->entries);

    if (entry != NULL)
      fprintf(out, "\n");

    if (section->name != NULL)
      fprintf(out, "[%s]\n", section->name);

    while (entry != NULL) {
      switch (entry->type) {
        case iniconfig_type_BOOLEAN:
          fprintf(out, "%s = %s\n", entry->name, (*entry->value.boolean.variable == TRUE) ? "Yes" : "No");
          break;
      }

      entry = LinkList_NextItem(entry);
    }

    section = LinkList_NextItem(section);
  }

  fclose(out);

  File_SetType(filename, filetype_TEXT);

  return TRUE;
}

/**
 * Read an IniConfig file from disc and update the current
 * values of the choices variables according to the data within
 * it.
 *
 * \param *file       Pointer to the IniFile instance to read.
 * \param *filename   Pointer to the filename to read from.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_ReadFile(iniconfig_file *file, char *filename)
{
  FILE *in = NULL;
  BOOL result = FALSE;

  in = fopen(filename, "r");
  if (in == NULL)
    return FALSE;

  result = IniConfig_ReadFileRaw(file, in);

  fclose(in);

  return result;
}

/**
 * Read an IniConfig file from disc and update the current
 * values of the choices variables according to the data within
 * it.
 *
 * \param *file       Pointer to the IniFile instance to read.
 * \param *in         Pointer to a C file handle to read from.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_ReadFileRaw(iniconfig_file *file, FILE *in)
{
  iniconfig_section *section;
  iniconfig_entry *entry;
  char line[INICONFIG_MAX_LINE], *token, *value, *end, *tail;
  BOOL success = TRUE;

  if (in == NULL)
    return FALSE;

  section = IniConfig_FindSection(file, NULL);
  Log(log_INFORMATION, "Initial section: 0x%x", section);

  while (fgets(line, INICONFIG_MAX_LINE, in) != NULL) {
    /* Find and terminate the end of the line, including white space. */
    for (tail = line; tail - line < INICONFIG_MAX_LINE && *tail != '\0' && *tail != '\r' && *tail != '\n'; tail++);
    *tail = '\0';

    Log(log_INFORMATION, "-----------------------------------------");
    Log(log_INFORMATION, "Read Raw Line: '%s'", line);

    /* Remove white space from the end of the line. */
    for (; (tail > line) && isspace(*(tail - 1)); *--tail = '\0');

    Log(log_INFORMATION, "Read Line: '%s'", line);

    /* Move past the whitespace at the start of the line, to find the token. */
    for (token = line; *token != '\0' && isspace(*token); token++);

    /* Skip any blank lines or comments. */
    if (*token == '\0' || *token == '#') {
      Log(log_INFORMATION, "Skipping empty line or comment.");
      continue;
    }

    if (*token == '[') {
      /* If the opening character is [, find the closing half of the pair. */
      for (end = token; *end != '\0' && *end != ']'; end++);
      if (*end == '\0') {
        Log(log_WARNING, "End of section name not found.");
        success = FALSE;
        continue;
      }

      /* This must be the last thing on the line. */
      if (*(end + 1) != '\0') {
        Log(log_WARNING, "Non-space after closing ].");
        success = FALSE;
        continue;
      }

      /* Remove the brackets from the name, and make it lower case. */
      token++;
      *end = '\0';

      if (*token == '\0') {
        Log(log_WARNING, "Skipping enpty section.");
        success = FALSE;
        continue;
      }

      IniConfig_StringToLower(token);

      section = IniConfig_FindSection(file, token);
      if (section == NULL) {
        Log(log_WARNING, "Unexpected section '%s'", token);
        success = FALSE;
      } else {
        Log(log_INFORMATION, "Found section '%s' = 0x%x", token, section);
      }
    } else {
      /* Otherwise, this must be a token = value pair, so find the = */
      for (end = token; *end != '\0' && *end != '='; end++);
      if (*end == '\0') {
        Log(log_WARNING, "Token separator not found.");
        success = FALSE;
        continue;
      }

      /* The value must start after the separator. */
      value = end + 1;

      /* Remove any white space from the start of the value. */
      for (; *value != '\0' && isspace(*value); value++);

      /* If the value is quoted, remove them. */
      if ((tail > (value + 1)) && (*value == '"') && (*(tail - 1) == '"')) {
        value++;
        *--tail = '\0';
      }

      /* Terminate the token. */
      *end = '\0';

      /* Strip the trailing whitespace from the token, and make it lower case. */
      for (; (end > line) && isspace(*(end - 1)); *--end = '\0');
      if (token == '\0') {
        Log(log_WARNING, "Skipping enpty token.");
        success = FALSE;
        continue;
      }

      IniConfig_StringToLower(token);

      entry = IniConfig_FindEntry(section, token);
      if (entry != NULL) {
        Log(log_INFORMATION, "Found entry '%s' = 0x%x", token, entry);
        if (!IniConfig_UpdateEntry(entry, value)) {
          Log(log_WARNING, "Failed to update entry for '%s' with '%s'", token, value);
          success = FALSE;
        }
      } else {
        Log(log_WARNING, "Unexpected entry '%s'", token);
        success = FALSE;
      }
    }
  }

  return success;
}

/**
 * Given an entry and a value string from the file, update the entry's variable
 * with an appropriate new value.
 * 
 * \param *entry    Pointer to the entry to be updated.
 * \param *value    Pointer to the text value to be parsed.
 * \return          TRUE if successful; FALSE on failure.
 */
static BOOL IniConfig_UpdateEntry(iniconfig_entry *entry, char *value)
{
  if (entry == NULL)
    return FALSE;

  switch (entry->type) {
    case iniconfig_type_BOOLEAN:
      if (entry->value.boolean.variable == NULL)
        break;

      if (*value == '\0')
        return FALSE;

      if (IniConfig_StringCompare("yes", value, TRUE) || IniConfig_StringCompare("true", value, TRUE))
        *entry->value.boolean.variable = TRUE;
      else if (IniConfig_StringCompare("no", value, TRUE) || IniConfig_StringCompare("false", value, TRUE))
        *entry->value.boolean.variable = FALSE;
      else
        return FALSE;

      break;
  }

  return TRUE;
}

/**
 * Given a file and a string, find a matching file section. If the supplied
 * file pointer is NULL, NULL will be returned.
 *
 * \param *file     Pointer to the file to be searched.
 * \param *name     Pointer to the name to be matched, pre-converted into
 *                  lower case. If NULL, the default unnamed section will
 *                  be matched.
 * \return          Pointer to the first matching section, or NULL.
 */
static iniconfig_section *IniConfig_FindSection(iniconfig_file *file, char *name)
{
  iniconfig_section *section = NULL;

  if (file == NULL)
    return NULL;

  section = LinkList_FirstItem(&file->sections);
  while (section != NULL && !IniConfig_StringCompare(name, section->name, FALSE))
    section = LinkList_NextItem(section);

  return section;
}

/**
 * Given a section and a string, find a matching file entry. If the supplied
 * section pointer is NULL, NULL will be returned.
 *
 * \param *section  Pointer to the section to be searched.
 * \param *name     Pointer to the name to be matched, pre-converted into
 *                  lower case.
 * \return          Pointer to the first matching entry, or NULL.
 */
static iniconfig_entry *IniConfig_FindEntry(iniconfig_section *section, char *name)
{
  iniconfig_entry *entry = NULL;

  if (section == NULL)
    return NULL;

  entry = LinkList_FirstItem(&section->entries);
  while (entry != NULL && !IniConfig_StringCompare(name, entry->name, FALSE))
    entry = LinkList_NextItem(entry);

  return entry;
}

/**
 * Compare two strings case insensitively: one should already be lower case, the other
 * will have its characters converted (but not be modified in the process).
 * 
 * The strings can be required to match completely, or optionally the mixed-case
 * string can be a substring of the lower-case one starting at index zero.
 * 
 * \param *lower        Pointer to a string already converted to lower case.
 * \param *mixed        Pointer to a string in midex case.
 * \param allow short   TRUE to allow mixed to be a substring on lower.
 * \return              TRUE if the strings match; otherwise FALSE.
 */
static BOOL IniConfig_StringCompare(char *lower, const char *mixed, BOOL allow_short)
{
  int i;

  /* Two null strings match. */
  if (lower == NULL && mixed == NULL)
    return TRUE;

  /* One null string doesn't match. */
  if (lower == NULL || mixed == NULL)
    return FALSE;

  /* Match character by character. */
  for (i = 0; lower[i] == tolower(mixed[i]) && lower[i] != '\0'; i++);

  return (mixed[i] == '\0' && (lower[i] == '\0' || allow_short));
}

/**
 * Convert a string to lower case, in-situ.
 *
 * \param *str  Pointer to the string to be converted.
 */
static void IniConfig_StringToLower(char *str)
{
  for (; *str != '\0'; str++)
    *str = tolower(*str);
}
