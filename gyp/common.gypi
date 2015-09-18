{
    'target_defaults': {
        'default_configuration': 'Release',
        'configurations': {
            'Debug': {
                'cflags': [
                    '-O0',
                    '-g',
                ],
                'ldflags': [
                    '-L<(native_output_third_party)',
                ],
                'xcode_settings': {
                    "OTHER_LDFLAGS": [
                        '-L<(native_output_third_party)',
                        '-F<(native_output_third_party)',
                    ],
                    'ARCHS': [
                        'x86_64',
                    ],
                    'MACOSX_DEPLOYMENT_TARGET': [
                        '10.7'
                    ],
                    'SDKROOT': [
                        'macosx10.11'
                    ],
                    'OTHER_CPLUSPLUSFLAGS': [ 
		        '-stdlib=libc++',
                    ],
                    'OTHER_CFLAGS': [ 
                        '-g',
                        '-O0'
                    ]
                }
            },
            'Release': {
                'defines': ['NDEBUG'],
                'cflags': [
                    '-g',
                    '-O2',
                    '-Wall',
                ],
                'ldflags': [
                    '-L<(native_output_third_party)',
                ],
                'xcode_settings': {
                    "OTHER_LDFLAGS": [
                        '-L<(native_output_third_party)',
                        '-F<(native_output_third_party)',
                    ],
                    'ARCHS': [
                        'x86_64',
                    ],
                    'OTHER_CPLUSPLUSFLAGS': [ 
                        '-stdlib=libc++',
                        '-g',
                        '-O2',
                        '-Wall',
                        '-Wno-invalid-offsetof'
                    ],
                    'MACOSX_DEPLOYMENT_TARGET': [
                        '10.7'
                    ],
                    'SDKROOT': [
                        'macosx10.9'
                    ],
                    'OTHER_CFLAGS': [ 
                        '-g',
                        '-O2',
                        '-Wall',
                        '-Wno-invalid-offsetof'
                    ]
                },
            }
        },
        'conditions': [
            ['asan==1', {
                'cflags': [
                    '-fsanitize=address'
                ],
                'ldflags': [
                    '-fsanitize=address'
                ],
                'xcode_settings': {
                    "OTHER_LDFLAGS": [
                        '-fsanitize=address'
                    ],
                    'OTHER_CFLAGS': [ 
                        '-fsanitize=address'
                    ]
                }
            }],
            ['profiler==1', {
                'ldflags': [
                    '-lprofiler'
                ],
                'xcode_settings': {
                    "OTHER_LDFLAGS": [
                        '-fsanitize=address'
                    ],
                }
            }]
        ],
    },
}
