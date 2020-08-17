#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DeskLib:MsgTrans.h"
#include "DeskLib:Validation.h"
#include "DeskLib:Debug.h"
#include "DeskLib:Time.h"

#include "globals.h"
#include "export.h"

/**
 * The maximum length of a filename.
 */
#define EXPORT_MAX_FILENAME 256

/**
 * The maximum length of a field.
 */
#define EXPORT_MAX_FIELD 64

/**
 * The maximum length of an icon name.
 */
#define EXPORT_MAX_ICONNAME 128

/**
 * The maximum length of an exported line.
 */
#define EXPORT_MAX_LINE 256

/**
 * The line pointer for a line which hasn't been started.
 */
#define EXPORT_PTR_UNSTARTED (-1)

/**
 * A structure holding details of a file export.
 */
struct export_handle
{
  enum export_format format;          /**< The format to export in.                                  */
  enum export_flags flags;            /**< The options to apply to the export.                       */
  char filename[EXPORT_MAX_FILENAME]; /**< The full name of the export file.                         */
  FILE *fp;                           /**< The handle of the export file.                            */
  char time[EXPORT_MAX_FIELD];        /**< The time of the export.                                   */
  char prefix[EXPORT_MAX_FIELD];      /**< The prefix to use for entries.                            */
  char line[EXPORT_MAX_LINE];         /**< The buffer holding the current line.                      */
  int ptr;                            /**< A pointer to the next free character in the current line. */
  int linenumber;                     /**< The current line number.                                  */
};

/**
 * Write the current line out to the file, in the appropriate
 * format and reset the line pointer ready for a new line.
 * 
 * \param *instance   The export instance to process.
 */
static void export_write_line(struct export_handle *instance)
{
  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED)
    return;

  /* Write any pre-line formatting. */

  switch (instance->format)
  {
    case EXPORT_FORMAT_BASIC:
      fputc(0x0d, instance->fp);
      fputc((instance->linenumber >> 8) & 0xff, instance->fp);
      fputc(instance->linenumber & 0xff, instance->fp);
      fputc(instance->ptr + 4, instance->fp);
      break;
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_CENUM:
    case EXPORT_FORMAT_MESSAGES:
    case EXPORT_FORMAT_NONE:
      break;
  }

  /* Write the line itself. */

  fputs(instance->line, instance->fp);

  /* Write any post-line formatting. */

  switch (instance->format)
  {
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_CENUM:
    case EXPORT_FORMAT_MESSAGES:
      fputc('\n', instance->fp);
      break;
    case EXPORT_FORMAT_BASIC:
    case EXPORT_FORMAT_NONE:
      break;
  }

  /* Reset the line pointer. */

  instance->ptr = EXPORT_PTR_UNSTARTED;
}

/**
 * Start a new line of output, writing any existing line which
 * is held in the buffer out first.
 *
 * \param *instance   The export instance to process.
 */
static void export_start_line(struct export_handle *instance)
{
  if (instance == NULL)
    return;

  /* If there's already a buffered line, write it out. */

  if (instance->ptr != EXPORT_PTR_UNSTARTED)
    export_write_line(instance);

  /* Start a new line. */

  instance->ptr = 0;
  instance->line[0] = '\0';
  instance->linenumber++;
}

/**
 * Append text to the currently buffered line of output, starting
 * that line if it is currently unstarted.
 * 
 * The text should NOT contain newline or carriage return characters,
 * since these will confuse the BASIC line formatter.
 *
 * \param *instance   The export instance to process.
 * \param *text       Pointer to the text to be added.
 */
static void export_append_text(struct export_handle *instance, char *text)
{
  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED)
    return;

  /* Append the text, and terminate. */

  while (*text != '\0' && instance->ptr < EXPORT_MAX_LINE)
    instance->line[instance->ptr++] = *text++;

  if (instance->ptr < EXPORT_MAX_LINE)
    instance->line[instance->ptr] = '\0';

  /* Force termination for a buffer overrun. */

  instance->line[EXPORT_MAX_LINE - 1] = '\0';
}


static void export_append_format(struct export_handle *instance, char *cntrl_string, ...)
{
  int written, length;
  va_list ap;

  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED)
    return;

  length = EXPORT_MAX_LINE - instance->ptr;
  if (length <= 0)
    return;

  va_start(ap, cntrl_string);
  written = vsnprintf(instance->line + instance->ptr, length, cntrl_string, ap);

  if (written >= length) {
    instance->line[EXPORT_MAX_LINE - 1] = '\0';
    written = length - 1;
  }

  instance->ptr += written;
}


