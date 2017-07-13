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
        'msvs_configuration_platform': 'x64',
        'msvs_settings': {
            'VCLinkerTool': {
                'LinkTimeCodeGeneration': 1,
                'SubSystem': '1',  # console app
                "AdditionalLibraryDirectories": ["<(libapenetwork_output_third_party_path)"]
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
            "ARCHS": "$(ARCHS_STANDARD_64_BIT)",
            'MACOSX_DEPLOYMENT_TARGET': [
                '<(mac_deployment_target)'
            ],
            'SDKROOT': [
                '<(mac_sdk_sysroot)'
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
            ["target_os=='ios'", {
                "xcode_settings": {
                    "IPHONEOS_DEPLOYEMENT_TARGET": "9.2",
                    "TARGETED_DEVICE_FAMILY": "1,2", # iPhone / iPad
                    'OTHER_CFLAGS': [
                        '-fembed-bitcode'
                    ]
                },
                "defines": ["NDM_TARGET_IOS"]
            }],
            ["target_os=='tvos'", {
                "xcode_settings": {
                    "TVOS_DEPLOYEMENT_TARGET": "9.2",
                    'OTHER_CFLAGS': [
                        '-fembed-bitcode'
                    ]
                },
                "defines": ["NDM_TARGET_TVOS"],
            }],
        ]
    },
}
