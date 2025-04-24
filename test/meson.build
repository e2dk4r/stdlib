foreach testName : [
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
      # '-Wno-padded',
    ],
  )
  test(testName, t)
endforeach
