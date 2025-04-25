project(
  'stdlib',
  'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3', # -Wall -Wextra -Wpedantic
    'werror=true',     # -Werror
  ],
)

cc = meson.get_compiler('c')
is_build_debug = get_option('buildtype').startswith('debug')

is_compiler_gcc   = (cc.get_id() == 'gcc'  )
is_compiler_clang = (cc.get_id() == 'clang')
is_compiler_msvc  = (cc.get_id() == 'msvc' )

is_platform_linux   = (build_machine.system() == 'linux'  )
is_platform_windows = (build_machine.system() == 'windows')
is_platform_darwin  = (build_machine.system() == 'darwin' )
is_platform_freebsd = (build_machine.system() == 'freebsd')

add_project_arguments(
  [
    '-Wshadow' + (is_compiler_gcc ? '=local': ''), # Allow shadowing global declaration
    '-Wconversion', # Explicitly convert one type from another

    # Fixes -Weverything
    '-Wno-declaration-after-statement', # I am not using <C99
    '-Wno-reserved-macro-identifier',   # Do not care if I override some compiler macros
    '-Wno-extra-semi-stmt',             # assert uses this
    '-Wno-unsafe-buffer-usage',         # I know what I am doing! (famous last words)
    '-Wno-float-equal',                 # If you do not know floats have precision errors, why are you a programmer

    '-DIS_BUILD_DEBUG=' + is_build_debug.to_int().to_string(),

    '-DIS_COMPILER_GCC='   + is_compiler_gcc.to_int().to_string(),
    '-DIS_COMPILER_CLANG=' + is_compiler_clang.to_int().to_string(),
    '-DIS_COMPILER_MSVC='  + is_compiler_msvc.to_int().to_string(),

    '-DIS_PLATFORM_LINUX='   + is_platform_linux.to_int().to_string(),
    '-DIS_PLATFORM_WINDOWS=' + is_platform_windows.to_int().to_string(),
    '-DIS_PLATFORM_DARWIN='  + is_platform_darwin.to_int().to_string(),
    '-DIS_PLATFORM_FREEBSD=' + is_platform_freebsd.to_int().to_string(),
  ],
  language: 'c'
)

if is_build_debug
  add_project_arguments(
    [
      # Prototype in debug builds
      '-Wno-unused-function',
      '-Wno-unused-parameter',
      '-Wno-unused-result',
      '-Wno-unused-const-variable',
    ],
    language: 'c'
  )
else
  add_project_arguments(
    [
      '-Wpadded', # Warn about padding in structures
    ],
    language: 'c'
  )
endif

libm = cc.find_library('m')

if get_option('test')
  subdir('test')
endif
