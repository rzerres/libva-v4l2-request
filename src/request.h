/*
 * Copyright (C) 2007 Intel Corporation
 * Copyright (C) 2016 Florent Revest <florent.revest@free-electrons.com>
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
 * Copyright (C) 2019 Ralf Zerres <ralf.zerres@networkx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _V4L2_REQUEST_H_
#define _V4L2_REQUEST_H_

#include <linux/videodev2.h>
#include <stdbool.h>
#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_backend_vpp.h>
#include <va/va_enc_h264.h>
#include <va/va_enc_hevc.h>
#include <va/va_enc_mpeg2.h>
#include <va/va_fei.h>
#include <va/va_fei_h264.h>
#include <va/va_vpp.h>

#include "autoconfig.h"

#include "context.h"
#include "compiler.h"
#include "media.h"
#include "object_heap.h"
#include "va_backend_compat.h"
#include "video.h"
#include "utils.h"

//#ifdef HAVE_VA_X11
//# include "i965_output_dri.h"
//#endif

#ifdef HAVE_VA_WAYLAND
#include "v4l2_output_wayland.h"
#endif


#define V4L2_REQUEST_MAX_PROFILES		11
#define V4L2_REQUEST_MAX_ENTRYPOINTS		5
#define V4L2_REQUEST_MAX_CONFIG_ATTRIBUTES	10
#define V4L2_REQUEST_MAX_IMAGE_FORMATS		10
#define V4L2_REQUEST_MAX_SUBPIC_FORMATS		4
#define V4L2_REQUEST_MAX_DISPLAY_ATTRIBUTES	4

#define V4L2_REQUEST_STR_DRIVER_VENDOR		"V4L2"
#define V4L2_REQUEST_STR_DRIVER_NAME		"v4l2_request"

struct v4l2_request_data {
	struct object_heap config_heap;
	struct object_heap context_heap;
	struct object_heap surface_heap;
	struct object_heap buffer_heap;
	struct object_heap image_heap;
	struct object_heap subpic_heap;
	struct hw_codec_info *codec_info;
	struct video_format *video_format;
	//struct v4l2_render_state render_state;
#ifdef WITH_X11
	// VA/DRI (X11) specific data
	struct va_dri_output *dri_output;
#endif
#ifdef WITH_WAYLAND
	// VA/Wayland specific data
	struct va_wl_output *wl_output;
#endif

	pthread_mutex_t render_mutex;
	pthread_mutex_t pp_mutex;
	void *pp_context;
	char va_vendor[256];

	VADisplayAttribute *display_attributes;
	unsigned int num_display_attributes;
	VADisplayAttribute *rotation_attrib;
	VADisplayAttribute *brightness_attrib;
	VADisplayAttribute *contrast_attrib;
	VADisplayAttribute *hue_attrib;
	VADisplayAttribute *saturation_attrib;
	VAContextID current_context_id;

	VADriverContextP wrapper_pdrvcontext;

	int drm_fd;
	int video_fd;
	int media_fd;
};

#define NEW_CONFIG_ID() object_heap_allocate(&v4l2_request->config_heap);
#define NEW_CONTEXT_ID() object_heap_allocate(&v4l2_request->context_heap);
#define NEW_SURFACE_ID() object_heap_allocate(&v4l2_request->surface_heap);
#define NEW_BUFFER_ID() object_heap_allocate(&v4l2_request->buffer_heap);
#define NEW_IMAGE_ID() object_heap_allocate(&v4l2_request->image_heap);
#define NEW_SUBPIC_ID() object_heap_allocate(&v4l2_request->subpic_heap);

//#define CONFIG(id) ((struct object_config *)object_heap_lookup(&v4l2->config_heap, id))
//#define CONTEXT(id) ((struct object_context *)object_heap_lookup(&v4l2->context_heap, id))
//#define SURFACE(id) ((struct object_surface *)object_heap_lookup(&v4l2->surface_heap, id))
//#define BUFFER(id) ((struct object_buffer *)object_heap_lookup(&v4l2->buffer_heap, id))
//#define IMAGE(id) ((struct object_image *)object_heap_lookup(&v4l2->image_heap, id))
//#define SUBPIC(id) ((struct object_subpic *)object_heap_lookup(&v4l2->subpic_heap, id))

#define FOURCC_IA44 0x34344149
#define FOURCC_AI44 0x34344941

#define STRIDE(w)               (((w) + 0xf) & ~0xf)
#define SIZE_YUV420(w, h)       (h * (STRIDE(w) + STRIDE(w >> 1)))

static INLINE struct v4l2_request_data *v4l2_request_RequestData(VADriverContextP context)
{
	return (struct v4l2_request_data *)(context->pDriverData);
}

bool v4l2_request_VendorString(struct v4l2_request_data *v4l2_request);

VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP context);

#endif
