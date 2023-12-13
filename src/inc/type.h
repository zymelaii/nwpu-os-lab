#ifndef	MINIOS_TYPE_H
#define	MINIOS_TYPE_H

#ifndef NULL
#define NULL ((void*) 0)
#endif

typedef _Bool bool;
enum { false, true };

typedef long long		i64;
typedef unsigned long long	u64;
typedef int			i32;
typedef unsigned int		u32;
typedef short			i16;
typedef unsigned short		u16;
typedef char			i8;
typedef unsigned char		u8;

typedef i32			intptr_t;
typedef u32 			uintptr_t;

// 通常描述一个对象的大小，会根据机器的型号变化类型
typedef u32			size_t;
// signed size_t 通常描述系统调用返回值，会根据机器的型号变化类型
typedef i32			ssize_t;
// 通常描述偏移量，会根据机器的型号变化类型
typedef i32			off_t;
// 通常描述物理地址
typedef u32			phyaddr_t;

#define MIN(_a, _b)			\
({					\
	typeof(_a) __a = (_a);		\
	typeof(_b) __b = (_b);		\
	__a <= __b ? __a : __b;		\
})

#define MAX(_a, _b)			\
({					\
	typeof(_a) __a = (_a);		\
	typeof(_b) __b = (_b);		\
	__a >= __b ? __a : __b;		\
})

#define ROUNDDOWN(a, n)			\
({					\
	u32 __a = (u32) (a);		\
	(typeof(a)) (__a - __a % n);	\
})

#define ROUNDUP(a, n)			\
({					\
	u32 __n = (u32) (n);		\
	(typeof(a)) (ROUNDDOWN((u32) (a) + __n - 1, __n)); \
})

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

// 求出在结构体中的某个成员的偏移量
#define offsetof(type, member) ((size_t) (&((type*)0)->member))

#endif /* MINIOS_TYPE_H */
