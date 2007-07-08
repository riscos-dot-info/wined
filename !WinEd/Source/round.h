/* round.h */

/* Rounds variables to nearest word */

#ifndef __wined_round_h
#define __wined_round_h

#include "DeskLib:Wimp.h"

#include "choices.h"

int round_down_int(int a);

int round_up_int(int a);

void round_down_point(wimp_point *point);

void round_up_point(wimp_point *point);

void round_down_box(wimp_rect *box);

void round_up_box(wimp_rect *box);

/* These three added for bodge round-by-two option */
void round_down_box_less(wimp_rect *box);
void round_down_point_less(wimp_point *point);
int round_down_int_less(int a);

#endif
