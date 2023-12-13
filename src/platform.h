#ifndef JCMR_SRC_PLATFORM_H_HEADER_GUARD
#define JCMR_SRC_PLATFORM_H_HEADER_GUARD

extern void __assert_handler(const char* expr_str, const char* file, int line);

#ifndef ASSERT
#ifdef _DEBUG
#define DEBUG_BREAK() __debugbreak()
#define ASSERT(x)                                                                                                      \
    do {                                                                                                               \
        const volatile bool assert_b____ = !(x);                                                                       \
        if (assert_b____) {                                                                                            \
            __assert_handler(#x, __FILE__, __LINE__);                                                                  \
            DEBUG_BREAK();                                                                                             \
        }                                                                                                              \
    } while (false)
#else
#define DEBUG_BREAK() (void)0;
#define ASSERT(x) __assume(x)
#endif
#endif

namespace jcmr
{
typedef char               i8;
typedef unsigned char      u8;
typedef short              i16;
typedef unsigned short     u16;
typedef int                i32;
typedef unsigned int       u32;
typedef float              f32;
typedef double             f64;
typedef long long          i64;
typedef unsigned long long u64;
typedef i64                intptr;
typedef u64                uintptr;

static_assert(sizeof(i8) == 1, "Incorrect size of i8");
static_assert(sizeof(i16) == 2, "Incorrect size of i16");
static_assert(sizeof(i32) == 4, "Incorrect size of i32");
static_assert(sizeof(i64) == 8, "Incorrect size of i64");
static_assert(sizeof(f32) == 4, "Incorrect size of f32");
static_assert(sizeof(f64) == 8, "Incorrect size of f64");
static_assert(sizeof(uintptr) == sizeof(void*), "Incorrect size of uintptr");

using ByteArray = std::vector<u8>;

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::mat2;
using glm::mat2x2;
using glm::mat2x3;
using glm::mat2x4;
using glm::mat3;
using glm::mat3x2;
using glm::mat3x3;
using glm::mat3x4;
using glm::mat4;
using glm::mat4x2;
using glm::mat4x3;
using glm::mat4x4;

using glm::quat;

const u32 MAX_PATH_LENGTH = 260;

template <typename T, u32 count> constexpr u32 lengthOf(const T (&)[count])
{
    return count;
}
} // namespace jcmr

#endif // JCMR_SRC_PLATFORM_H_HEADER_GUARD
