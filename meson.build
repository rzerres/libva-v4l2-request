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

###project('v4l2_request-vaapi-driver', 'c',
project('libva-v4l2-request', 'c',
	version: '1.0.0.1',
	meson_version: '>= 0.43.0',
	default_options : [ 'warning_level=1',
			    'buildtype=debugoptimized',
			    'default_library=shared',
])

v4l2_request_vaapi_driver_version = meson.project_version()
version_arr = meson.project_version().split('.')
v4l2_request_vaapi_driver_major_version = version_arr[0]
v4l2_request_vaapi_driver_minor_version = version_arr[1]
v4l2_request_vaapi_driver_micro_version = version_arr[2]
v4l2_request_vaapi_driver_version = '@0@.@1@.@2@'.format(v4l2_request_vaapi_driver_major_version,
						  v4l2_request_vaapi_driver_minor_version,
						  v4l2_request_vaapi_driver_micro_version)
if version_arr.length() == 4
  v4l2_request_vaapi_driver_pre_version = version_arr[3]
  v4l2_request_vaapi_driver_version = '@0@.pre@1@'.format(v4l2_request_vaapi_driver_version,
						   v4l2_request_vaapi_driver_pre_version)
endif

add_project_arguments(language : 'c')

add_project_arguments('-DMESON_BUILD', language : 'c')

cc = meson.get_compiler('c')

git = find_program('git', required : false)

thread_dep = dependency('threads')
libdrm_dep = dependency('libdrm',
	version : '>= 2.4.52')

libva_version = '>= 1.1.0'
libva_dep = dependency('libva',
	method: 'pkg-config',
	version: libva_version,
	fallback : [ 'libva', 'libva_dep' ],
	required: true)
if get_option ('enable_tests')
	libva_drm_dep = dependency('libva-drm',
		version : libva_version,
		fallback : [ 'libva', 'libva_drm_dep' ])
endif

libdrm_dep = dependency('libdrm',
	method: 'pkg-config',
	version : '>= 2.4.52',
	required: true)

libudev_dep = dependency('libudev',
	method: 'pkg-config',
	required: true)

driverdir = get_option('driverdir')
if libva_dep.type_name() == 'pkgconfig'
	if driverdir == ''
 		driverdir = libva_dep.get_pkgconfig_variable('driverdir')
	endif
	va_api_version_array = libva_dep.version().split('.')
	va_api_major_version = va_api_version_array[0]
	va_api_minor_version = va_api_version_array[1]
else
	libva = subproject('libva')
	if driverdir == ''
		driverdir = libva.get_variable('driverdir')
	endif
	va_api_major_version = libva.get_variable('va_api_major_version')
	va_api_minor_version = libva.get_variable('va_api_minor_version')
endif
va_driver_init_func = '__vaDriverInit_@0@_@1@'.format(va_api_major_version,
							  va_api_minor_version)

if driverdir == ''
	driverdir = '@0@/@1@/@2@'.format(get_option('prefix'), get_option('libdir'), 'dri')
endif

subdir('src')

if get_option('enable_tests')
	srcdir = include_directories('src')
	subdir('test')
endif
