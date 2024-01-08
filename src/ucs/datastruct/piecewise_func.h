/*
 * Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCS_PIECEWISE_FUNC_H_
#define UCS_PIECEWISE_FUNC_H_

#include <ucs/datastruct/linear_func.h>
#include <ucs/datastruct/bitmap.h>
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
    ucs_bitmap_t(UCX_PIECEWISE_FUNC_MAX_SEGMENTS) free_segments_bitmap; // use free list (add free_head and consider adding head for used segments)
} ucs_piecewise_func_t;


/**
 * Construct a piecewise function
 *
 * @param [in]  c  Piecewise function constant functor
 * @param [in]  m  Piecewise function multiplicative functor
 *
 * @return A piecewise function which represents f(x) = c + x * m on
 *         the [0, SIZE_MAX] range.
 */
static UCS_F_ALWAYS_INLINE ucs_piecewise_func_t
ucs_piecewise_func_make(double c, double m)
{
    ucs_piecewise_func_t result = {};

    result.segments[0].func = ucs_linear_func_make(c, m);
    result.segments[0].end  = SIZE_MAX;
    result.segments[0].next = NULL;

    UCS_BITMAP_SET_ALL(result.free_segments_bitmap);
    UCS_BITMAP_UNSET(result.free_segments_bitmap, 0);

    return result;
}

static UCS_F_ALWAYS_INLINE void
_ucs_piecewise_func_assert_range(ucs_piecewise_func_t *func) {
#if ENABLE_ASSERT
    ucs_piecewise_segment_t *seg;
    for (seg = func->segments; seg->next != NULL; seg=seg->next) {}
    ucs_assertv(seg->end == SIZE_MAX,
                "piecewise function do not cover SIZE_MAX, end is %ld",
                seg->end);
#endif
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
ucs_piecewise_func_apply(ucs_piecewise_func_t *func, size_t x)
{
    ucs_piecewise_segment_t *seg = func->segments;

    _ucs_piecewise_func_assert_range(func);

    while (seg->end < x) {
        seg = seg->next;
    }

    return ucs_linear_func_apply(seg->func, x);
}


static UCS_F_ALWAYS_INLINE ucs_piecewise_segment_t*
_ucs_piecewise_func_acquire_free_segment(ucs_piecewise_func_t *func) {
    size_t free_idx = UCS_BITMAP_FFS(func->free_segments_bitmap);
    UCS_BITMAP_UNSET(func->free_segments_bitmap, free_idx);
    return &func->segments[free_idx];
}


/**
 * Set new segment on the provided half-interval for the provided piecewise
 * function.
 *
 * @param [inout]  func1    Piecewise function to update.
 * @param [in]     start    Start of the range(included).
 * @param [in]     end      End of the range(included).
 * @param [in]     trend    Function for the segment.
 */
static inline void
ucs_piecewise_func_add_segment(ucs_piecewise_func_t *func,
                               size_t start, size_t end,
                               ucs_linear_func_t trend)
{
    ucs_piecewise_segment_t *seg = func->segments;
    size_t seg_start             = 0;
    ucs_piecewise_segment_t *free_seg;
    size_t prev_end;

    _ucs_piecewise_func_assert_range(func);
    ucs_assert(start <= end);

    while (start > seg->end) {
        seg_start = seg->end + 1;
        seg  = seg->next;
    }

    /* Split the first segment */
    if (seg_start != start) {
        if (seg->end > end) {
            /* The added segment is in the middle of the different segment,
             * so it would split the segment to three parts:
             */

            free_seg       = _ucs_piecewise_func_acquire_free_segment(func);
            free_seg->next = seg->next;
            free_seg->end  = seg->end;
            free_seg->func = seg->func;

            seg->end  = end;
            seg->next = free_seg;
        }

        free_seg       = _ucs_piecewise_func_acquire_free_segment(func);
        free_seg->next = seg->next;
        free_seg->end  = seg->end;
        free_seg->func = seg->func;

        seg->end  = start - 1;
        seg->next = free_seg;

        /* Switch to next segment to not update the original segment by the
         * added one on the next step.
         */
        seg = seg->next;
    }

    /* Update the segments which are fully covered by added segment
     * (including the segments splitted the step before)
     */
    while ((seg != NULL) && (end >= seg->end)) {
        ucs_linear_func_add_inplace(&seg->func, trend);
        prev_end  = seg->end;
        seg       = seg->next;
    }

    /* Split the last segment */
    if ((seg != NULL) && (prev_end != end)) {
        /* All the piecewise functions should cover the [0, SIZE_MAX] range
         * so the last segment splitting is possible only when added segment
         * upper bound is less then SIZE_MAX.
         */
        ucs_assert(end != SIZE_MAX);

        free_seg       = _ucs_piecewise_func_acquire_free_segment(func);
        free_seg->next = seg->next;
        free_seg->end  = seg->end;
        free_seg->func = seg->func;

        seg->end  = end;
        seg->next = free_seg;
        ucs_linear_func_add_inplace(&seg->func, trend);
    }
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
                               ucs_piecewise_func_t *func2)
{
    size_t seg_start = 0;
    ucs_piecewise_segment_t *seg;

    _ucs_piecewise_func_assert_range(func1);
    _ucs_piecewise_func_assert_range(func2);

    for(seg = &func2->segments[0]; seg != NULL; seg = seg->next) {
        ucs_piecewise_func_add_segment(func1, seg_start, seg->end, seg->func);
        seg_start = seg->end + 1;
    }
}


#endif
