/*
 * Copyright (C) 2007 Intel Corporation
 * Copyright (C) 2016 Florent Revest <florent.revest@free-electrons.com>
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
 * Copyright (C) 2018 Ralf Zerres <ralf.zerres@networkx.de>
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

#define _GNU_SOURCE
#include "buffer.h"
#include "config.h"
#include "context.h"
#include "image.h"
#include "picture.h"
#include "subpicture.h"
#include "surface.h"

#include "autoconfig.h"

#include "request.h"
#include "utils.h"
#include "v4l2.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <va/va_backend.h>

#define CONFIG_ID_OFFSET                0x01000000
#define CONTEXT_ID_OFFSET               0x02000000
#define SURFACE_ID_OFFSET               0x04000000
#define BUFFER_ID_OFFSET                0x08000000
#define IMAGE_ID_OFFSET                 0x0a000000
#define SUBPIC_ID_OFFSET                0x10000000

uint32_t g_v4l2_request_debug_option_flags = 0;

static bool v4l2_request_DataInit(VADriverContextP context)
{
	// access driver specific data via context->pDriverData
	struct v4l2_request_data *v4l2_request = v4l2_request_RequestData(context);

	//struct drm_state * const drm_state = (struct drm_state *)context->drm_state;
	//int has_exec2 = 0, has_bsd = 0, has_blt = 0, has_vebox = 0;
	char *env_str = NULL;

	// Debug settings
	if ((env_str = getenv("VA_V4L2_REQUEST_DEBUG")))
		g_v4l2_request_debug_option_flags = atoi(env_str);

	if (g_v4l2_request_debug_option_flags)
		v4l2_request_log_info(context, "Debug settings: %x\n", g_v4l2_request_debug_option_flags);

	// Debug settings
	if ((env_str = getenv("VA_V4L2_REQUEST_DEBUG")))
		g_v4l2_request_debug_option_flags = atoi(env_str);

	if (g_v4l2_request_debug_option_flags)
		v4l2_request_log_info(context, "Debug settings: %x\n", g_v4l2_request_debug_option_flags);


	//v4l2_request_driver_get_revid(v4l2_request, &v4l2_request->revision);

	// prepare VendorString
	if (!v4l2_request_VendorString(v4l2_request))
	    return VA_STATUS_ERROR_ALLOCATION_FAILED;

	// initialize the object heap
	if (object_heap_init(&v4l2_request->config_heap,
			     sizeof(struct object_config),
			     CONFIG_ID_OFFSET))
		goto err_config_heap;

	if (object_heap_init(&v4l2_request->context_heap,
			     sizeof(struct object_context),
			     CONTEXT_ID_OFFSET))
		goto err_context_heap;

	if (object_heap_init(&v4l2_request->surface_heap,
			     sizeof(struct object_surface),
			     SURFACE_ID_OFFSET))
		goto err_surface_heap;

	if (object_heap_init(&v4l2_request->buffer_heap,
			     sizeof(struct object_buffer),
			     BUFFER_ID_OFFSET))
		goto err_buffer_heap;

	if (object_heap_init(&v4l2_request->image_heap,
			     sizeof(struct object_image),
			     IMAGE_ID_OFFSET))
		goto err_image_heap;

	/*
	if (object_heap_init(&v4l2_request->subpic_heap,
			     sizeof(struct object_subpic),
			     SUBPIC_ID_OFFSET))
			goto err_subpic_heap;
	*/

	//v4l2_request->batch = v4l2_request_batchbuffer_new(&v4l2_request->v4l2_request, I915_EXEC_RENDER, 0);
	//v4l2_request->pp_batch = v4l2_request_batchbuffer_new(&v4l2_request->v4l2_request, I915_EXEC_RENDER, 0);
	//_v4l2_requestInitMutex(&v4l2_request->render_mutex);
	//_v4l2_requestInitMutex(&v4l2_request->pp_mutex);

	return true;

//err_subpic_heap:
//	object_heap_destroy(&v4l2_request->image_heap);
err_image_heap:
	object_heap_destroy(&v4l2_request->buffer_heap);
err_buffer_heap:
	object_heap_destroy(&v4l2_request->surface_heap);
err_surface_heap:
	object_heap_destroy(&v4l2_request->context_heap);
