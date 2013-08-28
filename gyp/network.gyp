{
    'targets': [{
        'target_name': 'nativenetwork',
        'type': 'static_library',
        'include_dirs': [
            '<(DEPTH)/<(third_party_path)/c-ares/',
            '../network/',
            '../',
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
            }]
        ],
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
            '../ape_base64.c'
        ],
    }],
}
