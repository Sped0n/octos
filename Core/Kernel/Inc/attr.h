#ifndef __ATTR_H__
#define __ATTR_H__

#define OCTOS_FLATTEN __attribute__((flatten))
#define OCTOS_INLINE __attribute__((always_inline))
#define OCTOS_NO_INLINE __attribute__((noinline))
#define OCTOS_NAKED __attribute__((naked))
#define OCTOS_USED __attribute__((used))
#define OCTOS_UNUSED __attribute__((unused))
#define OCTOS_PACKED __attribute__((packed))

#endif
