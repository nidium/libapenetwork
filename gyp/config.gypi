{
    'variables' : {
        'native_output%': '../build/',
        # Hack to workaround two gyp issues : 
        # - Variables defined in command line are not relativized (at all)
        #   https://code.google.com/p/gyp/issues/detail?id=72
        #
        # - Variables named with a "%" at the end are not relativized
        #   https://code.google.com/p/gyp/issues/detail?id=444
        'variables': {
            'third_party%': '../third-party'
        },
        'third_party_path': '<(DEPTH)/<(third_party)',
        'asan%': 0,
    },
}
