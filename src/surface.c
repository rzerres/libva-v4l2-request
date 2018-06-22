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
#include "surface.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <drm_fourcc.h>

#include "v4l2.h"
#include "media.h"
#include "utils.h"

VAStatus SunxiCedrusCreateSurfaces2(VADriverContextP context,
	unsigned int format, unsigned int width, unsigned int height,
	VASurfaceID *surfaces_ids, unsigned int surfaces_count,
	VASurfaceAttrib *attributes, unsigned int attributes_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	unsigned int length[2];
	unsigned int offset[2];
	void *destination_data[2];
	VASurfaceID id;
	unsigned int i, j;
	int rc;

	sunxi_cedrus_log("%s()\n", __func__);

	if (format != VA_RT_FORMAT_YUV420)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	rc = v4l2_set_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_PIX_FMT_MB32_NV12, width, height);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_create_buffers(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surfaces_count);
	if (rc < 0)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	sunxi_cedrus_log("%s() go loop with %d\n", __func__, surfaces_count);

	for (i = 0; i < surfaces_count; i++) {
		id = object_heap_allocate(&driver_data->surface_heap);
		surface_object = SURFACE(id);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

			sunxi_cedrus_log("%s() requesting buffer %d\n", __func__, i);

		rc = v4l2_request_buffer(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, i, length, offset);
		if (rc < 0)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		destination_data[0] = mmap(NULL, length[0], PROT_READ | PROT_WRITE, MAP_SHARED, driver_data->video_fd, offset[0]);
		if (destination_data[0] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		destination_data[1] = mmap(NULL, length[1], PROT_READ | PROT_WRITE, MAP_SHARED, driver_data->video_fd, offset[1]);
		if (destination_data[1] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		sunxi_cedrus_log("%s() requested buffer %d\n", __func__, i);

		surface_object->status = VASurfaceReady;
		surface_object->width = width;
		surface_object->height = height;
		surface_object->source_index = 0;
		surface_object->source_data = NULL;
		surface_object->source_size = 0;
		surface_object->destination_index = 0;

		for (j = 0; j < 2; j++) {
			surface_object->destination_data[j] = destination_data[j];
			surface_object->destination_size[j] = length[j];
		}

		memset(&surface_object->mpeg2_header, 0, sizeof(surface_object->mpeg2_header));
		surface_object->slices_size = 0;
		surface_object->request_fd = -1;

		surfaces_ids[i] = id;
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusCreateSurfaces(VADriverContextP context, int width,
	int height, int format, int surfaces_count, VASurfaceID *surfaces_ids)
{
	sunxi_cedrus_log("%s()\n", __func__);

	return SunxiCedrusCreateSurfaces2(context, format, width, height, surfaces_ids, surfaces_count, NULL, 0);
}

VAStatus SunxiCedrusQuerySurfaceAttributes(VADriverContextP context,
	VAConfigID config, VASurfaceAttrib *attributes,
	unsigned int *attributes_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VASurfaceAttrib *attributes_list;
	unsigned int attributes_list_size = SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES * sizeof(*attributes);
	unsigned int i = 0;

	sunxi_cedrus_log("%s()\n", __func__);

	attributes_list = malloc(attributes_list_size);
	memset(attributes_list, 0, attributes_list_size);

	attributes_list[i].type = VASurfaceAttribPixelFormat;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = VA_FOURCC_NV12;
	i++;

	attributes_list[i].type = VASurfaceAttribMinWidth;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 32;
	i++;

	attributes_list[i].type = VASurfaceAttribMaxWidth;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 2048;
	i++;

	attributes_list[i].type = VASurfaceAttribMinHeight;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 32;
	i++;

	attributes_list[i].type = VASurfaceAttribMaxHeight;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 2048;
	i++;

	attributes_list[i].type = VASurfaceAttribMemoryType;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA |
		VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM |
		VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
	i++;

	attributes_list_size = i * sizeof(*attributes);

	if (attributes != NULL)
		memcpy(attributes, attributes_list, attributes_list_size);

	free(attributes_list);

	*attributes_count = i;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	unsigned int i, j;

	sunxi_cedrus_log("%s()\n", __func__);

	for (i = 0; i < surfaces_count; i++) {
		surface_object = SURFACE(surfaces_ids[i]);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_INVALID_SURFACE;

		if (surface_object->source_data != NULL && surface_object->source_size > 0)
			munmap(surface_object->source_data, surface_object->source_size);

		if (surface_object->request_fd >= 0)
			close(surface_object->request_fd);

		for (j = 0; j < 2; j++)
			if (surface_object->destination_data[j] != NULL && surface_object->destination_size[j] > 0)
				munmap(surface_object->destination_data[j], surface_object->destination_size[j]);

		object_heap_free(&driver_data->surface_heap, (struct object_base *) surface_object);
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSyncSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	VAStatus status;
	int request_fd = -1;
	int rc;

	sunxi_cedrus_log("%s()\n", __func__);

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL) {
			sunxi_cedrus_log("%s() ivalid surface %d\n", __func__, surface_id);
		status = VA_STATUS_ERROR_INVALID_SURFACE;
		goto error;
	}

	if (surface_object->status != VASurfaceRendering) {
			sunxi_cedrus_log("%s() m'kay we are good\n", __func__);
		status = VA_STATUS_SUCCESS;
		goto complete;
	}

	request_fd = surface_object->request_fd;
	if (request_fd < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_queue(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_wait_completion(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_reinit(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_buffer(driver_data->video_fd, -1, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, surface_object->source_index);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_buffer(driver_data->video_fd, -1, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surface_object->destination_index);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	surface_object->status = VASurfaceDisplaying;

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	if (request_fd >= 0) {
		close(request_fd);
		surface_object->request_fd = -1;
	}

complete:
	return status;
}

VAStatus SunxiCedrusQuerySurfaceStatus(VADriverContextP context,
	VASurfaceID surface_id, VASurfaceStatus *status)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	*status = surface_object->status;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusPutSurface(VADriverContextP context, VASurfaceID surface_id,
	void *draw, short src_x, short src_y, unsigned short src_width,
	unsigned short src_height, short dst_x, short dst_y,
	unsigned short dst_width, unsigned short dst_height,
	VARectangle *cliprects, unsigned int cliprects_count,
	unsigned int flags)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusLockSurface(VADriverContextP context,
	VASurfaceID surface_id, unsigned int *fourcc, unsigned int *luma_stride,
	unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
	unsigned int *luma_offset, unsigned int *chroma_u_offset,
	unsigned int *chroma_v_offset, unsigned int *buffer_name, void **buffer)
{
	sunxi_cedrus_log("%s()\n", __func__);

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusUnlockSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	sunxi_cedrus_log("%s()\n", __func__);

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusExportSurfaceHandle(VADriverContextP context,
	VASurfaceID surface_id, uint32_t mem_type, uint32_t flags,
	void *descriptor)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VADRMPRIMESurfaceDescriptor *surface_descriptor =
		(VADRMPRIMESurfaceDescriptor *) descriptor;
	struct object_surface *surface_object;
	unsigned int planes_count = 2;
	int export_fds[planes_count];
	unsigned int i;
	int rc;

	if (mem_type != VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2)
		return VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE;

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	sunxi_cedrus_log("%s()\n", __func__);

	// FIXME: O_RDONLY, flag from flags but meh
	rc = v4l2_export_buffer(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surface_object->destination_index, O_RDONLY, &export_fds, planes_count);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	surface_descriptor->fourcc = VA_FOURCC_NV12;
	surface_descriptor->width = surface_object->width;
	surface_descriptor->height = surface_object->height;
	surface_descriptor->num_objects = planes_count;

	for (i = 0; i < planes_count; i++) {
		surface_descriptor->objects[i].drm_format_modifier = DRM_FORMAT_MOD_NONE;
		surface_descriptor->objects[i].fd = export_fds[i];
		surface_descriptor->objects[i].size = surface_object->destination_size[i];
	}

	surface_descriptor->num_layers = planes_count;

	for (i = 0; i < planes_count; i++) {
		surface_descriptor->layers[i].drm_format = DRM_FORMAT_NV12;
		surface_descriptor->layers[i].num_planes = 1;
		surface_descriptor->layers[i].object_index[0] = i;
		surface_descriptor->layers[i].offset[0] = 0;
		surface_descriptor->layers[i].pitch[0] = surface_object->width; // FIXME? Kodi does not care...
	}

	return VA_STATUS_SUCCESS;
}
