#ifndef SRC_URL_CORE_INL_H_
#define SRC_URL_CORE_INL_H_

#include "url_core.h"

#define SET_THE_USERNAME_OR_PASSWORD(username_or_password)                     \
  CHECK(!_failed);                                                             \
                                                                               \
  /* To set the ?? given a url and username, set url’s ?? to */              \
  /* the result of running UTF-8 percent-encode on ?? using the */             \
  /* userinfo percent-encode set. */                                           \
  TempStringBuffer buff;                                                       \
  percent_encode::EncodeByUserInfoPercentEncodeSet(                            \
      reinterpret_cast<const unsigned char*>(username_or_password.c_str()),    \
      username_or_password.length(),                                           \
      &buff);                                                                  \
                                                                               \
  _parsed_url->username_or_password = buff.string()

#define SetTheUsername SET_THE_USERNAME_OR_PASSWORD
#define SetThePassword SET_THE_USERNAME_OR_PASSWORD

#endif  // SRC_URL_CORE_INL_H_
