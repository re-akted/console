project('rconsole', 'c',
  version: '0.0.1',
  default_options: [
    'c_std=c17',
    'warning_level=3',
  ],
)

rconsole_inc = include_directories('include')

rconsole_sources = files(
  'src/cvar.c',
  'src/console.c',
)

rconsole_static = static_library('rconsole', rconsole_sources,
  include_directories: rconsole_inc,
)

rconsole_shared = shared_library('rconsole', rconsole_sources,
  include_directories: rconsole_inc,
  install: true,
)

rconsole_dep = declare_dependency(
  include_directories: rconsole_inc,
  link_with: rconsole_static,
)

meson.override_dependency('rconsole', rconsole_dep)

install_subdir('include/redakt', install_dir: get_option('includedir'))

test_rconsole = executable('test_rconsole',
  'tests/test_rconsole.c',
  dependencies: rconsole_dep,
)

test('rconsole', test_rconsole)
