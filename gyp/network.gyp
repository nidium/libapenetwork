# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'targets': [{
        'target_name': 'network-includes',
        'type': 'none',
        'direct_dependent_settings': {
            'include_dirs': [
                '../src/',
                '<(third_party_path)/c-ares/',
                '<(third_party_path)/openssl/include/',
                '<(third_party_path)/zlib/',
            ],

            'conditions': [
                ['target_os=="win"', {
                    'include_dirs': [
                        '<(third_party_path)/openssl/inc32/',
                    ]
                }]
            ],

            'defines': [
                '_HAVE_SSL_SUPPORT',
                'OPENSSL_API_COMPAT=0x10100000L',
                'CARES_STATICLIB',
                'FD_SETSIZE=2048'
#                'USE_SPECIFIC_HANDLER',
#                'USE_SELECT_HANDLER'
            ],
        },
    }, {
        'target_name': 'network-link',
        'type': 'none',
        'direct_dependent_settings': {
            'conditions': [
                ['target_os=="linux" or target_os=="android"', {
                    "link_settings": {
                        'libraries': [
                            '-lcares',
                            '-lssl',
                            '-lcrypto',
                            '-lm',
                            '-lz',
                            '-lrt',
                        ]
                    }
                }],
                ['target_os=="mac"', {
                    "link_settings": {
                        'libraries': [
                            'libcares.a',
                            'libssl.a',
                            'libcrypto.a',
                            'libz.a'
                        ]
                    }
                }],
                ['target_os=="win"', {
                    "link_settings": {
                        'libraries': [
                            '-llibcares',
                            '-lssleay32',
                            '-llibeay32',
                            # GDI and User32 are required by openssl
                            '-lgdi32',
                            '-lUser32',
                            '-lWs2_32',
                            # Required by c-ares (RegClose...)
                            '-lAdvapi32'
                        ]
                    }
                }]
            ],
        },
    }, {
        'target_name': 'network',
        'type': 'static_library',
        'dependencies': [
            'network.gyp:network-includes',
        ],
        'cflags': [
            #'-fvisibility=hidden',
        ],
        'sources': [
            '../src/ape_netlib.c',
            '../src/ape_pool.c',
            '../src/ape_hash.c',
            '../src/ape_array.c',
            '../src/ape_buffer.c',
            '../src/ape_events.c',
            '../src/ape_event_kqueue.c',
            '../src/ape_event_epoll.c',
            '../src/ape_event_select.c',
            '../src/ape_events_loop.c',
            '../src/ape_socket.c',
            '../src/ape_dns.c',
            '../src/ape_timers_next.c',
            '../src/ape_base64.c',
            '../src/ape_websocket.c',
            '../src/ape_sha1.c',
            '../src/ape_ssl.c',
            '../src/ape_lz4.c',
            '../src/ape_blowfish.c'
        ],
    }],
}
