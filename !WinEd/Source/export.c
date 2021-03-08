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
 * The maximum length of an exported line.
 */
#define EXPORT_MAX_LINE 256

/**
 * The line pointer for a line which hasn't been started.
 */
#define EXPORT_PTR_UNSTARTED (-1)

/**
 * Substitutions enabling flags.
 */

enum export_subs
{
  EXPORT_SUBS_NONE = 0x00,
  EXPORT_SUBS_PREFIX = 0x01,
  EXPORT_SUBS_TIME = 0x02,
  EXPORT_SUBS_VARSUFFIX = 0x04,
  EXPORT_SUBS_WINNAME = 0x08,
  EXPORT_SUBS_ICONNAME = 0x10,
  EXPORT_SUBS_ICONHANDLE = 0x20,
  EXPORT_SUBS_COMMENT = 0x40
};

/**
 * A structure holding details of a file export.
 */
struct export_handle
{
  enum export_format format;          /**< The format to export in.                                  */
  enum export_flags flags;            /**< The options to apply to the export.                       */
  enum export_subs substitutions;     /**< The substitution enabling flags.                          */
  char filename[EXPORT_MAX_FILENAME]; /**< The full name of the export file.                         */
  FILE *fp;                           /**< The handle of the export file.                            */
  char time[EXPORT_MAX_FIELD];        /**< The time of the export.                                   */
  char prefix[EXPORT_MAX_FIELD];      /**< The prefix to use for entries.                            */
  char varsuffix[EXPORT_MAX_FIELD];   /**< The suffix to use for BASIC variables.                    */
  char winname[wimp_MAXNAME];         /**< The processed window name.                                */
  char iconname[EXPORT_MAX_FIELD];    /**< The processed icon name.                                  */
  char iconhandle[EXPORT_MAX_FIELD];  /**< The processed icon handle.                                */
  char line[EXPORT_MAX_LINE];         /**< The buffer holding the current line.                      */
  int ptr;                            /**< A pointer to the next free character in the current line. */
  int linenumber;                     /**< The current line number.                                  */
};

/**
 * Write the current line out to the file, in the appropriate
 * format and reset the line pointer ready for a new line. If the
 * line is split by \0 characters, each section becomes its own
 * line with a valid start and end.
 * 
 * \param *instance   The export instance to process.
 */
static void export_write_line(struct export_handle *instance)
{
  int length, ptr = 0;

  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED)
    return;

  do
  {
    length = strlen(instance->line + ptr);

    if (ptr + length <= instance->ptr)
    {
      /* Write any pre-line formatting. */

      switch (instance->format)
      {
        case EXPORT_FORMAT_BASIC:
          fputc(0x0d, instance->fp);
          fputc((instance->linenumber >> 8) & 0xff, instance->fp);
          fputc(instance->linenumber & 0xff, instance->fp);
          fputc(length + 4, instance->fp);
          break;
        case EXPORT_FORMAT_CDEFINE:
        case EXPORT_FORMAT_CENUM:
        case EXPORT_FORMAT_CTYPEDEF:
        case EXPORT_FORMAT_MESSAGES:
        case EXPORT_FORMAT_NONE:
          break;
      }

      /* Write the line itself. */

      fputs(instance->line + ptr, instance->fp);

      /* Write any post-line formatting. */

      switch (instance->format)
      {
        case EXPORT_FORMAT_CDEFINE:
        case EXPORT_FORMAT_CENUM:
        case EXPORT_FORMAT_CTYPEDEF:
        case EXPORT_FORMAT_MESSAGES:
          fputc('\n', instance->fp);
          break;
        case EXPORT_FORMAT_BASIC:
        case EXPORT_FORMAT_NONE:
          break;
      }
    }

    ptr += length + 1;
  } while (ptr <= instance->ptr);

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
    case EXPORT_FORMAT_CTYPEDEF:
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
 * Add a continuation of comment sequence to the currently buffered line
 * of output, including a trailing space for clarity.
 * 
 * \param *instance   The export instance to process.
 */
static void export_continue_comment(struct export_handle *instance)
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
    case EXPORT_FORMAT_CTYPEDEF:
      export_append_text(instance, " * ");
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
    case EXPORT_FORMAT_CTYPEDEF:
      export_append_text(instance, " */");
      break;
   case EXPORT_FORMAT_MESSAGES:
   case EXPORT_FORMAT_BASIC:
   case EXPORT_FLAGS_NONE:
      break;
  }
}

