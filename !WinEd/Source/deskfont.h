/* deskfont.h */
/* Just creates Homerton.Medium 12pt for use with icnedit_minsize */

#ifndef __wined_deskfont_h
#define __wined_deskfont_h

#include "DeskLib:Font.h"

#define Message_FontChanged	0x400cf

extern font_handle deskfont_handle;
extern void deskfont_init(void);

#endif