err_context_heap:
	object_heap_destroy(&v4l2_request->config_heap);
err_config_heap:

	return false;
}

VAStatus  v4l2_request_DataTerminate(VADriverContextP context)
{
	struct v4l2_request_data *v4l2_request = v4l2_request_RequestData(context);
	//pthread_mutex_destroy(&v4l2_request->contextmutex);

	struct object_buffer *buffer_object;
	struct object_image *image_object;
	struct object_surface *surface_object;
	struct object_context *context_object;
	struct object_config *config_object;
	int iterator;

	close(v4l2_request->video_fd);
	close(v4l2_request->media_fd);

	/* Cleanup leftover buffers. */
	/*
	subpic_object = (struct object_subpic *)
		object_heap_first(&v4l2_request->subpic_heap, &iterator);
	while (subpic_object != NULL) {
		RequestDestroyBuffer(context, (VASubpicID)buffer_object->base.id);
		subpic_object = (struct object_subpic *)
			object_heap_next(&v4l2_request->subpic_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->subpic_heap);
	*/

	image_object = (struct object_image *)
		object_heap_first(&v4l2_request->image_heap, &iterator);
	while (image_object != NULL) {
		RequestDestroyImage(context, (VAImageID)image_object->base.id);
		image_object = (struct object_image *)
			object_heap_next(&v4l2_request->image_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->image_heap);

	buffer_object = (struct object_buffer *)
		object_heap_first(&v4l2_request->buffer_heap, &iterator);
	while (buffer_object != NULL) {
		RequestDestroyBuffer(context,
				     (VABufferID)buffer_object->base.id);
		buffer_object = (struct object_buffer *)
			object_heap_next(&v4l2_request->buffer_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->buffer_heap);

	surface_object = (struct object_surface *)
		object_heap_first(&v4l2_request->surface_heap, &iterator);
	while (surface_object != NULL) {
		RequestDestroySurfaces(context,
				      (VASurfaceID *)&surface_object->base.id, 1);
		surface_object = (struct object_surface *)
			object_heap_next(&v4l2_request->surface_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->surface_heap);

	context_object = (struct object_context *)
		object_heap_first(&v4l2_request->context_heap, &iterator);
	while (context_object != NULL) {
		RequestDestroyContext(context,
				      (VAContextID)context_object->base.id);
		context_object = (struct object_context *)
			object_heap_next(&v4l2_request->context_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->context_heap);

	config_object = (struct object_config *)
		object_heap_first(&v4l2_request->config_heap, &iterator);
	while (config_object != NULL) {
		RequestDestroyConfig(context,
				     (VAConfigID)config_object->base.id);
		config_object = (struct object_config *)
			object_heap_next(&v4l2_request->config_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->config_heap);

	free(v4l2_request);
	v4l2_request = NULL;

	return VA_STATUS_SUCCESS;
}

static bool v4l2_request_DriverInit(VADriverContextP context, struct driver *driver)
{
	struct v4l2_request_data *v4l2_request = v4l2_request_RequestData(context);

	//pthread_mutex_destroy(&v4l2_request->contextmutex);
	struct v4l2_request_data *driver_data = context->pDriverData;
	struct object_buffer *buffer_object;
	struct object_image *image_object;
	struct object_surface *surface_object;
	struct object_context *context_object;
	struct object_config *config_object;
	int iterator;

	close(v4l2_request->video_fd);
	close(v4l2_request->media_fd);

	/* Cleanup leftover buffers. */
	/*
	subpic_object = (struct object_subpic *)
		object_heap_first(&v4l2_request->subpic_heap, &iterator);
	while (subpic_object != NULL) {
		RequestDestroyBuffer(context, (VASubpicID)buffer_object->base.id);
		subpic_object = (struct object_subpic *)
			object_heap_next(&v4l2_request->subpic_heap, &iterator);
	}

	object_heap_destroy(&v4l2_request->subpic_heap);
	*/

	image_object = (struct object_image *)
		object_heap_first(&driver_data->image_heap, &iterator);
	while (image_object != NULL) {
		RequestDestroyImage(context, (VAImageID)image_object->base.id);
		image_object = (struct object_image *)
			object_heap_next(&driver_data->image_heap, &iterator);
	}

	object_heap_destroy(&driver_data->image_heap);

	buffer_object = (struct object_buffer *)
		object_heap_first(&driver_data->buffer_heap, &iterator);
	while (buffer_object != NULL) {
		RequestDestroyBuffer(context,
				     (VABufferID)buffer_object->base.id);
		buffer_object = (struct object_buffer *)
			object_heap_next(&driver_data->buffer_heap, &iterator);
	}

	object_heap_destroy(&driver_data->buffer_heap);

	surface_object = (struct object_surface *)
		object_heap_first(&driver_data->surface_heap, &iterator);
	while (surface_object != NULL) {
		RequestDestroySurfaces(context,
				      (VASurfaceID *)&surface_object->base.id, 1);
		surface_object = (struct object_surface *)
			object_heap_next(&driver_data->surface_heap, &iterator);
	}

	object_heap_destroy(&driver_data->surface_heap);

	context_object = (struct object_context *)
		object_heap_first(&driver_data->context_heap, &iterator);
	while (context_object != NULL) {
		RequestDestroyContext(context,
				      (VAContextID)context_object->base.id);
		context_object = (struct object_context *)
			object_heap_next(&driver_data->context_heap, &iterator);
	}

	object_heap_destroy(&driver_data->context_heap);

	config_object = (struct object_config *)
		object_heap_first(&driver_data->config_heap, &iterator);
	while (config_object != NULL) {
		RequestDestroyConfig(context,
				     (VAConfigID)config_object->base.id);
		config_object = (struct object_config *)
			object_heap_next(&driver_data->config_heap, &iterator);
	}

	object_heap_destroy(&driver_data->config_heap);

	free(context->pDriverData);
	context->pDriverData = NULL;

	return VA_STATUS_SUCCESS;
}

static bool v4l2_request_DriverInit(VADriverContextP context, struct driver *driver)
{
	//struct driver *driver;
	struct decoder *decoder = NULL;
	char *media_path;
	char *subsystem;
	int rc;

	/* create driver structure */
	if (g_v4l2_request_debug_option_flags)
		v4l2_request_log_info(context, "Initializing driver structure...\n");
	driver_init(driver);

	/* environment settings */
	media_path = getenv("LIBVA_V4L2_REQUEST_MEDIA_PATH");

	if (media_path == NULL) {
		subsystem = "media";
		if (g_v4l2_request_debug_option_flags)
			v4l2_request_log_info(context, "Scanning for suitable v4l2 drivers (subsystem: %s)...\n",
					      subsystem);
		rc = udev_scan_subsystem(context, driver, subsystem);
	}
	else {
		if (g_v4l2_request_debug_option_flags)
			v4l2_request_log_info(context, "Probing v4l2 topology (media_path: %s)...\n",
					      media_path);

		// update media_path in driver structure
		driver_set(driver, 0, decoder);
		asprintf(&decoder->media_path, "%s", media_path);

		rc = media_scan_topology(context, driver, 0, media_path);
		if (rc < 0) {
			v4l2_request_log_error(context, "No capable v4l2 video-decoder for media_path '%s'.\n",
				    media_path);
			driver_delete(driver, 0);
		}
		/*
		else if (rc == 1)
			v4l2_request_log_info(context, "media_path '%s' offers streaming v4l2 video-decoder.\n",
				    media_path);
		*/
	}

	return rc;
}

extern const struct v4l2_request_device_info *v4l2_request_GetDeviceInfo(int devid);

bool v4l2_request_VendorString(struct v4l2_request_data *v4l2_request)
{
	int rc, len;

	if (v4l2_request->va_vendor[0] != '\0')
		return true;

	len = 0;
	rc = snprintf(v4l2_request->va_vendor, sizeof(v4l2_request->va_vendor),
		       "%s %s driver - %s.%s.%s",
		       V4L2_REQUEST_STR_DRIVER_VENDOR, V4L2_REQUEST_STR_DRIVER_NAME,
		       V4L2_REQUEST_DRIVER_MAJOR_VERSION, V4L2_REQUEST_DRIVER_MINOR_VERSION,
		       V4L2_REQUEST_DRIVER_MICRO_VERSION);
	if (rc < 0 || rc >= sizeof(v4l2_request->va_vendor))
		goto error;
	len = rc;

	if (V4L2_REQUEST_DRIVER_PRE_VERSION != NULL) {
		rc = snprintf(&v4l2_request->va_vendor[len], sizeof(v4l2_request->va_vendor) - len,
			      ".pre%s", V4L2_REQUEST_DRIVER_PRE_VERSION);
		if (rc < 0 || rc >= (sizeof(v4l2_request->va_vendor) - len))
			goto error;
		len += rc;

		rc = snprintf(&v4l2_request->va_vendor[len], sizeof(v4l2_request->va_vendor) - len,
			      " (%s)g56", V4L2_REQUEST_DRIVER_GIT_VERSION);
		if (rc < 0 || rc >= (sizeof(v4l2_request->va_vendor) - len))
			goto error;
		len += rc;
	}
	return true;

error:
	v4l2_request->va_vendor[0] = '\0';
	v4l2_request_assert_rc(rc > 0 && len < sizeof(v4l2_request->va_vendor), false);
	return false;
}

/* Set default visibility for the init function only. */
VAStatus __attribute__((visibility("default")))
VA_DRIVER_INIT_FUNC(VADriverContextP context);

// libva/va/va_backend.h:493
VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP context)
{
	struct VADriverVTable *vtable = context->vtable;
	//struct VADriverVTableVPP * const vtable_vpp = context->vtable_vpp;

	struct v4l2_request_data *v4l2_request;
	struct driver *driver;
	struct decoder *decoder;

	VAStatus status = VA_STATUS_ERROR_UNKNOWN;

	int video_fd = -1;
	int media_fd = -1;
	int rc;

	context->version_major = VA_MAJOR_VERSION;
	context->version_minor = VA_MINOR_VERSION;
	context->max_profiles = V4L2_REQUEST_MAX_PROFILES;
	context->max_entrypoints = V4L2_REQUEST_MAX_ENTRYPOINTS;
	context->max_attributes = V4L2_REQUEST_MAX_CONFIG_ATTRIBUTES;
	context->max_image_formats = V4L2_REQUEST_MAX_IMAGE_FORMATS;
	context->max_subpic_formats = V4L2_REQUEST_MAX_SUBPIC_FORMATS;
	context->max_display_attributes = V4L2_REQUEST_MAX_DISPLAY_ATTRIBUTES;
	context->str_vendor = V4L2_REQUEST_STR_DRIVER_VENDOR;

	vtable->vaTerminate = v4l2_request_DataTerminate;
	vtable->vaQueryConfigEntrypoints = RequestQueryConfigEntrypoints;
	vtable->vaQueryConfigProfiles = RequestQueryConfigProfiles;
	vtable->vaQueryConfigAttributes = RequestQueryConfigAttributes;
	vtable->vaCreateConfig = RequestCreateConfig;
	vtable->vaDestroyConfig = RequestDestroyConfig;
	vtable->vaGetConfigAttributes = RequestGetConfigAttributes;
	vtable->vaCreateSurfaces = RequestCreateSurfaces;
	vtable->vaCreateSurfaces2 = RequestCreateSurfaces2;
	vtable->vaDestroySurfaces = RequestDestroySurfaces;
	vtable->vaExportSurfaceHandle = RequestExportSurfaceHandle;
	vtable->vaCreateContext = RequestCreateContext;
	vtable->vaDestroyContext = RequestDestroyContext;
	vtable->vaCreateBuffer = RequestCreateBuffer;
	vtable->vaBufferSetNumElements = RequestBufferSetNumElements;
	vtable->vaMapBuffer = RequestMapBuffer;
	vtable->vaUnmapBuffer = RequestUnmapBuffer;
	vtable->vaDestroyBuffer = RequestDestroyBuffer;
	vtable->vaBufferInfo = RequestBufferInfo;
	vtable->vaAcquireBufferHandle = RequestAcquireBufferHandle;
	vtable->vaReleaseBufferHandle = RequestReleaseBufferHandle;
	vtable->vaBeginPicture = RequestBeginPicture;
	vtable->vaRenderPicture = RequestRenderPicture;
	vtable->vaEndPicture = RequestEndPicture;
	vtable->vaSyncSurface = RequestSyncSurface;
	//vtable->vaGetSurfaceAttributes = RequestGetSurfaceAttributes;
	vtable->vaQuerySurfaceAttributes = RequestQuerySurfaceAttributes;
	vtable->vaQuerySurfaceStatus = RequestQuerySurfaceStatus;
	vtable->vaPutSurface = RequestPutSurface;
	vtable->vaQueryImageFormats = RequestQueryImageFormats;
	vtable->vaCreateImage = RequestCreateImage;
	vtable->vaDeriveImage = RequestDeriveImage;
	vtable->vaDestroyImage = RequestDestroyImage;
	vtable->vaSetImagePalette = RequestSetImagePalette;
	vtable->vaGetImage = RequestGetImage;
	vtable->vaPutImage = RequestPutImage;
	vtable->vaQuerySubpictureFormats = RequestQuerySubpictureFormats;
	vtable->vaCreateSubpicture = RequestCreateSubpicture;
	vtable->vaDestroySubpicture = RequestDestroySubpicture;
	vtable->vaSetSubpictureImage = RequestSetSubpictureImage;
	vtable->vaSetSubpictureChromakey = RequestSetSubpictureChromakey;
	vtable->vaSetSubpictureGlobalAlpha = RequestSetSubpictureGlobalAlpha;
	vtable->vaAssociateSubpicture = RequestAssociateSubpicture;
	vtable->vaDeassociateSubpicture = RequestDeassociateSubpicture;
	vtable->vaQueryDisplayAttributes = RequestQueryDisplayAttributes;
	vtable->vaGetDisplayAttributes = RequestGetDisplayAttributes;
	vtable->vaSetDisplayAttributes = RequestSetDisplayAttributes;
	vtable->vaLockSurface = RequestLockSurface;
	vtable->vaUnlockSurface = RequestUnlockSurface;

	/*
	vtable_vpp->vaQueryVideoProcFilters = vaQueryVideoProcFilters;
	vtable_vpp->vaQueryVideoProcFilterCaps = vaQueryVideoProcFilterCaps;
	vtable_vpp->vaQueryVideoProcPipelineCaps = vaQueryVideoProcPipelineCaps;
	*/

	/* initialize driver data structure */
	v4l2_request = (struct v4l2_request_data *)calloc(1, sizeof(*v4l2_request));

	if (v4l2_request == NULL) {
		context->pDriverData = NULL;

		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	}

	v4l2_request->wrapper_pdrvcontext = NULL;
	context->pDriverData = (void *)v4l2_request;
	rc = v4l2_request_DataInit(context);

	if (rc == VA_STATUS_SUCCESS) {
		context->str_vendor = v4l2_request->va_vendor;
	} else {
		free(v4l2_request);
		context->pDriverData = NULL;
	}

	/* initialize driver and decoder structures */
	driver = calloc(1, sizeof(struct driver));
	decoder = calloc(1, sizeof(struct decoder));

	rc = v4l2_request_DriverInit(context, driver);
	if (rc < 0 ) {
		driver_free(driver);

		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	}

	if (driver_get_num_decoders(driver) > 0) {
		driver_print_all(context, driver);
		/*
		 * first decoder id: assign its elements
		 * to the corresponding config elements
		 */
		decoder = driver_get(context, driver, 0);
		v4l2_request_log_info(context, "Decoders: %i (Select: '%s', media_path: %s, video_path: %s, capabilities: %ld)\n",
			    driver->num_decoders,
			    decoder->name,
			    decoder->media_path,
			    decoder->video_path,
			    decoder->capabilities);
	}
	else {
		v4l2_request_log_error(context, "No suitable v4l2 decoder found.\n");
		status = VA_STATUS_ERROR_INVALID_CONFIG;
		goto error;
	}

	driver_print(driver);

	// open media pipeline
	video_fd = open(video_path, O_RDWR | O_NONBLOCK);
	if (video_fd < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	media_fd = open(decoder->media_path, O_RDWR | O_NONBLOCK);
	if (media_fd < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	v4l2_request->video_fd = video_fd;
	v4l2_request->media_fd = media_fd;

	driver_free(driver);

	status = VA_STATUS_SUCCESS;

	goto complete;

error:
	if (video_fd >= 0)
		close(video_fd);

	if (media_fd >= 0)
		close(media_fd);

complete:
	return status;
}
