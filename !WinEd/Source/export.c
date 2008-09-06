#include <string.h>
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Validation.h"
#include "DeskLib:Debug.h"
#include "DeskLib:Time.h"

#include "globals.h"
#include "export.h"

static  char buffer[256];

void export_puts(FILE *fp, const char *string)
{

  Log(log_DEBUG, "export_puts - Writing to text file, string: '%s'", string);

  for (;;++string)
  {
    switch (*string)
    {
      case '\\': /* E.g. it's a single slash character */
        switch (*++string) /* Check next character */
        {
          case 'n':
            putc('\n', fp);
            break;
          case 't':
            putc('\t', fp);
            break;
          default:
            putc(*string, fp);
            break;
        }
        break;
      default:
        if (*string < 32)
          return;
        putc(*string, fp);
        break;
    }
  }
}

void export_puts_basic(FILE *fp, char *string, int *lineno)
{
  char *newline;
  char *previous;

  previous = string;

  if (strlen(string) > 0 ) /* Don't write blank lines */
  {
    if (strcmp(string, "\\n") == 0)
      *previous = '\0'; /* Special case - allow a lonely "\n" to mean "print one blank line" */

    do
    {
      newline = strstr(previous, "\\n");                /* Find newline character */
      if (newline)                                      /* Insert terminator if there's another section to write out */
        *newline = '\0';
        export_puts_basic_line(fp, previous, *lineno);  /* Write first (and possibly last) section */
      (*lineno)++;                                      /* Increment output line number */
      previous = newline + 2;                           /* Skip past the actual "\n" */
    } while (newline);
  }
}

void export_puts_basic_line(FILE *fp, const char *string, int lineno)
{
  char *ptr;

  Log(log_DEBUG, "export_puts_basic_line - Writing to basic file (line %d), string: '%s'", lineno, string);

  ptr = (char *)&lineno;

  putc(13, fp);                 /* Start with a CR */
  putc(ptr[1], fp);             /* High byte of lineno */
  putc(ptr[0], fp);             /* Low byte of lineno */
  putc(strlen(string) + 4, fp); /* Length of string + line header */

  for (;;++string)
  {
    if (*string < 32)
      return;
    putc(*string, fp);
  }
}
/*
Use this if want to export upper case names...
char              *upper_case(char *s)
{
//  Log(log_DEBUG, "lower_case");

  char *ret = s;
  if (s)
  {
    while (*s)
    {
      *s = toupper(*s);
      s++;
    }
  }
  return ret;
}
*/

int export_scan(FILE *fp, const browser_winentry *winentry, BOOL write, const char *pre, int currentline)
{
  int icon, foundanicon = FALSE;
  char buffy[256];

  Log(log_DEBUG, "export_scan, write:%d", write);

  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
    Log(log_DEBUG, "Checking icon %d", icon);
    if (winentry->window->icon[icon].flags.data.indirected &&
    	winentry->window->icon[icon].flags.data.text &&
    	(int) winentry->window->icon[icon].data.indirecttext.validstring >= 0)
    {
      int valix;
      char *validstring =
      	winentry->window->icon[icon].data.indirecttext.validstring +
      	(int) winentry->window;

      valix = Validation_ScanString(validstring, 'N');
      if (valix)
      {
        char icnname[128];
        int n = 0;

        for (n = valix; (validstring[n] > 31) && (validstring[n] != ';') && (n-valix<sizeof(icnname)-1); ++n)
          icnname[n-valix] = validstring[n];
        icnname[n-valix] = 0;
        if (icnname[0])
        {
          Log(log_DEBUG, "Icon name: '%s'", icnname);
          if (write)
          {
            char icnnum[8];
            char *tag;
            unsigned char ro_time[5];
            char thetime[256];

            sprintf(icnnum, "%d", icon);
            if (winentry->window->window.numicons == 1)
              tag = "OneIcon";
            else if (icon == winentry->window->window.numicons-1)
              tag = "LastIcon";
            else if (icon == 0)
              tag = "1stIcon";
            else
              tag = "Icon";

            snprintf(buffy, 256, "%s%s", pre, tag);

            Time_ReadClock(ro_time);
            Time_ConvertDateAndTime(ro_time, thetime, sizeof(thetime), "%ce%yr/%mn/%dy, %24:%mi");

            MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
            	(char *) winentry->identifier, icnname, icnnum, thetime);
            if (strcmp(pre, "B_"))
              export_puts(fp, buffer);
            else
              export_puts_basic(fp, buffer, &currentline);
          }
          foundanicon = TRUE;
        }
      }
    }
  }
  if (write)
    return currentline;
  else
    /* This is used in first scan through to avoid writing an entry at all if there are no icons names */
    return foundanicon;
}

int export_winentry(FILE *fp, const browser_winentry *winentry, const char *pre, int currentline)
{
  char buffy[256], thetime[256];
  unsigned char ro_time[5];

  Log(log_DEBUG, "export_winentry: %p", winentry->identifier);

  if (export_scan(fp, winentry, FALSE, pre, 1)) /* Here, currentline isn't used or updated and        */
  {                                             /* the return value is used as a boolean check rather */
    snprintf(buffy, 256, "%sPreWin", pre);      /* than as a line counter                             */

    Time_ReadClock(ro_time);
    Time_ConvertDateAndTime(ro_time, thetime, sizeof(thetime), "%ce%yr/%mn/%dy, %24:%mi");

    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, thetime);
    if (strcmp(pre, "B_"))
      export_puts(fp, buffer);
    else
      export_puts_basic(fp, buffer, &currentline);
    currentline = export_scan(fp, winentry, TRUE, pre, currentline);
    snprintf(buffy, 256, "%sPostWin", pre);

    Time_ReadClock(ro_time);
    Time_ConvertDateAndTime(ro_time, thetime, sizeof(thetime), "%ce%yr/%mn/%dy, %24:%mi");

    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, thetime);
    if (strcmp(pre, "B_"))
      export_puts(fp, buffer);
    else
      export_puts_basic(fp, buffer, &currentline);
  }

  return currentline;
}
