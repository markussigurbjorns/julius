project(
  'julius',
  'c',
  version: '0.1',
  default_options: ['warning_level=3'],
)

cc = meson.get_compiler('c')

raylib = cc.find_library('raylib', required: true)
pipewire_dep = dependency('libpipewire-0.3')
math_lib = cc.find_library('m', required: true)

src_files = files(
  'main.c',
)

executable(
  'julius',
  src_files,
  dependencies: [raylib, pipewire_dep, math_lib],
)
