c_args = []
dependencies = []

if is_platform_linux
  # see stat.2, lstat()
  required_glibc_major = 2
  required_glibc_minor = 10
  glibc_major = cc.get_define('__GLIBC__', prefix: '#include <features.h>')
  glibc_minor = cc.get_define('__GLIBC_MINOR__', prefix: '#include <features.h>')
  if glibc_major == '' or glibc_minor == ''
    error(f'glibc not detected. This project requires glibc ≥ @required_glibc_major@.@required_glibc_minor@')
  endif
  if glibc_major.to_int() < required_glibc_major or glibc_minor.to_int() < required_glibc_minor
    error(f'glibc version is too low. Upgrade to glibc ≥ @required_glibc_major@.@required_glibc_minor@')
  endif
endif

if not is_platform_windows
  c_args += [ '-D_POSIX_C_SOURCE=200112L' ]
endif

if is_platform_windows
  lib_bcrypt = cc.find_library('bcrypt')
  dependencies += lib_bcrypt
endif

executable(
  'gen_pseudo_random',
  'gen_pseudo_random.c',
  dependencies: dependencies,
  c_args: c_args,
  include_directories: '../include',
)
