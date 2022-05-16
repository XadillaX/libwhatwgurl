{
  "targets": [{
    "target_name": "binding",
    "dependencies": [
      "libwhatwgurl.gyp:libwhatwgurl",
    ],
    "sources": [
      "binding/node_url_search_params.cc",
      "binding/node_url.cc",
    ],
    "include_dirs": [
      "<!(node -e \"require('nan')\")",
      "unicode/common/",
    ],
  }],
}
