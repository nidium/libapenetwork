# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

{
    'target_defaults': {
        'default_configuration': 'Release',
        'cflags': [
           #'-fvisibility=hidden',
            '-Wall',
        ],
        'cflags_cc': [
            '-std=c++11'
        ],
        'ldflags': [
            '-L<(libapenetwork_output_third_party_path)',
        ],
        'target_conditions': [
            ['_type=="static_library"', {
                'standalone_static_library': 1, # disable thin archive
            }],
        ],
        'msvs_configuration_platform': '<(COMPILER_PLATFORM)',
        'msvs_settings': {
            'VCLinkerTool': {
                'LinkTimeCodeGeneration': 1,
                'SubSystem': '1',  # console app
                "AdditionalLibraryDirectories": [
                    "../../../build/third-party", #<--FIXME, it is not working
                    "<(libapenetwork_output_third_party_path)",
                ]
            }
        },
        'xcode_settings': {
            "OTHER_LDFLAGS": [
                '-stdlib=libc++',
                '-L<(libapenetwork_output_third_party_path)',
                '-F<(libapenetwork_output_third_party_path)',
            ],
            "SYMROOT": "<(libapenetwork_output_path)",
            'OTHER_CPLUSPLUSFLAGS': [ 
                '-std=c++11',
                '-stdlib=libc++'
            ],
            'ARCHS': [
                '<(COMPILER_PLATFORM)',
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
        ]
    },
}
