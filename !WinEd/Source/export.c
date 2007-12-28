#include <string.h>
#include "DeskLib:MsgTrans.h"
#include "DeskLib:Validation.h"
#include "DeskLib:Debug.h"

#include "globals.h"
#include "export.h"

static  char buffer[256];

void export_puts(FILE *fp, const char *string)
{

  Debug_Printf("Writing to text file, string: '%s'", string);

  for (;;++string)
  {
    switch (*string)
    {
      case '\\':
        switch (*++string)
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

void export_puts_basic(FILE *fp, const char *string, int lineno)
{
  char *ptr;

  Debug_Printf("Writing to basic file (line %d), string: '%s'", lineno, string);

  ptr = (char *)&lineno;

  putc(13, fp);               /* Start with a CR */
  putc(ptr[1], fp);           /* High byte of lineno */
  putc(ptr[0], fp);           /* Low byte of lineno */
  putc(strlen(string)+4, fp); /* Length of string + line header */

  for (;;++string)
  {
    switch (*string)
    {
      case '\\':
        switch (*++string)
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

int export_scan(FILE *fp, const browser_winentry *winentry, BOOL write, const char *pre, int currentline)
{
  int icon; /* Note, first line is taken by PreWin token */
  char buffy[256];

  Debug_Printf("export_scan");

  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
    Debug_Printf("Checking icon %d", icon);
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

        for (n = valix; validstring[n] > 31 && validstring[n] != ';'; ++n)
          icnname[n-valix] = validstring[n];
        icnname[n-valix] = 0;
        if (icnname[0])
        {
          Debug_Printf("Icon name: '%s'", icnname);
          if (write)
          {
            char icnnum[8];
            char *tag;

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
            MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
            	(char *) winentry->identifier, icnname, icnnum, 0);
            if (strcmp(pre, "B_"))
              export_puts(fp, buffer);
            else
              export_puts_basic(fp, buffer, currentline++);
          }
        }
        else
        {
          Debug_Printf("Resetting linneno");
          if (!write) currentline = 0; /* This is used by export_winentry where write==FALSE */
        }
      }
    }
  }
  return currentline;
}

int export_winentry(FILE *fp, const browser_winentry *winentry, const char *pre, int currentline)
{
  char buffy[256];

  Debug_Printf("export_winentry: %s", winentry->identifier);

  if (export_scan(fp, winentry, FALSE, pre, 1)) /* Here, currentline isn't used or updated and        */
  {                                             /* the return value is used as a boolean check rather */
    snprintf(buffy, 256, "%sPreWin", pre);      /* than as a line counter                             */
    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, 0);
    if (strcmp(pre, "B_"))
      export_puts(fp, buffer);
    else
      export_puts_basic(fp, buffer, currentline++);
    currentline = export_scan(fp, winentry, TRUE, pre, currentline);
    snprintf(buffy, 256, "%sPostWin", pre);
    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, 0);
    if (strcmp(pre, "B_"))
      export_puts(fp, buffer);
    else
      export_puts_basic(fp, buffer, currentline++);
  }

  return currentline;
}
