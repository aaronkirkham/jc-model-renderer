#ifndef JCMR_APP_ALLOCATOR_H_HEADER_GUARD
#define JCMR_APP_ALLOCATOR_H_HEADER_GUARD

#include "platform.h"
#include <new>

#define JCMR_NEW(allocator, ...)                                                                                       \
    new (jcmr::NewPlaceholder(), (allocator).allocate_aligned(sizeof(__VA_ARGS__), alignof(__VA_ARGS__))) __VA_ARGS__
#define JCMR_DELETE(allocator, var) (allocator).deleteObject(var);

#define JCMR_ALLOC(allocator, size) (allocator).allocate(size);
#define JCMR_ALLOC_ALIGNED(allocator, size, align) (allocator).allocate_aligned(size, align);
#define JCMR_FREE(allocator, ptr) (allocator).deallocate(ptr);

namespace jcmr
{
struct NewPlaceholder {
};
} // namespace jcmr

inline void* operator new(jcmr::u64, jcmr::NewPlaceholder, void* where)
{
    return where;
}
inline void operator delete(void*, jcmr::NewPlaceholder, void*) {}

namespace jcmr
{
struct IAllocator {
    virtual ~IAllocator() = default;

    virtual void* allocate(u64 size)              = 0;
    virtual void  deallocate(void* ptr)           = 0;
    virtual void* reallocate(void* ptr, u64 size) = 0;

    virtual void* allocate_aligned(u64 size, u64 align)              = 0;
    virtual void  deallocate_aligned(void* ptr)                      = 0;
    virtual void* reallocate_aligned(void* ptr, u64 size, u64 align) = 0;

    template <typename T> void delete_object(T* ptr)
    {
        if (ptr) {
            ptr->~T();
            deallocate_aligned(ptr);
        }
    }
};

struct DefaultAllocator final : IAllocator {
    void* allocate(u64 size) override;
    void  deallocate(void* ptr) override;
    void* reallocate(void* ptr, u64 size) override;

    void* allocate_aligned(u64 size, u64 align) override;
    void  deallocate_aligned(void* ptr) override;
    void* reallocate_aligned(void* ptr, u64 size, u64 align) override;
};

} // namespace jcmr

#endif // JCMR_APP_ALLOCATOR_H_HEADER_GUARD
