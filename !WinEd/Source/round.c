/* round.c */

#include "common.h"
#include "round.h"

/**
 * Round a value down to a multiple of a given step, if the
 * Round Coordinates option is selected. Otherwise, do nothing.
 *
 * \param step  The multiple to round to.
 * \param a     The value to be rounded.
 * \return      The rounded value.
 */
int round_down_int(round_step step, int a)
{
  if (choices->round_coords) {
    switch (step) {
      case round_STEP_FOUR:
        a = a & ~3;
        break;
      case round_STEP_TWO:
        a = a & ~1;
        break;
      case round_STEP_NONE:
        break;
    }
  }

  return a;
}

/**
 * Round a value up to a multiple of a given step, if the
 * Round Coordinates option is selected. Otherwise, do nothing.
 *
 * \param step  The multiple to round to.
 * \param a     The value to be rounded.
 * \return      The rounded value.
 */
int round_up_int(round_step step, int a)
{
  if (choices->round_coords) {
    switch (step) {
      case round_STEP_FOUR:
        a = (a + 3) & ~3;
        break;
      case round_STEP_TWO:
        a = (a + 1) & ~1;
        break;
      case round_STEP_NONE:
        break;
    }
  }

  return a;
}

/**
 * Round the coordinates of a point down to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param point The point to be rounded.
 * \return      The rounded point.
 */
void round_down_point(round_step step, wimp_point *point)
{
  point->x = round_down_int(step, point->x);
  point->y = round_down_int(step, point->y);
}

/**
 * Round the coordinates of a point up to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param point The point to be rounded.
 * \return      The rounded point.
 */
void round_up_point(round_step step, wimp_point *point)
{
  point->x = round_up_int(step, point->x);
  point->y = round_up_int(step, point->y);
}

/**
 * Round the coordinates of a box down to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param box   The box to be rounded.
 * \return      The rounded box.
 */
void round_down_box(round_step step, wimp_rect *box)
{
  round_down_point(step, &box->min);
  round_down_point(step, &box->max);
}

/**
 * Round the coordinates of a box up to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param box   The box to be rounded.
 * \return      The rounded box.
 */
void round_up_box(round_step step, wimp_rect *box)
{
  round_up_point(step, &box->min);
  round_up_point(step, &box->max);
}
