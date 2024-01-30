/*
 * Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ucs/datastruct/piecewise_func.h>
#include <ucs/debug/memtrack_int.h>
#include <ucs/debug/assert.h>
#include <limits.h>
#include <stdint.h>

#define piecewise_func_head(_pw_func) \
    ucs_list_head(&(_pw_func)->head, ucs_piecewise_segment_t, list);

static ucs_piecewise_segment_t *
ucs_piecewise_segment_insert_after(ucs_linear_func_t func, size_t end,
                                   ucs_list_link_t *prev)
{
    ucs_piecewise_segment_t *result;

    result = ucs_malloc(sizeof(ucs_piecewise_segment_t), "piecewise segment");
    if (result == NULL) {
        return result;
    }

    result->func = func;
    result->end  = end;
    ucs_list_insert_after(prev, &result->list);

    return result;
}

ucs_status_t ucs_piecewise_func_init(ucs_piecewise_func_t *pw_func)
{
    ucs_piecewise_segment_t *seg;

    ucs_list_head_init(&pw_func->head);

    seg = ucs_piecewise_segment_insert_after(UCS_LINEAR_FUNC_ZERO, SIZE_MAX,
                                             &pw_func->head);
    if (seg == NULL) {
        return UCS_ERR_NO_MEMORY;
    }

    return UCS_OK;
}

void ucs_piecewise_func_cleanup(ucs_piecewise_func_t *pw_func)
{
    ucs_piecewise_segment_t *seg, *tmp;

    ucs_list_for_each_safe(seg, tmp, &pw_func->head, list) {
        ucs_free(seg);
    }
}

static void ucs_piecewise_func_check(ucs_piecewise_func_t *pw_func)
{
#if ENABLE_ASSERT
    size_t prev_end = 0;
    ucs_piecewise_segment_t *seg, *head;

    ucs_assertv(!ucs_list_is_empty(&pw_func->head), "pw_func=%p", pw_func);

    ucs_piecewise_func_seg_foreach(pw_func, seg) {
        /* First segment has no prev */
        if (seg != piecewise_func_head(pw_func)) {
            ucs_assertv(seg->end > prev_end,
                        "pw_func=%p seg->end=%zu prev_end=%zu",
                        pw_func, seg->end, prev_end);
        }
        prev_end = seg->end;
    }

    seg = ucs_list_tail(&pw_func->head, ucs_piecewise_segment_t, list);
    ucs_assertv(seg->end == SIZE_MAX, "pw_func=%p seg->end=%zu", pw_func,
                seg->end);
#endif
}

static ucs_piecewise_segment_t *
ucs_piecewise_func_find_segment(const ucs_piecewise_func_t *pw_func, size_t x)
{
    ucs_piecewise_segment_t *seg;

    ucs_piecewise_func_seg_foreach(pw_func, seg) {
        if (x <= seg->end) {
            break;
        }
    }

    return seg;
}

double ucs_piecewise_func_apply(const ucs_piecewise_func_t *pw_func, size_t x)
{
    ucs_piecewise_segment_t *seg = ucs_piecewise_func_find_segment(pw_func, x);

    return ucs_linear_func_apply(seg->func, x);
}

static ucs_status_t
ucs_piecewise_segment_split(ucs_piecewise_segment_t *seg, size_t split_point)
{
    ucs_piecewise_segment_t *new_seg;

    ucs_assertv(split_point < seg->end, "seg=%p seg->end=%zu split_point=%zu",
                seg, seg->end, split_point);

    new_seg  = ucs_piecewise_segment_insert_after(seg->func, seg->end,
                                                  &seg->list);
    if (new_seg == NULL) {
        return UCS_ERR_NO_MEMORY;
    }

    seg->end = split_point;
    return UCS_OK;
}

