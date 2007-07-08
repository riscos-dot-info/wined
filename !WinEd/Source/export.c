#include "DeskLib:MsgTrans.h"
#include "DeskLib:Validation.h"

#include "globals.h"
#include "export.h"

static  char buffer[256];

void export_puts(FILE *fp, const char *string)
{
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

static BOOL export_scan(FILE *fp, const browser_winentry *winentry, BOOL write, const char *pre)
{
  int icon;
  BOOL result = FALSE;
  char buffy[256];

  for (icon = 0; icon < winentry->window->window.numicons; ++icon)
  {
/*fprintf(fp, ">>> Checking icon %d\n", icon);*/
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

/*fprintf(fp, ">>> N: \"");*/
        for (n = valix; validstring[n] > 31 && validstring[n] != ';'; ++n)
          icnname[n-valix] = validstring[n];
        icnname[n-valix] = 0;
        if (icnname[0])
        {
/*fprintf(fp, "%s\"\n", icnname);*/
          result = TRUE;
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
            export_puts(fp, buffer);
          }
        }
/*else fprintf(fp, "\"\n");*/
      }
    }
  }
  return result;
}

void export_winentry(FILE *fp, const browser_winentry *winentry, const char *pre)
{
  char buffy[256];
/*fprintf(fp, ">>> Checking window \"%s\"\n", winentry->identifier);*/
  if (export_scan(fp, winentry, FALSE, pre))
  {

    snprintf(buffy, 256, "%sPreWin", pre);
    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, 0);
    export_puts(fp, buffer);
    export_scan(fp, winentry, TRUE, pre);
    snprintf(buffy, 256, "%sPostWin", pre);
    MsgTrans_LookupPS(messages, buffy, buffer, sizeof(buffer),
    	(char *) winentry->identifier, 0, 0, 0);
    export_puts(fp, buffer);
  }
}