/**
 * Decode a digit of a hex number.
 * 
 * \param in    A character to decode into a value from 0 to 15.
 * \param *out  Pointer to a variable to which to add the value.
 * \return      TRUE if the character was valid hex; otherwise FALSE.
 */
static BOOL export_decode_hex(char in, char *out)
{
  if (out == NULL)
    return FALSE;

  Log(log_INFORMATION, "Processing %c into %d", in, *out);

  if (in >= '0' && in <= '9')
    *out += (in - '0');
  else if (in >= 'A' && in <= 'F')
    *out += (in - 'A' + 10);
  else if (in >= 'a' && in <= 'f')
    *out += (in - 'a' + 10);
  else
    return FALSE;

  Log(log_INFORMATION, "Processed %c into %d", in, *out);

  return TRUE;
}

/**
 * Process an escape sequence in a template.
 *
 * \param *instance   The export instance to process.
 * \param **escape    Pointer to the variable holding the pointer to
 *                    the first character of the sequence. Updated
 *                    on exit to point to the next unprocessed character
 *                    in the template.
 */
static void export_process_escape_char(struct export_handle *instance, char **escape)
{
  char c = '\0';

  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED || escape == NULL)
    return;

  /* Step past the \ character. */

  (*escape)++;

  /* Process the escaped character and any more codes that we need,
   * leaving the pointer on the first character we don't understand.
   */

  switch (**escape)
  {
    case 'n': /* A line break. */
      instance->line[instance->ptr++] = '\0';
      (*escape)++;

      if (instance->substitutions & EXPORT_SUBS_COMMENT)
        export_continue_comment(instance);
      break;
    case 't': /* A tab character. */
      instance->line[instance->ptr++] = '\t';
      (*escape)++;
      break;
    case 'x': /* A hex sequence (eg. \xff). */
      (*escape)++;
      if (!export_decode_hex(*(*escape)++, &c))
        break;
      c *= 16;
      if (!export_decode_hex(*(*escape)++, &c))
        break;
      instance->line[instance->ptr++] = c;
      break;
    case '%': /* Literal characters. */
    case '[':
    case ']':
    case '@':
    case '\\':
      instance->line[instance->ptr++] = *(*escape)++;
      break;
    default: /* Codes we don't recognise. */
      (*escape)++;
      break;
  } 

  return;
}

/**
 * Attempt to expand an @ parameter in the template.
 * 
 * \param *instance     The export instance to process.
 * \param **parameter   Pointer to the variable holding the pointer to
 *                      the first character of the parameter. Updated
 *                      on exit to point to the next unprocessed character
 *                      in the template.
 * \param substitutions A bitmask to check against the configured substitutions
 *                      in the export instance. If one of the bits isn't set in
 *                      the instance, the substitution will be skipped.
 * \param *value        Pointer to the value to be substituted.
 * \return              TRUE if a substitution was made; otherwise FALSE.
 */
static BOOL export_try_substitution(struct export_handle *instance, char **parameter, char *code, enum export_subs substitutions, char *value)
{
  char *ptr;

  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED || parameter == NULL || code == NULL)
    return FALSE;

  Log(log_INFORMATION, "Testing %s for %s (subs 0x%x in 0x%x)", code, value, substitutions, instance->substitutions);

  /* Test the characters following the @, to see if they match the code. */

  ptr = *parameter;

  while (*ptr != '\0' && *code != '\0' && *code == *ptr)
  {
    code++;
    ptr++;
  }

  /* If the code didn't finish, it wasn't a match. */

  if (*code != '\0')
    return FALSE;

  /* Step the template past the matched code. */

  *parameter = ptr;

  /* If the substitution is disabled, give up. */

  if (!(instance->substitutions & substitutions) || value == NULL)
    return FALSE;

  /* Put out the substitution and return success. */

  export_append_text(instance, value);

  return TRUE;
}

/**
 * Process the expansion of a @ parameter in the template.
 * 
 * \param *instance   The export instance to process.
 * \param **parameter Pointer to the variable holding the pointer to
 *                    the first character of the parameter. Updated
 *                    on exit to point to the next unprocessed character
 *                    in the template.
 * \return            TRUE if a substitution was made; otherwise FALSE.
 */
