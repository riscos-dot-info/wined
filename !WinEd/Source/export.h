#ifndef __wined_export_h
#define __wined_export_h
/* Exports icon names */

#ifndef __stdio_h
#include <stdio.h>
#endif

#ifndef __wined_tempdefs_h
#include "tempdefs.h"
#endif

/**
 * The available output formats.
 */
enum export_format {
  EXPORT_FORMAT_NONE,
  EXPORT_FORMAT_BASIC,
  EXPORT_FORMAT_CENUM,
  EXPORT_FORMAT_CDEFINE,
  EXPORT_FORMAT_MESSAGES
};

enum export_flags {
  EXPORT_FLAGS_NONE = 0,
  EXPORT_FLAGS_UPPERCASE = 1,
  EXPORT_FLAGS_LOWERCASE = 2,
  EXPORT_FLAGS_SKIPIMPLIED = 4,
  EXPORT_FLAGS_USEREAL = 8,
  EXPORT_FLAGS_PREFIX = 16,
  EXPORT_FLAGS_WINNAME = 32
};

/**
 * An export instance handle.
 */
struct export_handle;


struct export_handle *export_openfile(enum export_format format, enum export_flags flags, char *filename);
BOOL export_closefile(struct export_handle *instance);

void export_preamble(struct export_handle *instance);
void export_postamble(struct export_handle *instance);


void export_puts(FILE *fp, const char *string);
void export_puts_basic(FILE *fp, char *string, int *lineno);
void export_puts_basic_line(FILE *fp, const char *string, int lineno);
void export_winentry(struct export_handle *instance, browser_winentry *winentry);


#endif
