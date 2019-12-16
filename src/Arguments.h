#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <dsaext.h>
#include <QTree.h>
#include <CharBuffer.h>
#include <string>
#include <stdexcept>

class Arguments
{
  private:
    class Entry
    {
      private:
        bool required   = false;
        bool present    = false;
        CharBuffer      value;

      public:
        Entry(const size_t value_capacity);
        virtual ~Entry() noexcept;
        Entry(const Entry& other) = default;
        Entry(Entry&& orig) = default;
        virtual Entry& operator=(const Entry& other) = default;
        virtual Entry& operator=(Entry&& orig) = default;

        virtual void mark_required() noexcept;
        virtual bool is_required() const noexcept;
        virtual void mark_present() noexcept;
        virtual bool is_present() const noexcept;
        CharBuffer& get_value() noexcept;
    };

    using ArgMap = QTree<const CharBuffer, Entry>;

    std::unique_ptr<ArgMap> arg_map_mgr;
    ArgMap*                 arg_map;

  public:
    class ArgumentsException : public std::exception
    {
      public:
        // @throws std::bad_alloc
        ArgumentsException(const std::string& error_msg);
        virtual ~ArgumentsException() noexcept;
        ArgumentsException(const ArgumentsException& other) = default;
        ArgumentsException(ArgumentsException&& orig) = default;
        virtual ArgumentsException& operator=(const ArgumentsException& other) = default;
        virtual ArgumentsException& operator=(ArgumentsException&& orig) = default;
        virtual const char* what() const noexcept override;
      private:
        const std::string message;
    };

    static const char SPLIT_CHAR;

    Arguments();
    virtual ~Arguments() noexcept;
    Arguments(const Arguments& other) = default;
    Arguments(Arguments&& orig) = default;
    virtual Arguments& operator=(const Arguments& other) = default;
    virtual Arguments& operator=(Arguments&& orig) = default;

    // @throws std::bad_alloc
    virtual void add_entry(const char* const key, const size_t value_capacity);

    // @throws std::bad_alloc, ArgumentException
    virtual void process_argument(const CharBuffer& argument);

    // @throws std::bad_alloc, ArgumentsException
    virtual CharBuffer& get_value(const char* const key);

    // @throws std::bad_alloc
    virtual void mark_required(const char* const key);

    // @throws std::bad_alloc, ArgumentsException
    virtual void check_required() const;

  private:
    static int compare_keys(const CharBuffer* const key, const CharBuffer* const other);
};

#endif /* ARGUMENTS_H */
