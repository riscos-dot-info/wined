/* InioConfig.h */

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

#include "DeskLib:Core.h"

/**
 * An IniConfig file instance.
 */

typedef struct iniconfig_file iniconfig_file;

iniconfig_file *IniConfig_Init(void);

void IniConfig_AddSection(iniconfig_file *file, const char *name);
void IniConfig_AddBoolean(iniconfig_file *file, const char *name, BOOL *variable, BOOL initial);


BOOL IniConfig_WriteFile(iniconfig_file *file, char *filename);
BOOL IniConfig_ReadFile(iniconfig_file *file, char *filename);

#endif