static BOOL export_process_substitution(struct export_handle *instance, char **parameter)
{
  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED || parameter == NULL)
    return FALSE;

  /* Step past the @ character which got us here.. */

  (*parameter)++;

  /* Attempt to substitute the prefix text. */

  if (export_try_substitution(instance, parameter, "pf", EXPORT_SUBS_PREFIX, instance->prefix))
    return TRUE;

  /* Attempt to substitute the date and time. */

  if (export_try_substitution(instance, parameter, "dt", EXPORT_SUBS_TIME, instance->time))
    return TRUE;

  /* Attempt to substitute the BASIC integer variable suffix. */

  if (export_try_substitution(instance, parameter, "vs", EXPORT_SUBS_VARSUFFIX, instance->varsuffix))
    return TRUE;

  /* Attempt to substitute the window name. */

  if (export_try_substitution(instance, parameter, "wn", EXPORT_SUBS_WINNAME, instance->winname))
    return TRUE;

  /* Attempt to substitute the icon name. */

  if (export_try_substitution(instance, parameter, "in", EXPORT_SUBS_ICONNAME, instance->iconname))
    return TRUE;

  /* Attempt to substitute the icon handle. */

  if (export_try_substitution(instance, parameter, "ih", EXPORT_SUBS_ICONHANDLE, instance->iconhandle))
    return TRUE;

  /* We didn't find a match. */

  return FALSE;
}

/**
 * Process a block (the text found [ between ] square brackets) of a
 * format string, recursively processing any further blocks which are
 * encountered in the way. Unless forced to be included, blocks will
 * be skipped (including any sub-blocks within them) unless an @ parameter
 * is expanded within them
 *
 * \param *instance   The export instance to process.
 * \param **template  Pointer to the variable pointing to the template string,
 *                    which is updated on exit to point to the next character
 *                    to be processed.
 * \param include     TRUE on entry to force the section to be unconditionally
 *                    included.
 */
static void export_append_parser_block(struct export_handle *instance, char **template, BOOL include)
{
  int start;

  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED || template == NULL)
    return;

  /* Record the start of the output buffer, so that we can roll back
   * if the decision is taken to not include the block. */

  start = instance->ptr;

  /* Process the characters in the template one at a time. */

  while (**template != '\0' && instance->ptr < EXPORT_MAX_LINE)
  {
    switch (**template)
    {
      case '\\': /* Escape the following characters. */
        export_process_escape_char(instance, template);
        break;

      case '@': /* Expand an @ parameter with its value. */
        if (export_process_substitution(instance, template))
          include = TRUE;
        break;

      case '[': /* Drop into a sub-block. */
        (*template)++;
        export_append_parser_block(instance, template, FALSE);
        break;

      case ']': /* Return out of this sub block. */
        (*template)++;
        if (!include)
          instance->ptr = start;
        return;

      default: /* Just process the character. */
        instance->line[instance->ptr++] = *(*template)++;
        break;
    }
  }
}

/**
 * Append data to the current line using the template string parser.
 * 
 * \param *instance   The export instance to process.
 * \param *template   The template string to use.
 */
static void export_append_parser(struct export_handle *instance, char *template)
{
  if (instance == NULL || instance->ptr == EXPORT_PTR_UNSTARTED || template == NULL)
    return;

  export_append_parser_block(instance, &template, TRUE);

  if (instance->ptr < EXPORT_MAX_LINE)
    instance->line[instance->ptr] = '\0';

  /* Force termination for a buffer overrun. */

  instance->line[EXPORT_MAX_LINE - 1] = '\0';
}

/**
 * Write output based on one of the format-agnostic tokens in
 * the messages file, packaging it as a comment in the target format.
 * 
 * \param *instance   The export instance to process.
 * \param *token      Pointer to the specific bit of the token name.
 */
