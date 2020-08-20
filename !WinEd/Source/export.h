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
enum export_format
{
  EXPORT_FORMAT_NONE,     /**< No format specified.                               */
  EXPORT_FORMAT_BASIC,    /**< BASIC variable assignments.                        */
  EXPORT_FORMAT_CENUM,    /**< C enum ... { ... }; format.                        */
  EXPORT_FORMAT_CDEFINE,  /**< C #define ... format.                              */
  EXPORT_FORMAT_CTYPEDEF, /**< C typedef enum { ... } ...; format.                */
  EXPORT_FORMAT_MESSAGES  /**< Empty MessageTrans tokens.                         */
};

/**
 * The available export options.
 */
enum export_flags
{
  EXPORT_FLAGS_NONE = 0x0000,         /**< No options explicitly specified.       */
  EXPORT_FLAGS_UPPERCASE = 0x0001,    /**< Convert all names to uppercase.        */
  EXPORT_FLAGS_LOWERCASE = 0x0002,    /**< Convert all names to lowercase.        */
  EXPORT_FLAGS_SKIPIMPLIED = 0x0004,  /**< Skip implied value assignments.        */
  EXPORT_FLAGS_USEREAL = 0x0008,      /**< Use real variable names in BASOC.      */
  EXPORT_FLAGS_PREFIX = 0x0010,       /**< Write the prefix on all names.         */
  EXPORT_FLAGS_WINNAME = 0x0020       /**< Include the window name in all names.  */
};

/**
 * An export instance handle.
 */
struct export_handle;

/**
 * Open a file for exporting icon names in a specified format.
 *
 * \param format      The format to use for export.
 * \param flags       The options to apply to the export.
 * \param *prefix     Pointer to a prefix string to include on names.
 * \param *filename   Pointer to the filename to open.
 * \return            Pointer to an export instance, or NULL on failure.
 */
struct export_handle *export_openfile(enum export_format format, enum export_flags flags, char *prefix, char *filename);

/**
 * Close the file associated with an export instance, and
 * tidy up.
 * 
 * \param *instance   The export instance to close.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL export_closefile(struct export_handle *instance);

/**
 * Write the preamble at the start of the file, allowing an initial
 * comment line and any format-specific lead-in.
 *
 * \param *instance   The export instance to operate on.
 */
void export_preamble(struct export_handle *instance);

/**
 * Write the postamble at the end of the file, allowing any format-specific
 * lead-out and a final comment line.
 *
 * \param *instance   The export instance to operate on.
 */
void export_postamble(struct export_handle *instance);

/**
 * Export the icon names from a winentry.
 * 
 * \param *instance   The export instance to operate on.
 * \param *winentry   The winentry to export.
 */
void export_winentry(struct export_handle *instance, browser_winentry *winentry);

#endif
