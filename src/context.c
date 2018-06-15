/*
 * Copyright (c) 2016 Florent Revest, <florent.revest@free-electrons.com>
 *               2007 Intel Corporation. All Rights Reserved.
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

#include "sunxi_cedrus.h"
#include "context.h"
#include "config.h"
#include "surface.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "v4l2.h"
#include "utils.h"

static VAStatus SunxiCedrusMapSurfaces(struct sunxi_cedrus_driver_data *driver,
				       struct object_context *context,
				       VASurfaceID *ids, unsigned int count)
{
	unsigned int i;
	VAStatus status;
	int rc;

	context->surfaces_count = 0;
	for (i = 0; i < count; i++) {
		struct object_surface *surface = SURFACE(driver, ids[i]);
		unsigned int length;
		unsigned int offset;
		void *data;

		if (surface == NULL) {
			status = VA_STATUS_ERROR_INVALID_SURFACE;
			goto err_unroll;
		}

		if (surface->destination_index != i)
			log("Mismatch between source index %d and destination index %d for surface %d\n", i, surface->destination_index, ids[i]);

		rc = v4l2_request_buffer(driver->video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, i, &length, &offset);
		if (rc < 0) {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto err_unroll;
		}

		data = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, driver->video_fd, offset);
		if (data == MAP_FAILED) {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto err_unroll;
		}

		surface->source_index = i;
		surface->source_data = data;
		surface->source_size = length;
		context->surfaces_count++;
	}

	context->surfaces_ids = ids;
	return VA_STATUS_SUCCESS;

err_unroll:
	for (i = 0; i < context->surfaces_count; i++) {
		struct object_surface *surface = SURFACE(driver, ids[i]);

		if (surface->source_data)
			munmap(surface->source_data, surface->source_size);
	}

	context->surfaces_count = 0;
	context->surfaces_ids = NULL;

	return status;
}

static void SunxiCedrusUnmapSurfaces(struct sunxi_cedrus_driver_data *data,
				     struct object_context *context)
{
	unsigned int i;

	if (context->surfaces_ids == NULL)
		return;

	for (i = 0; i < context->surfaces_count; i++) {
		struct object_surface *surface = SURFACE(data, context->surfaces_ids[i]);

		if (surface == NULL)
			continue;

		if (surface->source_data == NULL)
			continue;

		munmap(surface->source_data, surface->source_size);
	}
}

VAStatus SunxiCedrusCreateContext(VADriverContextP context,
	VAConfigID config_id, int picture_width, int picture_height, int flags,
	VASurfaceID *surfaces_ids, int surfaces_count,
	VAContextID *context_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_config *config_object;
	struct object_context *context_object = NULL;
	VASurfaceID *ids = NULL;
	VAContextID id;
	VAStatus status;
	unsigned int pixelformat;
	int rc;

	config_object = CONFIG(driver_data, config_id);
	if (config_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONFIG;

	id = object_heap_allocate(&driver_data->context_heap);
	context_object = CONTEXT(driver_data, id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	switch (config_object->profile) {
		case VAProfileH264Main:
		case VAProfileH264High:
		case VAProfileH264ConstrainedBaseline:
		case VAProfileH264MultiviewHigh:
		case VAProfileH264StereoHigh:
			pixelformat = V4L2_PIX_FMT_H264_SLICE;
			break;

		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			pixelformat = V4L2_PIX_FMT_MPEG2_FRAME;
			break;

		default:
			status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
			goto err_free_context_object;
	}

	rc = v4l2_set_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, pixelformat, picture_width, picture_height);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto err_free_context_object;
	}

	rc = v4l2_create_buffers(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, surfaces_count);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto err_free_context_object;
	}

	/*
	 * The surface_ids array has been allocated by the caller and
	 * we don't have any indication wrt its life time. Make sure
	 * it's life span is under our control.
	 */
	ids = malloc(surfaces_count * sizeof(VASurfaceID));
	if (ids == NULL) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto err_free_context_object;
	}
	memcpy(ids, surfaces_ids, surfaces_count * sizeof(VASurfaceID));

	status = SunxiCedrusMapSurfaces(driver_data, context_object,
					ids, surfaces_count);
	if (status != VA_STATUS_SUCCESS)
		goto err_free_ids;

	rc = v4l2_set_stream(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, true);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto err_surface;
	}

	rc = v4l2_set_stream(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, true);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto err_surface;
	}

	context_object->config_id = config_id;
	context_object->render_surface_id = VA_INVALID_ID;
	context_object->picture_width = picture_width;
	context_object->picture_height = picture_height;
	context_object->flags = flags;

	*context_id = id;

	return VA_STATUS_SUCCESS;

err_surface:
	SunxiCedrusUnmapSurfaces(driver_data, context_object);
err_free_ids:
	free(ids);
err_free_context_object:
	object_heap_free(&driver_data->context_heap, (struct object_base *) context_object);

	return status;
}

VAStatus SunxiCedrusDestroyContext(VADriverContextP context,
	VAContextID context_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	int rc;

	context_object = CONTEXT(driver_data, context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	object_heap_free(&driver_data->context_heap, (struct object_base *) context_object);

	rc = v4l2_set_stream(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, false);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_stream(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, false);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	return VA_STATUS_SUCCESS;
}
