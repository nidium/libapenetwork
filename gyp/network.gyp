# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'targets': [{
        'target_name': 'network-includes',
        'type': 'none',
        'direct_dependent_settings': {
            'include_dirs': [
                '../',
                '<(third_party_path)/c-ares/',
                '<(third_party_path)/openssl/include/',
                '<(third_party_path)/zlib/',
            ],

            'conditions': [
                ['OS=="win"', {
                    'include_dirs': [
                        '<(third_party_path)/openssl/inc32/',
                    ]
                }]
            ],

            'defines': [
                '_HAVE_SSL_SUPPORT',
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
                ['OS=="linux"', {
                    "link_settings": {
                        'libraries': [
                            '-lcares',
                            '-lssl',
                            '-lcrypto',
                            '-lm',
                            '-lz',
                        ]
                    }
                }],
                ['OS=="mac"', {
                    "link_settings": {
                        'libraries': [
                            'libssl.a',
                            'libcrypto.a',
                            'libz.a'
                        ]
                    }
                }],
                ['OS=="win"', {
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
            '../ape_netlib.c',
            '../ape_pool.c',
            '../ape_hash.c',
            '../ape_array.c',
            '../ape_buffer.c',
            '../ape_events.c',
            '../ape_event_kqueue.c',
            '../ape_event_epoll.c',
            '../ape_event_select.c',
            '../ape_events_loop.c',
            '../ape_socket.c',
            '../ape_dns.c',
            '../ape_timers_next.c',
            '../ape_base64.c',
            '../ape_websocket.c',
            '../ape_sha1.c',
            '../ape_ssl.c',
            '../lz4.c',
            '../ape_blowfish.c'
        ],
    }],
}
