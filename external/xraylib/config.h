#ifndef XTALOPT_XRAYLIB_CONFIG_H
#define XTALOPT_XRAYLIB_CONFIG_H

#if defined(_WIN32)
#define HAVE__STRDUP 1
#else
#define HAVE_STRDUP 1
#define HAVE_STRNDUP 1
#endif

#endif
