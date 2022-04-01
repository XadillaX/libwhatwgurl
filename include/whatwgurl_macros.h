#ifndef INCLUDE_WHATWGURL_MACROS_H_
#define INCLUDE_WHATWGURL_MACROS_H_

#define PARSESTATES(XX)                                                        \
  XX(kSchemeStart)                                                             \
  XX(kScheme)                                                                  \
  XX(kNoScheme)                                                                \
  XX(kSpecialRelativeOrAuthority)                                              \
  XX(kPathOrAuthority)                                                         \
  XX(kRelative)                                                                \
  XX(kRelativeSlash)                                                           \
  XX(kSpecialAuthoritySlashes)                                                 \
  XX(kSpecialAuthorityIgnoreSlashes)                                           \
  XX(kAuthority)                                                               \
  XX(kHost)                                                                    \
  XX(kHostname)                                                                \
  XX(kPort)                                                                    \
  XX(kFile)                                                                    \
  XX(kFileSlash)                                                               \
  XX(kFileHost)                                                                \
  XX(kPathStart)                                                               \
  XX(kPath)                                                                    \
  XX(kCannotBeBase)                                                            \
  XX(kQuery)                                                                   \
  XX(kFragment)

#define FLAGS(XX)                                                              \
  XX(kUrlFlagsNone, 0)                                                         \
  XX(kUrlFlagsFailed, 0x01)                                                    \
  XX(kUrlFlagsCannotBeBase, 0x02)                                              \
  XX(kUrlFlagsInvalidParseState, 0x04)                                         \
  XX(kUrlFlagsTerminated, 0x08)                                                \
  XX(kUrlFlagsSpecial, 0x10)                                                   \
  XX(kUrlFlagsHasUsername, 0x20)                                               \
  XX(kUrlFlagsHasPassword, 0x40)                                               \
  XX(kUrlFlagsHasHost, 0x80)                                                   \
  XX(kUrlFlagsHasPath, 0x100)                                                  \
  XX(kUrlFlagsHasQuery, 0x200)                                                 \
  XX(kUrlFlagsHasFragment, 0x400)                                              \
  XX(kUrlFlagsIsDefaultSchemePort, 0x800)

#endif  // INCLUDE_WHATWGURL_MACROS_H_
