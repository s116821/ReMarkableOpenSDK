#pragma once
// Owned build-only syntax shim. Upstream uses only ptrauth_calls queries.
#if !defined(__GNUC__) || defined(__clang__) || !defined(__arm__)
#error This compatibility shim is restricted to GCC ARM32.
#endif
#if !defined(__ARM_ARCH) || __ARM_ARCH != 7 || !defined(__ARM_PCS_VFP)
#error This compatibility shim requires ARMv7 with the hard-float procedure ABI.
#endif
#if defined(__aarch64__) || defined(__arm64e__) || defined(__ARM_FEATURE_PAUTH)
#error Pointer-authentication targets are outside this compatibility shim.
#endif
#ifndef __has_feature
#define __has_feature(feature) 0
#endif
