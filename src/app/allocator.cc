#include "pch.h"

#include "allocator.h"

#include <malloc.h>
#include <string.h>

namespace jcmr
{
void* DefaultAllocator::allocate(u64 size)
{
    return malloc(size);
}

void DefaultAllocator::deallocate(void* ptr)
{
    free(ptr);
}

void* DefaultAllocator::reallocate(void* ptr, u64 size)
{
    return realloc(ptr, size);
}

void* DefaultAllocator::allocate_aligned(u64 size, u64 align)
{
    return _aligned_malloc(size, align);
}

void DefaultAllocator::deallocate_aligned(void* ptr)
{
    _aligned_free(ptr);
}

void* DefaultAllocator::reallocate_aligned(void* ptr, u64 size, u64 align)
{
    return _aligned_realloc(ptr, size, align);
}
} // namespace jcmr