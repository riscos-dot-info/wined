/* IniConfig.h */

/**
 * Handle the loading and saving of choices structures in a textual
 * INI file format.
 * 
 * This code has been structured in its own file, with a view to
 * adding it to DeskLib as a stand-alone module.
 * 
 * (c) Stephen Fryatt, 2021.
 */

#ifndef __wined_iniconfig_h
#define __wined_iniconfig_h

#include <stdio.h>
#include "DeskLib:Core.h"

/**
 * An IniConfig file instance.
 */
typedef struct iniconfig_file iniconfig_file;

/**
 * Construct a new IniConfig file instance.
 * 
 * \return            Pointer to a new instance, or NULL.
 */
iniconfig_file *IniConfig_Init(void);

/**
 * Add a section to an IniConfig file.
 *
 * \param *file       Pointer to the IniFile instance to update.
 * \param *name       Pointer to the name of the section.
 */
void IniConfig_AddSection(iniconfig_file *file, const char *name);

/**
 * Add a boolean value to an IniConfig file.
 *
 * \param *file       Pointer to the IniFile instance to update.
 * \param *name       Pointer to the name of the value.
 * \param *variable   Pointer to the associated variable.
 * \param initial     The initial value to set the variable to.
 */
void IniConfig_AddBoolean(iniconfig_file *file, const char *name, BOOL *variable, BOOL initial);

/**
 * Reset the variables in an IniConfig file to their default
 * values.
 * 
 * \param *file       Pointer to the IniFile instance to update.
 */
void IniConfig_ResetDefaults(iniconfig_file *file);

/**
 * Write an IniConfig file to disc, storing the current values
 * of the choices variables within it.
 *
 * \param *file       Pointer to the IniFile instance to write.
 * \param *filename   Pointer to the filename to write to.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_WriteFile(iniconfig_file *file, char *filename);

/**
 * Read an IniConfig file from disc and update the current
 * values of the choices variables according to the data within
 * it.
 *
 * \param *file       Pointer to the IniFile instance to read.
 * \param *filename   Pointer to the filename to read from.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_ReadFile(iniconfig_file *file, char *filename);

/**
 * Read an IniConfig file from disc and update the current
 * values of the choices variables according to the data within
 * it.
 *
 * \param *file       Pointer to the IniFile instance to read.
 * \param *in         Pointer to a C file handle to read from.
 * \return            TRUE if successful; FALSE on failure.
 */
BOOL IniConfig_ReadFileRaw(iniconfig_file *file, FILE *in);

#endif
