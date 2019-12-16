#ifndef GENALLOC_H
#define GENALLOC_H

#include <cstddef>
#include <new>
#include <memory>
#include <mutex>

template<typename T>
class GenAlloc
{
  public:
    class Dealloc
    {
      private:
        GenAlloc* allocator;

      public:
        Dealloc(GenAlloc* const allocator_ref)
        {
            allocator = allocator_ref;
        }
        virtual ~Dealloc() noexcept
        {
        }
        Dealloc(const Dealloc& other) = default;
        Dealloc(Dealloc&& orig) = default;
        virtual Dealloc& operator=(const Dealloc& other) = default;
        virtual Dealloc& operator=(Dealloc&& orig) = default;
        virtual void operator()(T* obj) const
        {
            allocator->deallocate(obj);
        }
    };

  private:
    mutable std::mutex          lock;
    Dealloc                     deallocator = Dealloc(this);

    size_t                      pool_size;
    std::unique_ptr<T[]>        pool_mgr;
    T*                          pool;

    std::unique_ptr<T*[]>       free_list_mgr;
    T**                         free_list;
    size_t                      alloc_index;

  public:
    using scope_ptr = std::unique_ptr<T, GenAlloc<T>::Dealloc&>;

    // @throws std::bad_alloc
    GenAlloc(const size_t size)
    {
        pool_size   = size;
        pool_mgr    = std::unique_ptr<T[]>(new T[pool_size]);
        pool        = pool_mgr.get();

        free_list_mgr   = std::unique_ptr<T*[]>(new T*[pool_size]);
        free_list       = free_list_mgr.get();
        alloc_index     = pool_size;

        for (size_t idx = 0; idx < pool_size; ++idx)
        {
            free_list[idx] = &pool[idx];
        }
    }

    virtual ~GenAlloc() noexcept
    {
    }

    GenAlloc(const GenAlloc& other) = delete;

    GenAlloc(GenAlloc&& orig)
    {
        std::unique_lock<std::mutex> source_lock(orig.lock);

        pool_size   = orig.pool_size;
        pool_mgr    = std::move(orig.pool_mgr);
        pool        = orig.pool;
        orig.pool   = nullptr;

        free_list_mgr   = std::move(orig.free_list_mgr);
        free_list       = orig.free_list;
        alloc_index     = orig.alloc_index;
        orig.free_list  = nullptr;
    }

    virtual GenAlloc& operator=(const GenAlloc& other) = delete;

    virtual GenAlloc& operator=(GenAlloc&& orig)
    {
        if (this != &orig)
        {
            std::unique_lock<std::mutex> source_lock(orig.lock);
            std::unique_lock<std::mutex> instance_lock(lock);

            pool_size   = orig.pool_size;
            pool_mgr    = std::move(orig.pool_mgr);
            pool        = orig.pool;
            orig.pool   = nullptr;

            free_list_mgr   = std::move(orig.free_list_mgr);
            free_list       = orig.free_list;
            alloc_index     = orig.alloc_index;
            orig.free_list  = nullptr;
        }
        return *this;
    }

    virtual size_t get_pool_size() const
    {
        std::unique_lock<std::mutex> instance_lock(lock);
        return pool_size;
    }

    virtual size_t get_free_count() const
    {
        std::unique_lock<std::mutex> instance_lock(lock);
        return alloc_index;
    }

    virtual size_t get_allocated_count() const
    {
        std::unique_lock<std::mutex> instance_lock(lock);
        return pool_size - alloc_index;
    }

    // @throws std::bad_alloc
    virtual T* allocate()
    {
        std::unique_lock<std::mutex> instance_lock(lock);
        T* obj {nullptr};
        if (alloc_index > 0)
        {
            --alloc_index;
            obj = free_list[alloc_index];
        }
        else
        {
            throw std::bad_alloc();
        }
        return obj;
    }

    virtual void deallocate(T* obj)
    {
        std::unique_lock<std::mutex> instance_lock(lock);
        if (alloc_index < pool_size)
        {
            free_list[alloc_index] = obj;
            ++alloc_index;
        }
    }

    virtual T* object_at_index(const size_t idx)
    {
        T* obj {nullptr};
        if (idx < pool_size)
        {
            obj = &pool[idx];
        }
        return obj;
    }

    virtual scope_ptr allocate_scope()
    {
        return scope_ptr(allocate(), this->deallocator);
    }

    virtual scope_ptr make_scope_ptr(T* obj)
    {
        return scope_ptr(obj, this->deallocator);
    }
};

#endif /* GENALLOC_H */
