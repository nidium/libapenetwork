{
    'variables' : {
        'native_output%': '../out/',
        'native_output_third_party%': '../out/third-party-libs/release/',

        # Hack to workaround two gyp issues : 
        # - Variables defined in command line are not relativized (at all)
        #   https://code.google.com/p/gyp/issues/detail?id=72
        #
        # - Variables named with a "%" at the end are not relativized
        #   https://code.google.com/p/gyp/issues/detail?id=444
        'variables': {
            'third_party%': 'third-party'
        },
        'third_party_path': '<(DEPTH)/<(third_party)',

        'target_os%': '<(OS)',
        'mac_deployment_target': '10.7',
        'mac_sdk_version%': '10.11',

        'asan%': 0,
        'profiler%': 0,
    },
}