/**
 * Add a start of comment sequence to the currently buffered line
 * of output, including a trailing space for clarity.
 * 
 * \param *instance   The export instance to process.
 */
static void export_start_comment(struct export_handle *instance)
{
  if (instance == NULL)
    return;

  switch (instance->format)
  {
    case EXPORT_FORMAT_BASIC:
      export_append_text(instance, "\xf4 ");
      break;
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_CENUM:
      export_append_text(instance, "/* ");
      break;
    case EXPORT_FORMAT_MESSAGES:
      export_append_text(instance, "# ");
      break;
    case EXPORT_FLAGS_NONE:
      break;
  }
}

/**
 * Add an end of comment sequence to the currently buffered line
 * of output, including a trailing space for clarity.
 * 
 * \param *instance   The export instance to process.
 */
static void export_end_comment(struct export_handle *instance)
{
  if (instance == NULL)
    return;

  switch (instance->format)
  {
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_CENUM:
      export_append_text(instance, " */");
      break;
   case EXPORT_FORMAT_MESSAGES:
   case EXPORT_FORMAT_BASIC:
   case EXPORT_FLAGS_NONE:
      break;
  }
}

/**
 * Open a file for exporting icon names in a specified format.
 *
 * \param format      The format to use for export.
 * \param flags       The options to apply to the export.
 * \param *filename   The filename to open.
 * \return            Pointer to an export instance, or NULL on failure.
 */
struct export_handle *export_openfile(enum export_format format, enum export_flags flags, char *filename)
{
  struct export_handle *new = NULL;
  unsigned char ro_time[5];

  /* Allocate memory for the export instance block. */

  new = malloc(sizeof(struct export_handle));
  if (new == NULL)
    return NULL;

  /* Open a file for output. */

  new->fp = fopen(filename, "w");
  if (new->fp == NULL)
  {
    free(new);
    return NULL;
  }

  /* Set the block up. */

  new->format = format;
  new->flags = flags;
  new->ptr = EXPORT_PTR_UNSTARTED;
  new->linenumber = 0;

  /* Store the filename of the output file. */

  strncpy(new->filename, filename, EXPORT_MAX_FILENAME);
  new->filename[EXPORT_MAX_FILENAME - 1] = '\0';

  strncpy(new->prefix, "_PP_", EXPORT_MAX_FIELD);
  new->prefix[EXPORT_MAX_FIELD - 1] = '\0';

  /* Get the time as a string, for use by the output code. */

  Time_ReadClock(ro_time);
  Time_ConvertDateAndTime(ro_time, new->time, EXPORT_MAX_FIELD, "%ce%yr/%mn/%dy, %24:%mi:%se");

  return new;
}

/**
 * Close the file associated with an export instance, and
 * tidy up.
 * 
 * \param *instance   The export instance to close.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL export_closefile(struct export_handle *instance)
{
  int index, type;
  BOOL outcome = FALSE;

  if (instance == NULL)
    return FALSE;

  /* Write out any line which is currently buffered. */

  export_write_line(instance);

  /* Write any file termination which is required. */

  switch (instance->format)
  {
    case EXPORT_FORMAT_BASIC:
      fputc(0x0d, instance->fp);
      fputc(0xff, instance->fp);
      break;
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_CENUM:
    case EXPORT_FORMAT_MESSAGES:
    case EXPORT_FORMAT_NONE:
      break;
  }

  /* Close the file. */

  index = fclose(instance->fp);

  /* Type the file if it was created OK. */

  if (index != EOF)
  {
    switch (instance->format)
    {
      case EXPORT_FORMAT_BASIC:
        type = 0xffb;
        break;
      default:
        type = 0xfff;
        break;
    }

    File_SetType(instance->filename, type);

    outcome = TRUE;
  }
  else
  {
    File_Delete(instance->filename);
  }

  /* Free our instance data. */

  free(instance);

  return outcome;
}

/**
 * Substutute a character based on the valid characters
 * in a given language.
 * 
 * \param c       The character to process.
 * \param basic   TRUE if the target language is BASIC.
 * \param first   TRUE if this is the first character in an identifier.
 * \return        The updated character.
 */
