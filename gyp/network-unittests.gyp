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
                ['OS=="win"', {
                    "link_settings": {
                        'libraries': [
                            '-lgtest_main',
                            '-lgtest',
                            '-lpthreadVCE2',

                            '-lshlwapi',
                        ]
                    }
                }],
                ['OS=="linux"', {
                    "link_settings": {
                        'libraries': [
                            '-lgtest_main',
                            '-lgtest',
                            '-ldl',
                            '-lpthread',
                            '-lrt'
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
        'target_name': 'network-unittests',
        'type': 'executable',
        # When built from a different directory the current 
        # working directory of gyp is different between 
        # OSX and Linux. Workaround that.
        'conditions': [
            ['OS=="linux"', {
                'product_dir': '../build/tests/'
            }, {
            ['OS=="mac"', {
                'product_dir': '<(libapenetwork_tests_output_path)',
                "xcode_settings": {
                    'OTHER_LDFLAGS!': [
                        '../build/third-party/libssl.a',
                        '../build/third-party/libcrypto.a',
                    ],
                    'OTHER_LDFLAGS': [
                        '<(DEPTH)/build/third-party/libssl.a',
                        '<(DEPTH)/build/third-party/libcrypto.a',
                    ],
                },
            }, {
	    	# Windows
                'product_dir': '<(libapenetwork_tests_output_path)',
            }]
        ],
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
            '../tests/unittest_netlib.cpp',
            '../tests/unittest_events.cpp',
            '../tests/unittest_timersng.cpp',
            '../tests/unittest_socket.cpp',
            '../tests/unittest_dns.cpp',
            '../tests/unittest_ssl.cpp',
            '../tests/unittest_websocket.cpp',
            '../tests/unittest_log.cpp',
        ],
    }]
}