static void export_append_parser_token_comment(struct export_handle *instance, char *token)
{
  char fulltoken[EXPORT_MAX_FIELD], buffer[EXPORT_MAX_LINE];
  int p;

  if (instance == NULL || token == NULL)
    return;

  snprintf(fulltoken, EXPORT_MAX_FIELD, "Export%s", token);
  fulltoken[EXPORT_MAX_FIELD - 1] = '\0';
  buffer[0] = '\0';

  MsgTrans_LookupPS(messages, fulltoken, buffer, EXPORT_MAX_LINE, 0, 0, 0, 0);

  if (strlen(buffer) > 0)
  {
    instance->substitutions |= EXPORT_SUBS_COMMENT;
    export_start_line(instance);
    export_start_comment(instance);
    export_append_parser(instance, buffer);

    /* Multiline comments in C have their closing tag
     * on the following line.
     */

    switch (instance->format)
    {
      case EXPORT_FORMAT_CDEFINE:
      case EXPORT_FORMAT_CENUM:
      case EXPORT_FORMAT_CTYPEDEF:
        for (p = 0; p < instance->ptr; p++)
        {
          if (instance->line[p] == '\0')
          {
            if (instance->ptr < (EXPORT_MAX_LINE - 1))
              instance->line[++instance->ptr] = '\0';

            break;
          } 
        }
        break;
      
      case EXPORT_FORMAT_BASIC:
      case EXPORT_FORMAT_MESSAGES:
      case EXPORT_FORMAT_NONE:
        break;
    }

    export_end_comment(instance);
    instance->substitutions &= ~EXPORT_SUBS_COMMENT;
  }
}

/**
 * Write output based on one of the format-specific tokens in
 * the messages file.
 * 
 * \param *instance   The export instance to process.
 * \param *token      Pointer to the specific bit of the token name.
 * \param newline     Should a new line be started if the token exists.
 */
static void export_append_parser_token(struct export_handle *instance, char *token, BOOL newline)
{
  char fulltoken[EXPORT_MAX_FIELD], buffer[EXPORT_MAX_LINE];
  char *section = NULL;

  if (instance == NULL || token == NULL)
    return;

  switch (instance->format)
  {
    case EXPORT_FORMAT_CENUM:
      section = "Enum";
      break;
    case EXPORT_FORMAT_CTYPEDEF:
      section = "Typedef";
      break;
    case EXPORT_FORMAT_CDEFINE:
      section = "Define";
      break;
    case EXPORT_FORMAT_BASIC:
      section = "Basic";
      break;
    case EXPORT_FORMAT_MESSAGES:
      section = "Message";
      break;
    case EXPORT_FORMAT_NONE:
      break;
  }

  if (section == NULL)
    return;

  snprintf(fulltoken, EXPORT_MAX_FIELD, "Export%s%s", section, token);
  fulltoken[EXPORT_MAX_FIELD - 1] = '\0';
  buffer[0] = '\0';

  MsgTrans_LookupPS(messages, fulltoken, buffer, EXPORT_MAX_LINE, 0, 0, 0, 0);

  if (strlen(buffer) > 0)
  {
    if (newline)
      export_start_line(instance);
    export_append_parser(instance, buffer);
  }
}

/**
 * Open a file for exporting icon names in a specified format.
 *
 * \param format      The format to use for export.
 * \param flags       The options to apply to the export.
 * \param *prefix     Pointer to a prefix string to include on names.
 * \param *filename   Pointer to the filename to open.
 * \return            Pointer to an export instance, or NULL on failure.
 */
struct export_handle *export_openfile(enum export_format format, enum export_flags flags, char *prefix, char *filename)
{
  struct export_handle *new = NULL;
  unsigned char ro_time[5];

  if (filename == NULL || prefix == NULL)
    return NULL;

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

  strncpycr(new->prefix, prefix, EXPORT_MAX_FIELD);
  new->prefix[EXPORT_MAX_FIELD - 1] = '\0';

  strncpy(new->varsuffix, "%", EXPORT_MAX_FIELD);
  new->varsuffix[EXPORT_MAX_FIELD - 1] = '\0';

  new->winname[0] = '\0';
  new->iconname[0] = '\0';
  new->iconhandle[0] = '\0';

  /* Set up the substitution flags. */

  new->substitutions = EXPORT_SUBS_TIME | EXPORT_SUBS_WINNAME;

  if (flags & EXPORT_FLAGS_PREFIX)
    new->substitutions |= EXPORT_SUBS_PREFIX;

  if (!(flags & EXPORT_FLAGS_USEREAL))
    new->substitutions |= EXPORT_SUBS_VARSUFFIX;

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
    case EXPORT_FORMAT_CTYPEDEF:
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
 * \param *instance   The export instance to operate on.
 * \param c           The character to process.
 * \param first       TRUE if this is the first character in an identifier.
 * \return            The updated character.
 */
static char export_substitute_character(struct export_handle *instance, char c, BOOL first)
{
  if (instance == NULL)
    return '_';

  /* Message files are very different, so special-case them now. */

  if (instance->format == EXPORT_FORMAT_MESSAGES)
  {
    if (c <= ' ' || c == ',' || c == ')' || c == ':' || c == '?' || c == ':')
      return '_';

    return c;
  }

  /* BASIC variables can use a backtick. */

  if (instance->format == EXPORT_FORMAT_BASIC && c == '`')
    return c;

  /* Both C and BASIC can use alphabetci characters anywhere. */

  if (strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c) != NULL)
    return c;

