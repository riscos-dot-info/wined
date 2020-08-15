/* diagutils.h */

/* Handles various types of common icons in dialogue boxes */

#ifndef __wined_diagutils_h
#define __wined_diagutils_h

#include "DeskLib:Wimp.h"

/* Register handlers for a pair of 'bumper' arrows next to a numerical field;
   Bumper icons must immediately follow display field numerically */
void diagutils_bumpers(window_handle window,icon_handle field,
     		       int min,int max /* Limits */,int step);

/* Register handlers for a set of colour selection icons in the order:
   display field, dec, inc, menu. 'none' switch decides whether to allow
   None (transparent) */
void diagutils_colour(window_handle window,icon_handle field,BOOL none);

/* Handlers for choosing button type */
void diagutils_btype(window_handle window,icon_handle field);

/* Load Colour dbox and create menus */
void diagutils_init(void);

/* Write a button-type value */
void diagutils_writebtype(window_handle window,icon_handle field,int value);

/* Read a button-type value */
int diagutils_readbtype(window_handle window,icon_handle field);

/* Read/Write a colour field */
int diagutils_readcolour(window_handle window,icon_handle field);
void diagutils_writecolour(window_handle window,icon_handle field,int value);

/**
 * General purpose event handler to check the state of radio icons
 * after an Adjust click and ensure that they have not been deselected.
 * 
 * \param *event      The event poll block.
 * \param *reference  Unused.
 * \returns           FALSE, as this never claims the event.
 */
BOOL diagutils_adjust_radio(event_pollblock *event, void *reference);

#endif
