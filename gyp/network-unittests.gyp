{
    'targets': [
    {
        'target_name': 'libnativenetwork_unittests',
        'type': 'executable',
        'include_dirs': [
                '../',
                '<(third_party_path)/gtest/include',
         ],
        'conditions': [
            ['OS=="linux"', {
                "link_settings": {
                    'ldflags': ['-L<(third_party_path)/gtest', '<(third_party_path)/gtest/libgtest_main.a'],
                    'libraries': [
                        '-lpthread',
                        '-ldl',
                        '-lz',
                        '-lrt',
                        '-lgtest',
                    ]
                }
            }],
            ['OS=="mac"', {
                "link_settings": {
                    'ldflags': ['-L<(third_party_path)/gtest', '<(third_party_path)/gtest/libgtest_main.a'],
                    'libraries': [
                        '-pthread',
                        '-dl',
                        '-z',
                        '-rt',
                        'libgtest.a',
                    ]
                }
            }]
        ],
        'dependencies': [
            'network.gyp:*',
        ],
        'cflags': [
            #'-fvisibility=hidden',
        ],
        'sources': [
            '../tests/unittest_0.cpp',
            '../tests/unittest_common.cpp',
            '../tests/unittest_array.cpp',
            '../tests/unittest_base64.cpp',
            '../tests/unittest_blowfish.cpp',
            '../tests/unittest_buffer.cpp',
            '../tests/unittest_hash.cpp',
            '../tests/unittest_dns.cpp',
            '../tests/unittest_events.cpp',
            '../tests/unittest_netlib.cpp',
            '../tests/unittest_pool.cpp',
            '../tests/unittest_sha1.cpp',
            '../tests/unittest_socket.cpp',
            '../tests/unittest_ssl.cpp',
            '../tests/unittest_timersng.cpp',
            '../tests/unittest_websocket.cpp',
        ],
    },
    {
        'target_name': 'libapenetwork_benchmark_new_pool',
        'type': 'executable',
        'include_dirs': [
                '../',
         ],
        'conditions': [
            ['OS=="linux"', {
                "link_settings": {
                    'libraries': [
                        '-lz',
                        '-lrt',
                    ]
                }
            }],
            ['OS=="mac"', {
                "link_settings": {
                    'libraries': [
                        '-z',
                        '-rt',
                    ]
                }
            }]
        ],
        'dependencies': [
            'network.gyp:*',
        ],
        'cflags': [
            #'-fvisibility=hidden',
        ],
        'sources': [
            '../tests/benchmark_new_pool.c',
        ],
    },
    {
        'target_name': 'libapenetwork_benchmark_timers_next',
        'type': 'executable',
        'include_dirs': [
                '../',
         ],
        'conditions': [
            ['OS=="linux"', {
                "link_settings": {
                    'libraries': [
                        '-lz',
                        '-lrt',
                        '-ldl',
                    ]
                }
            }],
            ['OS=="mac"', {
                "link_settings": {
                    'libraries': [
                        '-z',
                        '-rt',
                        '-dl',
                    ]
                }
            }]
        ],
        'dependencies': [
            'network.gyp:*',
        ],
        'cflags': [
            #'-fvisibility=hidden',
        ],
        'sources': [
            '../tests/benchmark_timers_next.c',
        ],
    }

    ]
}

