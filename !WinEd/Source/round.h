/* round.h */

/* Rounds variables to nearest required multiple. */

#ifndef __wined_round_h
#define __wined_round_h

#include "DeskLib:Wimp.h"

#include "choices.h"

/**
 * The step sizes which can be applied to rounding.
 */
typedef enum {
  round_STEP_NONE,  /**< Apply no rounding.                             */
  round_STEP_TWO,   /**< Round up or down to the nearest multiple of 2. */
  round_STEP_FOUR   /**< Round up or down to the nearest multiple of 4. */
} round_step;

/**
 * Round a value down to a multiple of a given step, if the
 * Round Coordinates option is selected. Otherwise, do nothing.
 *
 * \param step  The multiple to round to.
 * \param a     The value to be rounded.
 * \return      The rounded value.
 */
int round_down_int(round_step step, int a);

/**
 * Round a value up to a multiple of a given step, if the
 * Round Coordinates option is selected. Otherwise, do nothing.
 *
 * \param step  The multiple to round to.
 * \param a     The value to be rounded.
 * \return      The rounded value.
 */
int round_up_int(round_step step, int a);

/**
 * Round the coordinates of a point down to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param point The point to be rounded.
 * \return      The rounded point.
 */
void round_down_point(round_step step, wimp_point *point);

/**
 * Round the coordinates of a point up to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param point The point to be rounded.
 * \return      The rounded point.
 */
void round_up_point(round_step step, wimp_point *point);

/**
 * Round the coordinates of a box down to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param box   The box to be rounded.
 * \return      The rounded box.
 */
void round_down_box(round_step step, wimp_rect *box);

/**
 * Round the coordinates of a box up to a multiple of a given
 * step, if the Round Coordinates option is selected. Otherwise,
 * do nothing.
 *
 * \param step  The multiple to round to.
 * \param box   The box to be rounded.
 * \return      The rounded box.
 */
void round_up_box(round_step step, wimp_rect *box);

#endif
