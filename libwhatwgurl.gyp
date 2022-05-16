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
    }
  },
  "targets": [{
    "target_name": "libwhatwgurl",
    "type": "static_library",
    "sources": [
      "src/host/host.cc",
      "src/host/ip_util.cc",
      "src/utils/assert.cc",
      "src/code_points.cc",
      "src/idna.cc",
      "src/parse.cc",
      "src/path.cc",
      "src/percent_encode-data.cc",
      "src/percent_encode.cc",
      "src/scheme.cc",
      "src/temp_string_buffer.cc",
      "src/url_core.cc",
      "src/url_search_params.cc",
    ],
    "include_dirs": [
      "include"
    ],
    "direct_dependent_settings": {
      "include_dirs": [
        "include"
      ]
    }
  }]
}
