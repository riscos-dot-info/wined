#ifndef __wined_export_h
#define __wined_export_h
/* Exports icon names */

#ifndef __stdio_h
#include <stdio.h>
#endif

#ifndef __wined_tempdefs_h
#include "tempdefs.h"
#endif

void export_puts(FILE *fp, const char *string);

void export_winentry(FILE *fp, const browser_winentry *, const char *pre);

#endif
