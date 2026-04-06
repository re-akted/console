project('rconsole', 'c',
  version: '0.0.1',
  default_options: [
    'c_std=c17',
    'warning_level=3',
  ],
)

rconsole_inc = include_directories('include')

rconsole_lib = static_library('rconsole',
  files(
    'src/cvar.c',
    'src/console.c',
  ),
  include_directories: rconsole_inc,
)

rconsole_dep = declare_dependency(
  include_directories: rconsole_inc,
  link_with: rconsole_lib,
)

meson.override_dependency('rconsole', rconsole_dep)

test_rconsole = executable('test_rconsole',
  'tests/test_rconsole.c',
  dependencies: rconsole_dep,
)

test('rconsole', test_rconsole)
