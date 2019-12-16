#ifndef ZERO_MEMORY_H
#define ZERO_MEMORY_H

template<typename T>
void zero_memory(volatile T* const objects, const size_t object_count)
{
    for (size_t idx = 0; idx < object_count; ++idx)
    {
        objects[idx] = 0;
    }
}

#endif /* ZERO_MEMORY_H */