ucs_status_t ucs_piecewise_func_add_range(ucs_piecewise_func_t *pw_func,
                                          size_t start, size_t end,
                                          ucs_linear_func_t range_func)
{
    size_t seg_start = 0;
    ucs_piecewise_segment_t *seg;
    ucs_status_t status;

    ucs_piecewise_func_check(pw_func);
    ucs_assertv(start <= end, "pw_func=%p start=%zu end=%zu", pw_func, start,
                end);

    /*                 ___________________________________
     * func before:   |__________|______|_________________|
     *                       __________________
     * range to add:        |__________________|
     *                 ___________________________________
     * func after:    |_____|____|_______|_____|__________|
     */
    ucs_piecewise_func_seg_foreach(pw_func, seg) {
        if (start <= seg->end) {
            /* Split the first affected segment*/
            if (start > seg_start) {
                ucs_assertv(start > 0, "pw_func=%p start=%zu", pw_func, start);
                status = ucs_piecewise_segment_split(seg, start - 1);
                if (status != UCS_OK) {
                    return status;
                }

                /* Move to the first segment affected by the specified range */
                seg = ucs_list_next(&seg->list, ucs_piecewise_segment_t, list);
            }

            /* Split the last affected segment */
            if (end < seg->end) {
                status = ucs_piecewise_segment_split(seg, end);
                if (status != UCS_OK) {
                    return status;
                }
            }

            /* Sum up funcs for the segments which are fully covered */
            if (end >= seg->end) {
                ucs_linear_func_add_inplace(&seg->func, range_func);
            }

            if (end == seg->end) {
                break;
            }
        }

        seg_start = seg->end + 1;
    }

    return UCS_OK;
}

ucs_status_t ucs_piecewise_func_add_inplace(ucs_piecewise_func_t *dst_pw_func,
                                            ucs_piecewise_func_t *src_pw_func)
{
    size_t seg_start = 0;
    ucs_piecewise_segment_t *seg;
    ucs_status_t status;

    ucs_piecewise_func_check(src_pw_func);

    ucs_piecewise_func_seg_foreach(src_pw_func, seg) {
        status = ucs_piecewise_func_add_range(dst_pw_func, seg_start, seg->end,
                                              seg->func);
        if (status != UCS_OK) {
            return status;
        }

        seg_start = seg->end + 1;
    }

    return UCS_OK;
}

ucs_status_t
ucs_piecewise_func_make_max_inplace(ucs_piecewise_func_t *dst_pw_func,
                                    ucs_piecewise_func_t *src_pw_func)
{
    size_t seg_start = 0;
    ucs_piecewise_segment_t *dst_seg, *src_seg;
    double cross_point, dst_value, src_value;
    ucs_status_t status;
    size_t seg_end;

    ucs_piecewise_func_check(dst_pw_func);
    ucs_piecewise_func_check(src_pw_func);

    do {
        dst_seg = ucs_piecewise_func_find_segment(dst_pw_func, seg_start);
        src_seg = ucs_piecewise_func_find_segment(dst_pw_func, seg_start);
        seg_end = ucs_min(dst_seg->end, src_seg->end);

        status  = ucs_linear_func_intersect(dst_seg->func, src_seg->func,
                                                           &cross_point);
        if (status != UCS_OK) {
            ucs_trace("No intersection since segments are parallel. "
                      "dst_pw_func=%p dst_seg->func=%f + %f*x dst_seg->end=%ld "
                      "src_pw_func=%p src_seg->func=%f + %f*x src_seg->end=%ld",
                      dst_pw_func, dst_seg->func.c, dst_seg->func.m,
                      src_pw_func, src_seg->func.c, src_seg->func.m);
        // CALCULATIONS CAN BE INACCURATE DUE TO ROUNDING - IS THAT A PROBLEM??
        } else if ((size_t)cross_point >= seg_start) {
            seg_end = ucs_min(seg_end, (size_t)cross_point);
        }

        dst_value = ucs_linear_func_apply(dst_seg->func, seg_end);
        src_value = ucs_linear_func_apply(src_seg->func, seg_end);

        if (seg_end != dst_seg->end) {
            status = ucs_piecewise_segment_split(dst_seg, seg_end);
        }

        if (src_value > dst_value) {
            dst_seg->func = src_seg->func;
        }

        seg_start = seg_end + 1;
    } while (seg_end != SIZE_MAX);
}
