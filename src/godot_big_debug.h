#pragma once

// IWYU pragma: always_keep

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

#define BIG_ERR_FAIL_V_MSG(m_retval, m_msg) \
	if (unlikely(BigNat::is_test_suite)) { \
		return m_retval; \
	} else \
		ERR_FAIL_V_MSG(m_retval, m_msg)

#define BIG_ERR_FAIL_COND_V(m_cond, m_retval) \
	if (unlikely(BigNat::is_test_suite)) { \
		if (unlikely((m_cond))) { \
			return m_retval; \
		} \
	} else \
		ERR_FAIL_COND_V(m_cond, m_retval)

#define BIG_ERR_FAIL_COND_MSG(m_cond, m_msg) \
	if (unlikely(BigNat::is_test_suite)) { \
		if (unlikely((m_cond))) { \
			return; \
		} \
	} else \
		ERR_FAIL_COND_MSG(m_cond, m_msg)

#define BIG_ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg) \
	if (unlikely(BigNat::is_test_suite)) { \
		if (unlikely((m_cond))) { \
			return m_retval; \
		} \
	} else \
		ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)
