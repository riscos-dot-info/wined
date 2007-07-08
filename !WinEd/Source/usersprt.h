/* usersprt.h */
/* Controls WinEd's user sprite area */

#ifndef __wined_usersprt_h
#define __wined_usersprt_h

#include "DeskLib:Sprite.h"

/* Add 22 to a spritefile name if in hi-res mode; buffer contains
   raw string on entry and must be large enough for additional "22"
*/
void usersprt_modfilename(char *name);

/* WinEd's sprite area */
extern sprite_area user_sprites;

/* Initialise sprite area */
void usersprt_init(void);

/* Merge a sprite file in to sprite area */
BOOL usersprt_merge(char *filename,int size,void *reference);

/* Clear all sprites from user sprite area */
void usersprt_clear(void);

#endif
