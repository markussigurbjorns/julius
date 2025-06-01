project(
  'julius',
  'c',
  version: '0.1',
  default_options: ['warning_level=3'],
)

cc = meson.get_compiler('c')

# ------------------------------------------------------------------ LVGL
sdl2_dep = dependency('sdl2')

# Include directories - add core LVGL directories first
includes = include_directories(
  '.',
  'lvgl',
  'lvgl/src',
  'lvgl/src/core',
  'lvgl/src/misc',
  'lvgl/src/drivers/sdl',
)

# Configuration defines
common_c_args = [
  '-DLV_CONF_INCLUDE_SIMPLE=1',
]

# Get source files
lvgl_src = run_command('find', 'lvgl/src', '-type', 'f', '-name', '*.c', check: true).stdout().strip().split('\n')
lv_drivers_src = run_command('find', 'lv_drivers', '-type', 'f', '-name', '*.c', check: true).stdout().strip().split('\n')

# Build LVGL core
lvgl_lib = static_library(
  'lvgl',
  lvgl_src,
  include_directories: includes,
  c_args: common_c_args,
  dependencies: sdl2_dep,
)

# Combined dependency
lvgl_dep = declare_dependency(link_with: [lvgl_lib], include_directories: includes)
# ------------------------------------------------------------------ LVGL END

gst_dep = dependency('gstreamer-1.0', required: true)
gst_base_dep = dependency('gstreamer-base-1.0', fallback: ['gstreamer', 'gst_base_dep'])

gtk_dep = dependency('gtk4', required: true)

sofia_sip_dep = dependency('sofia-sip-ua', required: true)
sofia_sip_glib_dep = dependency('sofia-sip-ua-glib', required: true)

glib_dep = dependency('glib-2.0', required: true)

src_files = files(
  'gui.c',
  'main.c',
  'network.c',
  'ua.c',
)

executable(
  'main',
  src_files,
  dependencies: [
    gst_dep,
    gst_base_dep,
    gtk_dep,
    sofia_sip_dep,
    sofia_sip_glib_dep,
    glib_dep,
    lvgl_dep,
    sdl2_dep,
  ],
)
