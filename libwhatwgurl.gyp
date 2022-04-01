{
  "target_defaults": {
    "default_configuration": "Release",
    "configurations": {
      "Release": {
        "cflags": [ "-O3" ]
      },
      "Debug": {
        "defines": [ "DEBUG", "_DEBUG" ],
        "cflags": [ "-g", "-O0" ]
      }
    },
    "conditions": [
      [["os=='macos'"], {
        "xcode_settings": {
          "CLANG_CXX_LANGUAGE_STANDARD": "gnu++1y"
        }
      }]
    ],
    "cflags_cc": [
      "-std=gnu++1y"
    ]
  },
  "targets": [{
    "target_name": "libwhatwgurl",
    "type": "static_library",
    "sources": [
      "src/table_data.cc",
      "src/whatwgurl.cc",
      "src/whatwgurl_host.cc",
      "src/whatwgurl_parse.cc"
    ],
    "include_dirs": [
      "include"
    ],
    "direct_dependent_settings": {
      "include_dirs": [
        "include"
      ]
    }
  }, {
    "target_name": "example",
    "type": "executable",
    "sources": [
      "example/main.cc"
    ],
    "dependencies": [
      "libwhatwgurl"
    ]
  }]
}
