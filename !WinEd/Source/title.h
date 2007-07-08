/* title.h */

/* Set a browser's title; actual function is in browser.c but is needed
   by functions below browser's level */

#ifndef __wined_title_h
#define __wined_title_h

/* Set title of window; if title is NULL use existing title */
void browser_settitle(browser_fileinfo *browser,char *title,BOOL altered);

#endif
