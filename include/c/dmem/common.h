/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_COMMON_H
#define DMEM_COMMON_H

#if defined __cplusplus || __STDC_VERSION__ + 0 >= 199901L
#   define DMEM_INLINE static inline
#else
#   define DMEM_INLINE static
#endif

#ifdef _MSC_VER
#	define DMEM_MSC_WARNING(x) __pragma(warning(x))
#else
#	define DMEM_MSC_WARNING(x) 
#endif

#endif

