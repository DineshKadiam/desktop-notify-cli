project('fyi', 'c',
        version: '1.0.0',
        license: 'MIT',
        default_options: [
          'c_std=c11',
          'warning_level=1',
          'werror=true',
          'b_ndebug=if-release'])

env = find_program('env', native: true)
generate_version_sh = files('generate-version.sh')
version = custom_target(
  'generate_version',
  build_always_stale: true,
  output: 'version.h',
  command: [env, 'LC_ALL=C', generate_version_sh, meson.project_version(), '@CURRENT_SOURCE_DIR@', '@OUTPUT@'])

dbus = dependency('dbus-1')
executable('fyi', 'main.c', version, dependencies: [dbus], install: true)
