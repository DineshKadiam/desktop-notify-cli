project('hedwig', 'c',
        version: '1.0.0',
        license: 'MIT',
        default_options: [
          'c_std=c11',
          'warning_level=1',
          'werror=true',
          'b_ndebug=if-release'])

dbus = dependency('dbus-1')
executable('hedwig', 'main.c', dependencies: [dbus])
