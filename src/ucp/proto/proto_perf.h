/**
 * Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2024. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCP_PROTO_PERF_H_
#define UCP_PROTO_PERF_H_

#include "proto.h"

#include <ucs/datastruct/linear_func.h>
#include <ucs/datastruct/string_buffer.h>
#include <ucs/type/status.h>


/* Protocol performance data structure over multiple ranges */
typedef struct ucp_proto_perf ucp_proto_perf_t;


/* Protocol performance segment, defines the performance in a single range */
typedef struct ucp_proto_perf_segment ucp_proto_perf_segment_t;


/* Protocol performance factor type */
typedef enum {
    UCP_PROTO_PERF_FACTOR_LOCAL_CPU,
    UCP_PROTO_PERF_FACTOR_REMOTE_CPU,
    UCP_PROTO_PERF_FACTOR_LOCAL_TL,
    UCP_PROTO_PERF_FACTOR_REMOTE_TL,
    UCP_PROTO_PERF_FACTOR_LATENCY,
    UCP_PROTO_PERF_FACTOR_LAST
} ucp_proto_perf_factor_id_t;


typedef struct {
    size_t            start;
    size_t            end;
    ucs_linear_func_t value;
} ucp_proto_flat_perf_range_t;

UCS_ARRAY_DECLARE_TYPE(ucp_proto_flat_perf_t, unsigned,
                       ucp_proto_flat_perf_range_t);


/* Array of all performance factors */
typedef ucs_linear_func_t ucp_proto_perf_factors_t[UCP_PROTO_PERF_FACTOR_LAST];

#define UCP_PROTO_PERF_FACTORS_INITIALIZER  {}


/* Iterate on all segments within a given range */
#define ucp_proto_perf_segment_foreach_range(_seg, _seg_start, _seg_end, \
                                              _perf, _range_start, _range_end) \
    for (_seg = ucp_proto_perf_find_segment_lb(_perf, _range_start); \
         (_seg != NULL) && \
         (_seg_start = \
                  ucs_max(_range_start, ucp_proto_perf_segment_start(seg)), \
         _seg_end = ucs_min(_range_end, ucp_proto_perf_segment_end(seg)), \
         _seg_start <= seg_end); \
         _seg = ucp_proto_perf_segment_next(_perf, _seg))


/**
 * Initialize a new performance data structure.
 *
 * @param [in]  name        Name of the performance data structure.
 * @param [out] perf_p      Filled with the new performance data structure.
 */
ucs_status_t ucp_proto_perf_create(const char *name, ucp_proto_perf_t **perf_p);


/**
 * Destroy a performance data structure and free associated memory.
 * The reference counts of any perf_node objects passed to
 * @ref ucp_proto_perf_add_func() will be adjusted accordingly.
 *
 * @param [in]  perf        Performance data structure to destroy.
*/
void ucp_proto_perf_destroy(ucp_proto_perf_t *perf);


int ucp_proto_perf_is_empty(const ucp_proto_perf_t *perf);


/**
 * Add linear functions to several performance factors at the range
 * [ @a start, @a end ]. The performance functions to add are provided in the
 * array @a funcs, each entry corresponding to a factor id defined in
 * @ref ucp_proto_perf_factor_id_t.
 *
 * Initially, all ranges are uninitialized; repeated calls to this function
 * should be used to populate the @a perf data structure.
 *
 * @param [in] perf            Performance data structure to update.
 * @param [in] start           Add the performance function to this range start (inclusive).
 * @param [in] end             Add the performance function to this range end (inclusive).
 * @param [in] perf_factors    Array of performance functions to add.
 * @param [in] perf_node       Performance node that represents the added function.
 *                             Can be NULL.
 *
 *  TODO add option to create custom perf node (a child of the range) when adding
 *
 * @note This function may adjust the reference count of @a perf_node as needed.
 */
ucs_status_t
ucp_proto_perf_add_funcs(ucp_proto_perf_t *perf, size_t start, size_t end,
                         const ucp_proto_perf_factors_t perf_factors,
                         ucp_proto_perf_node_t *perf_node, const char *title,
                         const char *desc_fmt, ...);


