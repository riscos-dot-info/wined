/* picker.h */

/* Icon picker window; outline fonts not allowed */

#ifndef __wined_picker_h
#define __wined_picker_h

#include "tempdefs.h"

/* dummy winentry for picker window */
extern browser_winentry picker_winentry;

/* Load picker definition */
void picker_init(void);

/* Open picker window */
void picker_open(void);

/* Close picker */
void picker_close(void);

/* Increase/decrease number of windows known to picker, so it can decide
   whether window should be open */
void picker_activeinc(void);
void picker_activedec(void);

#endif
