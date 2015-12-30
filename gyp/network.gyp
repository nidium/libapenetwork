{
    'targets': [{
        'target_name': 'nativenetwork-includes',
        'type': 'none',
        'direct_dependent_settings': {
            'include_dirs': [
                '../',
                '<(third_party_path)/c-ares/',
                '<(third_party_path)/openssl/include/',
            ],
            'defines': [
                '_HAVE_SSL_SUPPORT',
#                'USE_SPECIFIC_HANDLER',
#                'USE_SELECT_HANDLER'
            ],
        },
    }, {
        'target_name': 'nativenetwork-link',
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
        },
    }, {
        'target_name': 'nativenetwork',
        'type': 'static_library',
        'dependencies': [
            'network.gyp:nativenetwork-includes',
        ],
        'cflags': [
            #'-fvisibility=hidden',
        ],
        'sources': [
            '../native_netlib.c',
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