static char export_substitute_character(char c, BOOL basic, BOOL first)
{
  if (basic && c == '`')
    return c;

  if (strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c) != NULL)
    return c;

  if (!first && strchr("0123456789", c) != NULL)
    return c;

  return '_';
}

/**
 * Convert the case of the contents of a text buffer and remove
 * any invalid characters according to the selected choices.
 * 
 * \param *instance   The export instance to operate on.
 * \param *buffer     The buffer to process.
 */
static void export_process_lang_name(struct export_handle *instance, char *buffer)
{
  BOOL first = TRUE, basic = FALSE;

  if (instance == NULL || buffer == NULL)
    return;

  /* There's no need to special-case the first character if we're prefixing
   * all names with an underscore anyway. */

  if (instance->flags && EXPORT_FLAGS_PREFIX)
    first = FALSE;

  if (instance->format == EXPORT_FORMAT_BASIC)
    basic = TRUE;

  while (*buffer != '\0')
  {
    *buffer = export_substitute_character(*buffer, basic, first);
    first = FALSE;

    if (instance->flags & EXPORT_FLAGS_UPPERCASE)
      *buffer = toupper(*buffer);
    else if (instance->flags & EXPORT_FLAGS_LOWERCASE)
      *buffer = tolower(*buffer);

    buffer++;
  }
}

/**
 * Export the icon names from a winentry.
 * 
 * \param *instance   The export instance to operate on.
 * \param *winentry   The winentry to export.
 */
void export_winentry(struct export_handle *instance, browser_winentry *winentry)
{
  char winname[wimp_MAXNAME], iconname[EXPORT_MAX_ICONNAME], buffer[EXPORT_MAX_LINE];
  int icon, last_icon = -2, namelen;
  BOOL foundicons = FALSE;

  Log(log_INFORMATION, "Exporting window %s", winentry->identifier);

  if (instance == NULL || winentry == NULL)
    return;

  /* Copy the window name and sanitise it for the target language. */

  strncpy(winname, winentry->identifier, wimp_MAXNAME);
  winname[wimp_MAXNAME - 1] = '\0';
  export_process_lang_name(instance, winname);

  /* Process all of the icons in the window. */

  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
    namelen = extract_iconname(winentry, icon, iconname, EXPORT_MAX_ICONNAME);
    if (namelen == 0)
      continue;

    /* Sanitise the icon name for the target language. */

    export_process_lang_name(instance, iconname);

    /* Clear any unfinished business from the previous line before
     * on this icon. */

    if (foundicons == FALSE)
    {
      /* This is the first icon in the window, so write the window title comment. */

      MsgTrans_LookupPS(messages, "ExportWindow", buffer, EXPORT_MAX_LINE, winentry->identifier, 0, 0, instance->time);

      if (strlen(buffer) > 0) {
        if (instance->ptr != EXPORT_PTR_UNSTARTED)
          export_start_line(instance);
 
        export_start_line(instance);
        export_start_comment(instance);
        export_append_text(instance, buffer);
        export_end_comment(instance);
      }

      /* Perform any window initialisation for the target format. */

      switch (instance->format)
      {
        case EXPORT_FORMAT_CENUM:
          export_start_line(instance);
          export_append_text(instance, "typedef enum {");
          break;
        case EXPORT_FORMAT_BASIC:
        case EXPORT_FORMAT_CDEFINE:
        case EXPORT_FORMAT_MESSAGES:
        case EXPORT_FORMAT_NONE:
          break;
      }
    }
    else
    {
      /* This is a follow-up icon, so perform any finishing touches to the previous line. */

      switch (instance->format)
      {
        case EXPORT_FORMAT_CENUM:
          export_append_text(instance, ",");
          break;
        case EXPORT_FORMAT_BASIC:
        case EXPORT_FORMAT_CDEFINE:
        case EXPORT_FORMAT_MESSAGES:
        case EXPORT_FORMAT_NONE:
          break;
      }
    }

    foundicons = TRUE;

    /* Start the new line. */

    export_start_line(instance);

    /* Output the icon details. */

    switch (instance->format)
    {
      case EXPORT_FORMAT_CENUM:
        export_append_text(instance, "  ");

        if (instance->flags & EXPORT_FLAGS_PREFIX)
          export_append_text(instance, instance->prefix);

        if (instance->flags & EXPORT_FLAGS_WINNAME)
          export_append_format(instance, "%s_", winname);

        export_append_text(instance, iconname);

        if (!(instance->flags & EXPORT_FLAGS_SKIPIMPLIED) || (last_icon != (icon - 1)))
          export_append_format(instance, " = %d", icon);
        break;

      case EXPORT_FORMAT_CDEFINE:
        export_append_text(instance, "#define ");

        if (instance->flags & EXPORT_FLAGS_PREFIX)
          export_append_text(instance, instance->prefix);

        if (instance->flags & EXPORT_FLAGS_WINNAME)
          export_append_format(instance, "%s_", winname);

        export_append_text(instance, iconname);

        export_append_format(instance, "(%d)", icon);
        break;

      case EXPORT_FORMAT_BASIC:
        if (instance->flags & EXPORT_FLAGS_PREFIX)
          export_append_text(instance, instance->prefix);

        if (instance->flags & EXPORT_FLAGS_WINNAME)
          export_append_format(instance, "%s_", winname);

        export_append_text(instance, iconname);

        if (!(instance->flags & EXPORT_FLAGS_USEREAL))
          export_append_text(instance, "%");

        export_append_format(instance, " = %d", icon);
        break;

      case EXPORT_FORMAT_MESSAGES:
        if (instance->flags & EXPORT_FLAGS_PREFIX)
          export_append_text(instance, instance->prefix);

        if (instance->flags & EXPORT_FLAGS_WINNAME)
          export_append_format(instance, "%s_", winname);

        export_append_text(instance, iconname);
        break;

      case EXPORT_FORMAT_NONE:
        break;
    }

    last_icon = icon;
  }

  /* If any icons were found, write the tail of the window data */

  if (foundicons)
  {
      switch (instance->format)
      {
        case EXPORT_FORMAT_CENUM:
          export_start_line(instance);
          snprintf(buffer, EXPORT_MAX_LINE, "} %s_icons;", winname);
          export_append_text(instance, buffer);
          break;
        case EXPORT_FORMAT_BASIC:
        case EXPORT_FORMAT_CDEFINE:
        case EXPORT_FORMAT_MESSAGES:
        case EXPORT_FORMAT_NONE:
          break;
      }
  }
}

