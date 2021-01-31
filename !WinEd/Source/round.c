/* round.c */

#include "common.h"
#include "round.h"

int round_down_int(int a)
{
  if (choices->round_coords) a = a & ~3;
  return a;
}

int round_up_int(int a)
{
  if (choices->round_coords) a = (a + 3) & ~3;
  return a;
}

void round_down_point(wimp_point *point)
{
  point->x = round_down_int(point->x);
  point->y = round_down_int(point->y);
}

void round_up_point(wimp_point *point)
{
  point->x = round_up_int(point->x);
  point->y = round_up_int(point->y);
}

void round_down_box(wimp_rect *box)
{
  round_down_point(&box->min);
  round_down_point(&box->max);
}

void round_up_box(wimp_rect *box)
{
  round_up_point(&box->min);
  round_up_point(&box->max);
}

/* Added to bodge a round by only 2 rather than 4 */
void round_down_box_less(wimp_rect *box)
{
  round_down_point_less(&box->min);
  round_down_point_less(&box->max);
}

void round_down_point_less(wimp_point *point)
{
  point->x = round_down_int_less(point->x);
  point->y = round_down_int_less(point->y);
}

int round_down_int_less(int a)
{
  if ((choices->round_coords) && ((a%2) == 1)) a = a - 1;
  return a;
}