/**
 * Create a proto perf structure that is the aggregation of multiple other perf
 * structures: In the ranges where ALL given perf structures are defined, the
 * result is the factor-wise sum of the performance values. The performance node
 * of the resulting range will be the parent of the respective ranges in the
 * provided perf structures.
 * Other ranges, where at least one of the given perf structures is not defined,
 * will also not be defined in the result.
 *
 * @param [in]  name        Name of the performance data structure.
 * @param [in]  perf_elems  Array of pointers to the performance structures
 *                          that should be aggregated.
 * @param [in]  num_elems   Number of elements in @a perf_elems array.
 * @param [out] perf_p      Filled with the new performance data structure.
*/
ucs_status_t ucp_proto_perf_aggregate(const char *name,
                                      const ucp_proto_perf_t *const *perf_elems,
                                      unsigned num_elems,
                                      ucp_proto_perf_t **perf_p);


ucs_status_t ucp_proto_perf_aggregate2(const char *name,
                                       const ucp_proto_perf_t *perf1,
                                       const ucp_proto_perf_t *perf2,
                                       ucp_proto_perf_t **perf_p);


/**
 * Create a proto perf structure that is equal to @a remote_perf but all
 * local factors' values are turned to remote ones and vice versa.
 *
 * @param [in]  remote_perf Performance data structure to turn.
 * @param [out] perf_p      Filled with the new performance data structure.
*/
ucs_status_t ucp_proto_perf_turn_remote(const ucp_proto_perf_t *remote_perf,
                                        ucp_proto_perf_t **perf_p);


/* Envelope */
ucs_status_t
ucp_proto_perf_envelope(const ucp_proto_perf_t *perf, int convex,
                        ucp_proto_flat_perf_t *flat_perf);


/* Sum of all parts */
ucs_status_t ucp_proto_perf_sum(const ucp_proto_perf_t *perf,
                                 ucp_proto_flat_perf_t *flat_perf);


/**
 * Find the first segment that contains a point greater than or equal to a given
 * lower bound value.
 *
 * @param [in] perf          Performance data structure.
 * @param [in] lb            Lower bound of the segment to find.
 *
 * @return Pointer to the first segment that contains a point greater than or
 *         equal to @a lb, or NULL if all segments end before @a lb.
 */
ucp_proto_perf_segment_t *
ucp_proto_perf_find_segment_lb(const ucp_proto_perf_t *perf, size_t lb);


/**
 * Get the performance function of a given factor at a given segment.
 *
 * @param [in] seg           Segment to get the performance function from.
 * @param [in] factor_id     Performance factor id.
 *
 * @return The performance function of @a factor_id at @a seg.
 */
ucs_linear_func_t
ucp_proto_perf_segment_func(const ucp_proto_perf_segment_t *seg,
                            ucp_proto_perf_factor_id_t factor_id);


/**
 * Get the start point of a given segment.
 *
 * @param [in] seg           Segment to get the start value from.
 *
 * @return The start point of @a seg.
 */
size_t ucp_proto_perf_segment_start(const ucp_proto_perf_segment_t *seg);


/**
 * Get end point of a given segment.
 *
 * @param [in] seg           Segment to get the end value from.
 *
 * @return The end point of @a seg.
 */
size_t ucp_proto_perf_segment_end(const ucp_proto_perf_segment_t *seg);


/**
 * Get the performance node of a given segment.
 *
 * @param [in] seg           Segment to get the performance node from.
 *
 * @return The performance node of @a seg.
 */
ucp_proto_perf_node_t *
ucp_proto_perf_segment_node(const ucp_proto_perf_segment_t *seg);


/**
 * Get next segment, or NULL if none.
 */
const ucp_proto_perf_segment_t *
ucp_proto_perf_segment_next(const ucp_proto_perf_t *perf,
                            const ucp_proto_perf_segment_t *seg);


/**
 * Get last segment, or NULL if none.
 */
const ucp_proto_perf_segment_t *
ucp_proto_perf_segment_last(const ucp_proto_perf_t *perf);


void ucp_proto_perf_segment_str(const ucp_proto_perf_segment_t *seg,
                                ucs_string_buffer_t *strb);


/**
 * Dump the performance data structure to a string buffer.
 *
 * @param [in]  perf        Performance data structure to dump.
 * @param [out] strb        String buffer to dump the performance data to.
 */
void ucp_proto_perf_str(const ucp_proto_perf_t *perf,
                        ucs_string_buffer_t *strb);


const ucp_proto_flat_perf_range_t *
ucp_proto_flat_perf_find_lb(const ucp_proto_flat_perf_t *flat_perf, size_t lb);


#endif
