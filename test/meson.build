foreach testName : [
  # Order is important
  'memory',
  'text',
  'teju',
  'string_cursor',
  'string_builder',
  'math',
]
  t = executable(
    testName + '_test',
    testName + '_test.c',
    include_directories: '../include',
    dependencies: [
      libm,
    ],
    c_args: [
      '-Wno-padded',          # Do not care wasting memory
      '-Wno-unused-function', # Do not care if all functions not tested
      '-Wno-unused-result',   # Do not care not using any result (e.g. Printing string to console)
    ],
  )
  test(testName, t)
endforeach
