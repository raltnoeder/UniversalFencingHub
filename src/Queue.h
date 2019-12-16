#ifndef QUEUE_H
#define QUEUE_H

template<typename T>
class Queue
{
  public:
    class Node
    {
        friend class Queue;

      private:
        T* next_node = nullptr;
        T* prev_node = nullptr;

      public:
        Node()
        {
        }

        virtual ~Node() noexcept
        {
        }

        Node(const Node& other) = default;
        Node(Node&& orig) = default;
        virtual Node& operator=(const Node& other) = default;
        virtual Node& operator=(Node&& orig) = default;

        virtual T* get_next_node()
        {
            return next_node;
        }

        virtual T* get_prev_node()
        {
            return prev_node;
        }
    };

  private:
    size_t size  = 0;
    T* head_node = nullptr;
    T* tail_node = nullptr;

  public:
    Queue()
    {
    }

    virtual ~Queue() noexcept
    {
    }

    Queue(const Queue& other) = delete;
    Queue(Queue&& orig) = default;
    virtual Queue& operator=(const Queue& other) = delete;
    virtual Queue& operator=(Queue&& orig) = default;

    virtual void add_first(T* ins_node)
    {
        if (head_node != nullptr)
        {
            head_node->prev_node = ins_node;
        }
        else
        {
            tail_node = ins_node;
        }
        ins_node->next_node = head_node;
        ins_node->prev_node = nullptr;
        head_node = ins_node;
        ++size;
    }

    virtual void add_last(T* ins_node)
    {
        if (tail_node != nullptr)
        {
            tail_node->next_node = ins_node;
        }
        else
        {
            head_node = ins_node;
        }
        ins_node->next_node = nullptr;
        ins_node->prev_node = tail_node;
        tail_node = ins_node;
        ++size;
    }

    virtual T* get_first()
    {
        return head_node;
    }

    virtual T* get_last()
    {
        return tail_node;
    }

    virtual T* remove_first()
    {
        T* rmv_node = head_node;
        if (rmv_node != nullptr)
        {
            if (head_node == tail_node)
            {
                head_node = nullptr;
                tail_node = nullptr;
            }
            else
            {
                head_node = rmv_node->next_node;
                head_node->prev_node = nullptr;
            }
            rmv_node->next_node = nullptr;
            --size;
        }
        return rmv_node;
    }

    virtual T* remove_last()
    {
        T* rmv_node = tail_node;
        if (rmv_node != nullptr)
        {
            if (head_node == tail_node)
            {
                head_node = nullptr;
                tail_node = nullptr;
            }
            else
            {
                tail_node = rmv_node->prev_node;
                tail_node->next_node = nullptr;
                --size;
            }
            rmv_node->prev_node = nullptr;
        }
        return rmv_node;
    }

    virtual void remove(T* rmv_node)
    {
        if (rmv_node->prev_node == nullptr)
        {
            head_node = rmv_node->next_node;
        }
        else
        {
            rmv_node->prev_node->next_node = rmv_node->next_node;
        }
        if (rmv_node->next_node == nullptr)
        {
            tail_node = rmv_node->prev_node;
        }
        else
        {
            rmv_node->next_node->prev_node = rmv_node->prev_node;
        }
        rmv_node->prev_node = nullptr;
        rmv_node->next_node = nullptr;
        --size;
    }

    virtual void clear()
    {
        T* clr_node = head_node;
        while (clr_node != nullptr)
        {
            T* next_node = clr_node->next_node;
            clr_node->next_node = nullptr;
            clr_node->prev_node = nullptr;
            clr_node = next_node;
        }
        head_node = nullptr;
        tail_node = nullptr;
        size = 0;
    }

    virtual size_t get_size() const
    {
        return size;
    }
};

#endif /* QUEUE_H */
