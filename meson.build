project('fyi', 'c',
        version: '1.0.2',
        license: 'MIT',
        default_options: [
          'c_std=c11',
          'warning_level=1',
          'werror=true',
          'b_ndebug=if-release'])

is_debug_build = get_option('buildtype').startswith('debug')
cc = meson.get_compiler('c')

add_project_arguments(
  (is_debug_build
    ? ['-D_DEBUG']
   : [cc.get_supported_arguments('-fno-asynchronous-unwind-tables')]),
  language: 'c')

scdoc = dependency('scdoc', native: true, required: get_option('docs'))
if scdoc.found()
  install_data(
    'LICENSE',
    'README.md',
    'CHANGELOG.md',
    install_dir: join_paths(get_option('datadir'), 'doc', 'fyi'))
  subdir('doc')
endif

env = find_program('env', native: true)
generate_version_sh = files('generate-version.sh')
version = custom_target(
  'generate_version',
  build_always_stale: true,
  output: 'version.h',
  command: [env, 'LC_ALL=C', generate_version_sh, meson.project_version(), '@CURRENT_SOURCE_DIR@', '@OUTPUT@'])

dbus = dependency('dbus-1')
executable('fyi', 'main.c', version, dependencies: [dbus], install: true)

subdir('completions')
