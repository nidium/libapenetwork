{
    'targets': [{
        'target_name': 'nativenetwork',
        'type': 'static_library',
        'includes': [
            'common.gypi',
            'config.gypi'
        ],
        'include_dirs': [
            '<(third_party_path)/c-ares/',
            '<(third_party_path)/openssl/include/',
            '../',
        ],
        'defines': [
            '_HAVE_SSL_SUPPORT'
        ],
        'conditions': [
            ['OS=="mac"', {
				'xcode_settings': {
					'OTHER_CFLAGS': [
                        '-fvisibility=hidden'
					],
				},
			}],
            ['OS=="linux"', {
                'cflags': [
                    '-fvisibility=hidden',
                ],
            }],
            ['target_os=="android"', {
                'defines': ['ANDROID'],
            }],
        ],
        'direct_dependent_settings': {
            'include_dirs': [
                '../',
                '<(third_party_path)/c-ares/',
                '<(third_party_path)/openssl/include/',
            ],
            'conditions': [
                ['OS=="linux"', {
                    "link_settings": {
                        'libraries': [
                            '-lssl',
                            '-lcrypto'
                        ]
                    }
                }],
                ['OS=="mac"', {
                    "link_settings": {
                        'libraries': [
                            'libssl.a',
                            'libcrypto.a'
                        ]
                    }
                }]
            ],
            'defines': [
                '_HAVE_SSL_SUPPORT'
            ]
        },
        'sources': [
            '../native_netlib.c',
            '../ape_pool.c',
            '../ape_hash.c',
            '../ape_http_parser.c',
            '../ape_array.c',
            '../ape_buffer.c',
            '../ape_events.c',
            '../ape_event_kqueue.c',
            '../ape_event_epoll.c',
            '../ape_event_select.c',
            '../ape_events_loop.c',
            '../ape_socket.c',
            '../ape_dns.c',
            '../ape_timers.c',
            '../ape_timers_next.c',
            '../ape_base64.c',
            '../ape_websocket.c',
            '../ape_sha1.c',
            '../ape_ssl.c'
        ],
    }],
}
