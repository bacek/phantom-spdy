// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_MISC_H_
#define PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_MISC_H_

// UNUSED macro to mark function parameters.
// Use like "void foo(int UNUSED(p)) {}"
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
# define UNUSED(x)
#else
# define UNUSED(x) x
#endif

#endif  // PHANTOM_IO_BENCHMARK_METHOD_STREAM_SPDY_SPDY_MISC_H_

