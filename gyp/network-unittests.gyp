# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'targets': [
    {
        'target_name': 'unittests-settings',
        'type': 'none',
        'direct_dependent_settings': {
            'include_dirs': [
                 '<(third_party_path)/gtest/googletest/include',
             ],
            'conditions': [
                ['OS=="linux"', {
                    "link_settings": {
                        'libraries': [
                            '-lgtest_main',
                            '-lgtest',
                            '-ldl',
                            '-lpthread',
                        ]
                    }
                }],
                ['OS=="mac"', {
                    "link_settings": {
                        'libraries': [
                            'libgtest_main.a',
                            'libgtest.a',
                        ]
                    }
                }]
            ],
        },
    },
    {
        'target_name': 'nativenetwork-unittests',
        'type': 'executable',
        'product_dir': '<(native_tests_output)',
        'dependencies': [
            'network-unittests.gyp:unittests-settings',
            'network.gyp:*',
        ],
        'sources': [
            '../tests/unittest_0.cpp',
            '../tests/unittest_common.cpp',
            '../tests/unittest_base64.cpp',
            '../tests/unittest_blowfish.cpp',
            '../tests/unittest_sha1.cpp',
            '../tests/unittest_buffer.cpp',
            '../tests/unittest_pool.cpp',
            '../tests/unittest_array.cpp',
            '../tests/unittest_hash.cpp',
            #'../tests/unittest_netlib.cpp',
            #'../tests/unittest_events.cpp',
            #'../tests/unittest_timersng.cpp',
            #'../tests/unittest_socket.cpp',
            #'../tests/unittest_dns.cpp',
            '../tests/unittest_ssl.cpp',
            '../tests/unittest_websocket.cpp',
        ],
    },
    {
        'target_name': 'nativenetwork_benchmark_new_pool',
        'type': 'executable',
        'product_dir': '<(native_tests_output)',
        'dependencies': [
            'network-unittests.gyp:unittests-settings',
            'network.gyp:*',
        ],
        'sources': [
            '../tests/benchmark_new_pool.c',
        ],
    },
    {
        'target_name': 'nativenetwork_benchmark_timers_next',
        'type': 'executable',
        'product_dir': '<(native_tests_output)',
        'dependencies': [
            'network-unittests.gyp:unittests-settings',
            'network.gyp:*',
        ],
        'sources': [
            '../tests/benchmark_timers_next.c',
        ],
    }]
}

