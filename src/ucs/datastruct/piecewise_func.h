/*
 * Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCS_PIECEWISE_FUNC_H_
#define UCS_PIECEWISE_FUNC_H_

#include <ucs/datastruct/linear_func.h>
#include <ucs/debug/assert.h>
#include <stddef.h>
#include <limits.h>

#define UCX_PIECEWISE_FUNC_MAX_SEGMENTS 128

/* The zero function */
#define UCS_PIECEWISE_FUNC_ZERO ucs_piecewise_func_make(0, 0)


struct ucs_piecewise_segment;
typedef struct ucs_piecewise_segment ucs_piecewise_segment_t;
/*
 * A piecewise func segment which represents linear function on a half-open
 * interval.
 */
struct ucs_piecewise_segment {
    ucs_linear_func_t       func;
    size_t                  end;
    ucs_piecewise_segment_t *next;
};


/**
 * A piecewise function consisting of several segments.
 */
typedef struct {
    ucs_piecewise_segment_t segments[UCX_PIECEWISE_FUNC_MAX_SEGMENTS];
} ucs_piecewise_func_t;


/**
 * Construct a piecewise function
 *
 * @param [in]  c  Piecewise function constant functor
 * @param [in]  m  Piecewise function multiplicative functor
 *
 * @return A piecewise function which represents f(x) = c + x * m on
 *         the unbounded range.
 */
static UCS_F_ALWAYS_INLINE ucs_piecewise_func_t
ucs_piecewise_func_make(double c, double m)
{
    ucs_piecewise_func_t result;

    result.segments[0].func = ucs_linear_func_make(c, m);
    result.segments[0].end  = SIZE_MAX;
    result.segments[0].next = NULL;

    return result;
}


/**
 * Calculate the piecewise function value for a specific point.
 *
 * @param [in] func    Piecewise function to apply.
 * @param [in] x       Point to apply the function at.
 *
 * @return The value of piecewise function in the given point.
 */
static UCS_F_ALWAYS_INLINE double
ucs_piecewise_func_apply(ucs_piecewise_func_t func, double x) // Do we need double for x??? Can we use integral instead?
{
    ucs_piecewise_segment_t *segment = &func.segments[0];

    ucs_assert(x <= SIZE_MAX);
    while (segment->end < x) {
        segment = segment->next;
    }
    ucs_assert(segment != NULL);

    return ucs_linear_func_apply(segment->func, x);
}


/**
 * Sum two piecewise functions.
 *
 * @param [in]  func1    First function to add.
 * @param [in]  func2    Second function to add.
 *
 * @return piecewise function representing (func1 + func2)
 */
static UCS_F_ALWAYS_INLINE ucs_piecewise_func_t
ucs_piecewise_func_add(ucs_piecewise_func_t func1, ucs_piecewise_func_t func2)
{
    // TBD
    return UCS_PIECEWISE_FUNC_ZERO;
}


/**
 * Add one piecewise function to another in-place.
 *
 * @param [inout]  func1    First function to add, and the result of the
 *                          operation
 * @param [in]     func2    Second function to add.
 */
static inline void
ucs_piecewise_func_add_inplace(ucs_piecewise_func_t *func1,
                               ucs_piecewise_func_t func2)
{
    // TBD
}

/**
 * Set new segment on the provided half-interval for the provided piecewise
 * function.
 *
 * @param [inout]  func1    Piecewise function to update.
 * @param [in]     start    Start of the range(not included).
 * @param [in]     end      End of the range(included).
 * @param [in]     trend    Function for the segment.
 */
static inline void
ucs_piecewise_func_set_segment(ucs_piecewise_func_t *func,
                               size_t start, size_t end,
                               ucs_linear_func_t trend)
{
    ucs_assert(start < end);
    // TBD
}


#endif
