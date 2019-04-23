/*
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <libudev.h>
#include <stdbool.h>
#include <linux/media.h>
#include <va/va_backend.h>

#include "autoconfig.h"
#include "drm_fourcc.h"

#define DRIVER_LAST	-1

#define VA_V4L2_REQUEST_DEBUG_OPTION_ASSERT    (1 << 0)

#define v4l2_request_assert_rc(value, fail_rc) do {	\
	if (!(value)) {                     \
		if (g_v4l2_request_debug_option_flags & VA_V4L2_REQUEST_DEBUG_OPTION_ASSERT) \
			assert(value);      \
		return fail_rc;		    \
	}                                   \
  } while (0)

extern uint32_t g_v4l2_request_debug_option_flags;

/*
 * Structures
 */

struct decoder {
	int id;
	char *name;
	char *media_path;
	char *video_path;
	unsigned int capabilities;
};

struct driver {
	int capacity;
	void **decoder;
	int num_decoders;
};

/*
 * Functions
 */

/* v4l2 driver */
int driver_add(struct driver *driver, struct decoder *decoder);
void driver_delete(struct driver *driver, int id);
int driver_extend(struct driver *driver);
void driver_free(struct driver *driver);
struct decoder *driver_get(VADriverContextP context, struct driver *driver, int id);
__u32 driver_get_capabilities(VADriverContextP context, struct driver *driver, int id);
int driver_get_first_id(struct driver *driver);
int driver_get_last_id(struct driver *driver);
int driver_get_num_decoders(struct driver *driver);
void driver_init(struct driver *driver);
void driver_print(VADriverContextP context, struct driver *driver, int id);
void driver_print_all(VADriverContextP context, struct driver *driver);
void driver_reduce(struct driver *driver);
void driver_set(struct driver *driver, int id, void *decoder);
__u32 driver_set_capabilities(VADriverContextP context, struct driver *driver, int id, unsigned int capabilities);

/* udev handling */
char *udev_get_devpath(VADriverContextP context, struct media_v2_intf_devnode *devnode);
int udev_scan_subsystem(VADriverContextP context, struct driver *driver, char *subsystem);

/* v4l2 request */
//bool v4l2_request_ensure_vendor_string(struct v4l2_request_data *v4l2_request);
void v4l2_request_log_error(VADriverContextP context, const char *format, ...);
void v4l2_request_log_info(VADriverContextP context, const char *format, ...);
void request_log(const char *format, ...);

#endif
