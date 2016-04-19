# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'target_defaults': {
        'default_configuration': 'Release',
        'conditions': [
            ['target_os=="android"', {
                'defines': ['__ANDROID__', 'ANDROID'],
            }],
            ['target_os=="win"', {
                'msvs_configuration_platform': 'x64'
            }]
        ],
        'msvs_configuration_platform': 'x64',
        'cflags': [
           #'-fvisibility=hidden',
            '-Wall',
        ],
        'ldflags': [
            '-L<(native_output_third_party)',
        ],
        'msvs_settings': {
            'VCLinkerTool': {
                'LinkTimeCodeGeneration': 1,
                'SubSystem': '1',  # console app
                "AdditionalLibraryDirectories": ["<(native_output_third_party)"]
            }               
        },
        'xcode_settings': {
            "OTHER_LDFLAGS": [
                '-L<(native_output_third_party)',
                '-F<(native_output_third_party)',
            ],
            'ARCHS': [
                'x86_64',
            ],
            'MACOSX_DEPLOYMENT_TARGET': [
                '<(mac_deployment_target)'
            ],
            'SDKROOT': [
                'macosx<(mac_sdk_version)'
            ],
        },
        'configurations': {
            'Debug': {
                'cflags': [
                    '-O0',
                    '-g',
                ],
                'xcode_settings': {
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
                ],
                'xcode_settings': {
                    'OTHER_CFLAGS': [ 
                        '-g',
                        '-O2',
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
