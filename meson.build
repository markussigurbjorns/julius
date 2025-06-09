project(
  'julius',
  'c',
  version: '0.1',
  default_options: ['warning_level=3'],
)

cc = meson.get_compiler('c')

add_project_arguments('-w', language: 'c')
# ------------------------------ Configurations --------------------
#
# Configuration defines
common_c_args = [
  '-DLV_CONF_INCLUDE_SIMPLE=1',
]

use_sdl = get_option('use_sdl')
use_drm = get_option('use_drm')

if use_sdl
  sdl2_dep = dependency('sdl2', required: true)
  common_c_args += ['-DJULIUS_USE_SDL']
endif

if use_drm
  drm_dep = dependency('libdrm', required: true)
  common_c_args += ['-DJULIUS_USE_DRM']
endif

conf_data = configuration_data()
conf_data.set('LV_USE_SDL', use_sdl ? 1 : 0)
conf_data.set('LV_USE_DRM', use_drm ? 1 : 0)
conf_data.set('LV_USE_EVDEV', use_drm ? 1 : 0)

configure_file(
  input: 'lv_conf.h.in',
  output: 'lv_conf.h',
  configuration: conf_data,
)

# ---------------------------- LVGL --------------------------------

# Include directories - add core LVGL directories first
lvgl_includes = include_directories(
  '.',
  'lvgl',
  'lvgl/src',
  'lvgl/src/core',
  'lvgl/src/misc',
  'lvgl/src/drivers/sdl',
)

# Get source files
lvgl_src = run_command('find', 'lvgl/src', '-type', 'f', '-name', '*.c', check: true).stdout().strip().split('\n')

# Build LVGL core
lvgl_lib = static_library(
  'lvgl',
  lvgl_src,
  include_directories: lvgl_includes,
  c_args: common_c_args,
  dependencies: use_drm ? [drm_dep] : [],
)

lvgl_dep = declare_dependency(link_with: lvgl_lib, include_directories: lvgl_includes)

# ------------------------- sofia-sip -------------------------

sofia_sip_dep = dependency('sofia-sip-ua', required: true)

# ------------------------- sofia-sip-glib -------------------------

sofia_sip_glib_includes = include_directories(
  'sofia-sip-1.13.17',
  'sofia-sip-1.13.17/libsofia-sip-ua-glib/su-glib',
)

sofia_sip_glib_dep = declare_dependency(
  include_directories: sofia_sip_glib_includes,
  dependencies: sofia_sip_dep,
  link_args: [
    '-L'
    + meson.project_source_root()
    + '/sofia-sip-1.13.17/libsofia-sip-ua-glib/.libs',
    '-lsofia-sip-ua-glib',
  ],
)
# ------------------------- gstreamer ------------------------------------

gst_dep = dependency('gstreamer-1.0', required: true)
gst_base_dep = dependency('gstreamer-base-1.0', fallback: ['gstreamer', 'gst_base_dep'])

# --------------------------- executable ---------------------------------

glib_dep = dependency('glib-2.0', required: true)

src_files = files(
  'gui.c',
  'main.c',
  'network.c',
  'ua.c',
)

deps = [
  gst_dep,
  gst_base_dep,
  sofia_sip_dep,
  sofia_sip_glib_dep,
  glib_dep,
  lvgl_dep,
]

if use_sdl
  deps += sdl2_dep
endif

if use_drm
  deps += drm_dep
endif

executable(
  'main',
  src_files,
  dependencies: deps,
  c_args: common_c_args,
)