  /* Both C and BASIC can use digits after the first character. */

  if (!first && strchr("0123456789", c) != NULL)
    return c;

  /* Anything else, map to underscore. */

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
  BOOL first = TRUE;

  if (instance == NULL || buffer == NULL)
    return;

  /* There's no need to special-case the first character if we're prefixing
   * all names with an underscore anyway. */

  if (instance->flags && EXPORT_FLAGS_PREFIX)
    first = FALSE;

  while (*buffer != '\0')
  {
    *buffer = export_substitute_character(instance, *buffer, first);
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
  int icon, last_icon = -1, namelen;
  BOOL foundicons = FALSE;

  Log(log_INFORMATION, "Exporting window %s", winentry->identifier);

  if (instance == NULL || winentry == NULL)
    return;

  /* Copy the window name and sanitise it for the target language. */

  strncpycr(instance->winname, winentry->identifier, wimp_MAXNAME);
  instance->winname[wimp_MAXNAME - 1] = '\0';
  export_process_lang_name(instance, instance->winname);

  /* Process all of the icons in the window. */

  instance->substitutions |= (EXPORT_SUBS_ICONNAME | EXPORT_SUBS_ICONHANDLE);

  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
    namelen = extract_iconname(winentry, icon, instance->iconname, EXPORT_MAX_FIELD);
    if (namelen == 0)
      continue;

    snprintf(instance->iconhandle, EXPORT_MAX_FIELD, "%d", icon);
    instance->iconhandle[EXPORT_MAX_FIELD - 1] = '\0';

    /* Sanitise the icon name for the target language. */

    export_process_lang_name(instance, instance->iconname);

    /* Clear any unfinished business from the previous line before
     * on this icon. */

    if (foundicons == FALSE)
    {
      /* This is the first icon in the window, so write the window title comment. */

      if (instance->ptr != EXPORT_PTR_UNSTARTED)
        export_start_line(instance);

      export_append_parser_token_comment(instance, "Window");

      /* Perform any window initialisation for the target format. */

      export_append_parser_token(instance, "Start", TRUE);

      if (!(instance->flags & EXPORT_FLAGS_WINNAME))
        instance->substitutions &= ~EXPORT_SUBS_WINNAME;
    }
    else
    {
      /* This is a follow-up icon, so perform any finishing touches to the previous line. */

      export_append_parser_token(instance, "Sep", FALSE);
    }

    foundicons = TRUE;

    /* Start the new line. */

    export_start_line(instance);

    /* Output the icon details. */

    if ((instance->flags & EXPORT_FLAGS_SKIPIMPLIED) && (last_icon == (icon - 1)))
      instance->substitutions &= ~EXPORT_SUBS_ICONHANDLE;

    export_append_parser_token(instance, "Icon", FALSE);

    instance->substitutions |= EXPORT_SUBS_ICONHANDLE;
    last_icon = icon;
  }

  /* If any icons were found, write the tail of the window data */

  instance->substitutions |= EXPORT_SUBS_WINNAME;
  instance->substitutions &= ~(EXPORT_SUBS_ICONNAME | EXPORT_SUBS_ICONHANDLE);

  if (foundicons)
    export_append_parser_token(instance, "End", TRUE);
}

/**
 * Write the preamble at the start of the file, allowing an initial
 * comment line and any format-specific lead-in.
 *
 * \param *instance   The export instance to operate on.
 */
void export_preamble(struct export_handle *instance)
{
  if (instance == NULL)
    return;

  /* Output any preamble message. */

  export_append_parser_token_comment(instance, "Preamble");

  /* Do any format-specific initialisation. */

  export_append_parser_token(instance, "Preamble", TRUE);
}

/**
 * Write the postamble at the end of the file, allowing any format-specific
 * lead-out and a final comment line.
 *
 * \param *instance   The export instance to operate on.
 */
void export_postamble(struct export_handle *instance)
{
  if (instance == NULL)
    return;

  /* Do any format-specific finalisation. */

  export_append_parser_token(instance, "Postamble", TRUE);

  /* Output any postamble message. */

  export_append_parser_token_comment(instance, "Postamble");
}
