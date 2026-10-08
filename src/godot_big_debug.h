#pragma once

#ifdef DEV_ENABLED
#define DEBUG_FLOAT_ENABLED
#define DEBUG_KARATSUBA_ENABLED
#endif

#ifdef DEBUG_ENABLED
#define CALIBRATION_SPECIFIERS static thread_local
#define CALIBRATION_SPECIFIERS_MUTABLE
#else
#define CALIBRATION_SPECIFIERS static constexpr
#endif
