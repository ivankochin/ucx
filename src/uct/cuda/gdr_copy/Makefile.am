#
# Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2001-2017. ALL RIGHTS RESERVED.
# See file LICENSE for terms.
#

if HAVE_GDR_COPY

module_LTLIBRARIES              = libuct_cuda_gdrcopy.la
libuct_cuda_gdrcopy_la_CPPFLAGS = $(BASE_CPPFLAGS) $(GDR_COPY_CPPFLAGS)
libuct_cuda_gdrcopy_la_CFLAGS   = $(BASE_CFLAGS) $(LT_CFLAGS)
libuct_cuda_gdrcopy_la_LDFLAGS  = $(GDR_COPY_LDFLAGS) -version-info $(SOVERSION)
libuct_cuda_gdrcopy_la_LIBADD   = $(top_builddir)/src/ucs/libucs.la \
                                  $(top_builddir)/src/uct/cuda/libuct_cuda.la \
                                  $(GDR_COPY_LIBS)

noinst_HEADERS = \
	gdr_copy_md.h \
	gdr_copy_iface.h \
	gdr_copy_ep.h

libuct_cuda_gdrcopy_la_SOURCES = \
	gdr_copy_md.c \
	gdr_copy_iface.c \
	gdr_copy_ep.c

PKG_CONFIG_NAME=gdrcopy

include $(top_srcdir)/config/module.am
# TODO: enable pkg-config processing when module static build is enabled
# include $(top_srcdir)/config/module-pkg-config.am

endif
