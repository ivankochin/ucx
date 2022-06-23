/**
 * Copyright (C) Mellanox Technologies Ltd. 2001-2017.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UD_IFACE_COMMON_H
#define UD_IFACE_COMMON_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ucs/config/parser.h>

BEGIN_C_DECLS

/**
 * Common configuration for IB non peer-to-peer transports (UD and DC).
 */
typedef struct uct_ud_iface_common_config {
    unsigned      rx_queue_len_init;
} uct_ud_iface_common_config_t;


extern ucs_config_field_t uct_ud_iface_common_config_table[];

END_C_DECLS

#endif
