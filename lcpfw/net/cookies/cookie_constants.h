// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_COOKIES_COOKIE_CONSTANTS_H_
#define NET_COOKIES_COOKIE_CONSTANTS_H_

#include <string>

#include "base/time/time.h"
#include "net/base/net_export.h"

namespace net {

// The time threshold for considering a cookie "short-lived" for the purposes of
// allowing unsafe methods for unspecified-SameSite cookies defaulted into Lax.
NET_EXPORT extern const base::TimeDelta kLaxAllowUnsafeMaxAge;
// The short version of the above time threshold, to be used for tests.
NET_EXPORT extern const base::TimeDelta kShortLaxAllowUnsafeMaxAge;

enum CookiePriority {
  COOKIE_PRIORITY_LOW     = 0,
  COOKIE_PRIORITY_MEDIUM  = 1,
  COOKIE_PRIORITY_HIGH    = 2,
  COOKIE_PRIORITY_DEFAULT = COOKIE_PRIORITY_MEDIUM
};

// See https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00
// and https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis for
// information about same site cookie restrictions.
// These values are allowed for the SameSite field of a cookie. They mostly
// correspond to CookieEffectiveSameSite values.
// Note: Don't renumber, as these values are persisted to a database.
enum class CookieSameSite {
  UNSPECIFIED = -1,
  NO_RESTRICTION = 0,
  LAX_MODE = 1,
  STRICT_MODE = 2,
  // Reserved 3 (was EXTENDED_MODE), next number is 4.
};

// These are the enforcement modes that may be applied to a cookie when deciding
// inclusion/exclusion. They mostly correspond to CookieSameSite values.
// Keep in sync with enums.xml.
enum class CookieEffectiveSameSite {
  NO_RESTRICTION = 0,
  LAX_MODE = 1,
  STRICT_MODE = 2,
  LAX_MODE_ALLOW_UNSAFE = 3,
  // Undefined is used when no value applies for the object as there is no
  // valid cookie object to evaluate on.
  UNDEFINED = 4,

  // Keep last, used for histograms.
  COUNT,
  kMaxValue = COUNT
};

// Used for histograms only. Do not renumber. Keep in sync with enums.xml.
enum class CookieSameSiteString {
  // No SameSite attribute is present.
  kUnspecified = 0,
  // The SameSite attribute is present but has no value.
  kEmptyString = 1,
  // The SameSite attribute has an unrecognized value.
  kUnrecognized = 2,
  // The SameSite attribute has a recognized value.
  kLax = 3,
  kStrict = 4,
  kNone = 5,
  kExtended = 6,  // Deprecated, kept for metrics only.

  // Keep last, update if adding new value.
  kMaxValue = kExtended
};

// What rules to apply when determining whether access to a particular cookie is
// allowed.
enum class CookieAccessSemantics {
  // Has not been checked yet or there is no way to check.
  UNKNOWN = -1,
  // Has been checked and the cookie should *not* be subject to legacy access
  // rules.
  NONLEGACY = 0,
  // Has been checked and the cookie should be subject to legacy access rules.
  LEGACY,
};

enum class CookieSamePartyStatus {
  // Used when there should be no SameParty enforcement (either because the
  // cookie is not marked SameParty, or the enforcement is irrelevant).
  kNoSamePartyEnforcement = 0,
  // Used when SameParty enforcement says to exclude the cookie.
  kEnforceSamePartyExclude = 1,
  // Used when SameParty enforcement says to include the cookie.
  kEnforceSamePartyInclude = 2,
};

// What scheme was used in the setting of a cookie.
// Do not renumber.
enum class CookieSourceScheme {
  kUnset = 0,
  kNonSecure = 1,
  kSecure = 2,

  kMaxValue = kSecure  // Keep as the last value.
};

enum class CookiePort {
  // DO NOT REORDER OR RENUMBER. These are used for histograms.

  // Potentially interesting port values for cookies for use with histograms.

