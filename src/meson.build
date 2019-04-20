# Copyright (C) 2019 Bootlin
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

autoconf_data = configuration_data()
autoconf_data.set('VA_DRIVER_INIT_FUNC', va_driver_init_func)
autoconf_data.set('V4L2_REQUEST_DRIVER_VERSION', v4l2_request_vaapi_driver_version)
autoconf_data.set('V4L2_REQUEST_DRIVER_MAJOR_VERSION', v4l2_request_vaapi_driver_major_version)
autoconf_data.set('V4L2_REQUEST_DRIVER_MINOR_VERSION', v4l2_request_vaapi_driver_minor_version)
autoconf_data.set('V4L2_REQUEST_DRIVER_MICRO_VERSION', v4l2_request_vaapi_driver_micro_version)
autoconf_data.set('V4L2_REQUEST_DRIVER_PRE_VERSION', v4l2_request_vaapi_driver_pre_version)
if WITH_WAYLAND
	autoconf_data.set10('HAVE_VA_WAYLAND', 1)
endif

if git.found()
	git_version = run_command(
		git, '--git-dir',
		join_paths(meson.source_root(), '.git'),
		'describe', '--tags')
	v4l2_request_driver_git_version = git_version.stdout().strip()
else
	v4l2_request_driver_git_version = v4l2_request_vaapi_driver_version
endif
autoconf_data.set('V4L2_REQUEST_DRIVER_GIT_VERSION', v4l2_request_driver_git_version)

autoconf_file = configure_file(
	input: 'mesonconfig.h.in',
	output: 'autoconfig.h',
	configuration: autoconf_data)

sources = [
	'request.c',
	'object_heap.c',
	'config.c',
	'surface.c',
	'context.c',
	'buffer.c',
	'picture.c',
	'subpicture.c',
	'image.c',
	'utils.c',
	'tiled_yuv.S',
	'video.c',
	'media.c',
	'v4l2.c',
	'mpeg2.c',
	'h264.c',
	'h265.c'
]

headers = [
	'request.h',
	'object_heap.h',
	'config.h',
	'surface.h',
	'context.h',
	'buffer.h',
	'picture.h',
	'subpicture.h',
	'image.h',
	'utils.h',
	'tiled_yuv.h',
	'video.h',
	'media.h',
	'v4l2.h',
	'mpeg2.h',
	'h264.h',
	'h265.h'
]

includes = [
	include_directories('../include')
]

cflags = [
	'-DVA_DRIVERS_PATH="' + driverdir + '"',
	#'-DHAVE_CONFIG_H',
	'-Wall',
	'-fvisibility=hidden'
]

shared_sources = [
	sources,
	headers,
	autoconf_file
]

shared_deps = [
	libva_dep,
	libdrm_dep,
	libudev_dep
]

shared_sources = [
	sources,
	headers,
	autoconf_file
]

libv4l2_request_drv_video = static_library('v4l2_request_drv_video',
	c_args: cflags,
	include_directories : includes,
	sources: shared_sources,
	dependencies: shared_deps)

v4l2_request_drv_video = library('v4l2_request_drv_video',
	name_prefix: '',
	install: true,
	install_dir: driverdir,
	soversion: '1.0.0',
	c_args: cflags,
	link_whole: libv4l2_request_drv_video,
	dependencies: shared_deps)