/**
 * Write the preamble at the start of the file, allowing an initial
 * comment line and any format-specific lead-in.
 *
 * \param *instance   The export instance to operate on.
 */
void export_preamble(struct export_handle *instance)
{
  char buffer[EXPORT_MAX_LINE];

  if (instance == NULL)
    return;

  /* Output any preamble message. */

  MsgTrans_LookupPS(messages, "ExportPreamble", buffer, EXPORT_MAX_LINE, 0, 0, 0, instance->time);

  if (strlen(buffer) > 0) {
    export_start_line(instance);
    export_start_comment(instance);
    export_append_text(instance, buffer);
    export_end_comment(instance);
  }

  /* Do any format-specific initialisation. */

  switch (instance->format)
  {
    case EXPORT_FORMAT_BASIC:
      if (instance->ptr != EXPORT_PTR_UNSTARTED)
        export_start_line(instance);

      /* DEF PROCiconnumbers */

      export_start_line(instance);
      export_append_text(instance, "\xdd \xf2iconnumbers");
      break;
    case EXPORT_FORMAT_CENUM:
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_MESSAGES:
    case EXPORT_FORMAT_NONE:
      break;
  }
}

/**
 * Write the postamble at the end of the file, allowing any format-specific
 * lead-out and a final comment line.
 *
 * \param *instance   The export instance to operate on.
 */
void export_postamble(struct export_handle *instance)
{
  char buffer[EXPORT_MAX_LINE];

  if (instance == NULL)
    return;

  /* Do any format-specific finalisation. */

  switch (instance->format)
  {
    case EXPORT_FORMAT_BASIC:
      if (instance->ptr != EXPORT_PTR_UNSTARTED)
        export_start_line(instance);

      /* ENDPROC */

      export_start_line(instance);
      export_append_text(instance, "\xe1");
      break;
    case EXPORT_FORMAT_CENUM:
    case EXPORT_FORMAT_CDEFINE:
    case EXPORT_FORMAT_MESSAGES:
    case EXPORT_FORMAT_NONE:
      break;
  }

  /* Output any postamble message. */

  MsgTrans_LookupPS(messages, "ExportPostamble", buffer, EXPORT_MAX_LINE, 0, 0, 0, instance->time);

  if (strlen(buffer) > 0) {
    export_start_line(instance);
    export_start_comment(instance);
    export_append_text(instance, buffer);
    export_end_comment(instance);
  }
}