  // Not a port explicitly listed below, including invalid ports (-1, 65536,
  // etc).
  kOther = 0,
  // HTTP
  k80 = 1,
  k81 = 2,
  k82 = 3,
  k83 = 4,
  k84 = 5,
  k85 = 6,
  // HTTPS
  k443 = 7,
  k444 = 8,
  k445 = 9,
  k446 = 10,
  k447 = 11,
  k448 = 12,
  // JS Framework
  k3000 = 13,
  k3001 = 14,
  k3002 = 15,
  k3003 = 16,
  k3004 = 17,
  k3005 = 18,
  // JS Framework
  k4200 = 19,
  k4201 = 20,
  k4202 = 21,
  k4203 = 22,
  k4204 = 23,
  k4205 = 24,
  // JS Framework
  k5000 = 25,
  k5001 = 26,
  k5002 = 27,
  k5003 = 28,
  k5004 = 29,
  k5005 = 30,
  // Common Dev Ports
  k7000 = 31,
  k7001 = 32,
  k7002 = 33,
  k7003 = 34,
  k7004 = 35,
  k7005 = 36,
  // HTTP
  k8000 = 37,
  k8001 = 38,
  k8002 = 39,
  k8003 = 40,
  k8004 = 41,
  k8005 = 42,
  // HTTP
  k8080 = 43,
  k8081 = 44,
  k8082 = 45,
  k8083 = 46,
  k8084 = 47,
  k8085 = 48,
  // HTTP
  k8090 = 49,
  k8091 = 50,
  k8092 = 51,
  k8093 = 52,
  k8094 = 53,
  k8095 = 54,
  // JS Framework
  k8100 = 55,
  k8101 = 56,
  k8102 = 57,
  k8103 = 58,
  k8104 = 59,
  k8105 = 60,
  // JS Framework
  k8200 = 61,
  k8201 = 62,
  k8202 = 63,
  k8203 = 64,
  k8204 = 65,
  k8205 = 66,
  // HTTP(S)
  k8443 = 67,
  k8444 = 68,
  k8445 = 69,
  k8446 = 70,
  k8447 = 71,
  k8448 = 72,
  // HTTP
  k8888 = 73,
  k8889 = 74,
  k8890 = 75,
  k8891 = 76,
  k8892 = 77,
  k8893 = 78,
  // Common Dev Ports
  k9000 = 79,
  k9001 = 80,
  k9002 = 81,
  k9003 = 82,
  k9004 = 83,
  k9005 = 84,
  // HTTP
  k9090 = 85,
  k9091 = 86,
  k9092 = 87,
  k9093 = 88,
  k9094 = 89,
  k9095 = 90,

  // Keep as last value.
  kMaxValue = k9095
};

// Scheme or trustworthiness used to access or set a cookie.
// "potentially trustworthy" here refers to the notion from
// https://www.w3.org/TR/powerful-features/#is-origin-trustworthy
enum class CookieAccessScheme {
  // Scheme was non-cryptographic. The non-cryptographic source origin was
  // either not potentially trustworthy, or its potential
  // trustworthiness wasn't checked.
  kNonCryptographic = 0,
  // Scheme was cryptographic (https or wss). This implies potentially
  // trustworthy.
  kCryptographic = 1,
  // Source was non-cryptographic, but URL was otherwise potentially
  // trustworthy.
  kTrustworthy = 2,

  kMaxValue = kTrustworthy  // Keep as the last value.
};

// Returns the Set-Cookie header priority token corresponding to |priority|.
NET_EXPORT std::string CookiePriorityToString(CookiePriority priority);

// Converts the Set-Cookie header priority token |priority| to a CookiePriority.
// Defaults to COOKIE_PRIORITY_DEFAULT for empty or unrecognized strings.
NET_EXPORT CookiePriority StringToCookiePriority(const std::string& priority);

// Returns a string corresponding to the value of the |same_site| token.
// Intended only for debugging/logging.
NET_EXPORT std::string CookieSameSiteToString(CookieSameSite same_site);

// Converts the Set-Cookie header SameSite token |same_site| to a
// CookieSameSite. Defaults to CookieSameSite::UNSPECIFIED for empty or
// unrecognized strings. Returns an appropriate value of CookieSameSiteString in
// |samesite_string| to indicate what type of string was parsed as the SameSite
// attribute value, if a pointer is provided.
NET_EXPORT CookieSameSite
StringToCookieSameSite(const std::string& same_site,
                       CookieSameSiteString* samesite_string = nullptr);

NET_EXPORT void RecordCookieSameSiteAttributeValueHistogram(
    CookieSameSiteString value,
    bool is_cookie_same_party = false);

// This function reduces the 65535 available TCP port values down to a <100
// potentially interesting values that cookies could be set by or sent to. This
// is because UMA cannot handle the full range.
NET_EXPORT CookiePort ReducePortRangeForCookieHistogram(const int port);

}  // namespace net

#endif  // NET_COOKIES_COOKIE_CONSTANTS_H_
