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
    UCP_PROTO_PERF_FACTOR_SINGLE, /* For compatibility; to remove */
    UCP_PROTO_PERF_FACTOR_MULTI,  /* For compatibility; to remove */
    UCP_PROTO_PERF_FACTOR_LAST
} ucp_proto_perf_factor_id_t;


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


/**
 * Create proto perf structure from proto caps.
 *
 * @param [in]  name        Name of the performance data structure.
 * @param [in]  proto_caps  Proto caps to create perf from.
 * @param [out] perf_p      Filled with the new performance data structure.
*/
ucs_status_t ucp_proto_perf_from_caps(const char *name,
                                      const ucp_proto_caps_t *proto_caps,
                                      ucp_proto_perf_t **perf_p);


/**
 * Add the linear function @a func to the performance of @a factor_id at the
 * range [ @a start, @a end ].
 * Initially, all ranges are uninitialized; repeated calls to this function
 * should be used to populate the @a perf data structure.
 *
 * @param [in] perf          Performance data structure to update.
 * @param [in] start         Add the performance function to this range start (inclusive).
 * @param [in] end           Add the performance function to this range end (inclusive).
 * @param [in] factor_id     Performance factor id.
 * @param [in] func          Performance function to add.
 * @param [in] perf_node     Performance node that represents the added function.
 *                           Can be NULL.
 *
 * @note This function may adjust the reference count of @a perf_node as needed.
 */
ucs_status_t ucp_proto_perf_add_func(ucp_proto_perf_t *perf, size_t start,
                                     size_t end,
                                     ucp_proto_perf_factor_id_t factor_id,
                                     ucs_linear_func_t func,
                                     ucp_proto_perf_node_t *perf_node);


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
 * Dump the performance data structure to a string buffer.
 *
 * @param [in]  perf        Performance data structure to dump.
 * @param [out] strb        String buffer to dump the performance data to.
 */
void ucp_proto_perf_dump(const ucp_proto_perf_t *perf,
                         ucs_string_buffer_t *strb);

#endif
