project(
  'stdlib',
  'c',
  default_options: [
    'c_std=c11',
    'warning_level=everything', # -Weverything
    'werror=true',              # -Werror
  ],
)

is_build_debug = get_option('buildtype').startswith('debug')
add_project_arguments(
  [
    '-Wno-declaration-after-statement', # I am not using <C99
    '-Wno-reserved-macro-identifier',   # Do not care if I override some compiler macros
    '-Wno-extra-semi-stmt',             # assert uses this
    '-Wno-unsafe-buffer-usage',         # I know what I am doing! (famous last words)
    '-Wno-float-equal',                 # If you do not know floats precision errors, why are you a programmer

    '-DIS_BUILD_DEBUG=' + is_build_debug.to_int().to_string(),

    '-DIS_PLATFORM_LINUX='   + (build_machine.system() == 'linux'  ).to_int().to_string(),
    '-DIS_PLATFORM_WINDOWS=' + (build_machine.system() == 'windows').to_int().to_string(),
    '-DIS_PLATFORM_DARWIN='  + (build_machine.system() == 'darwin' ).to_int().to_string(),
    '-DIS_PLATFORM_FREEBSD=' + (build_machine.system() == 'freebsd').to_int().to_string(),
  ],
  language: 'c'
)

if is_build_debug
  add_project_arguments(
    [
      '-Wno-unused-function',
    ],
    language: 'c'
  )
endif

cc = meson.get_compiler('c')
libm = cc.find_library('m')
subdir('test')
