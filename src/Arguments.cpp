#include "Arguments.h"

#include <RangeException.h>
#include <iostream> // FIXME: DEBUG

const char Arguments::SPLIT_CHAR = '=';

// @throws std::bad_alloc
Arguments::Arguments():
    arg_map_mgr(new ArgMap(&Arguments::compare_keys))
{
    arg_map = arg_map_mgr.get();
}

Arguments::~Arguments() noexcept
{
    arg_map = arg_map_mgr.get();
    if (arg_map != nullptr)
    {
        ArgMap::NodesIterator map_iter(*arg_map);
        while (map_iter.has_next())
        {
            ArgMap::Node* const map_node = map_iter.next();
            delete map_node->get_key();
            delete map_node->get_value();
        }
        arg_map->clear();
    }
}

// @throws std::bad_alloc, dsaext::DuplicateInsertException
void Arguments::add_entry(const char* const key, const size_t value_capacity)
{
    std::unique_ptr<CharBuffer> key_mgr(new CharBuffer(key));
    std::unique_ptr<Entry> entry_mgr(new Entry(value_capacity));
    arg_map->insert(key_mgr.get(), entry_mgr.get());
    key_mgr.release();
    entry_mgr.release();
}

// @throws std::bad_alloc, ArgumentsException
void Arguments::process_argument(const CharBuffer& argument)
{
    const size_t split_idx = argument.index_of(SPLIT_CHAR);
    if (split_idx != CharBuffer::NPOS)
    {
        CharBuffer search_key(split_idx);
        search_key.substring_from(argument, 0, split_idx);

        Entry* const arg_entry = arg_map->get(&search_key);
        if (arg_entry != nullptr)
        {
            if (arg_entry->is_present())
            {
                std::string error_msg("Duplicate argument with argument key \"");
                error_msg += search_key.c_str();
                error_msg += "\"";
                throw ArgumentsException(error_msg);
            }
            CharBuffer& value = arg_entry->get_value();
            try
            {
                value.substring_from(argument, split_idx + 1, argument.length());
                arg_entry->mark_present();
            }
            catch (RangeException&)
            {
                std::string error_msg("The value for the argument key \"");
                error_msg += search_key.c_str();
                error_msg += "\" exceeds its allowed maximum length";
                throw ArgumentsException(error_msg);
            }
        }
        else
        {
            std::string error_msg("Invalid argument key \"");
            error_msg += search_key.c_str();
            error_msg += "\"";
            throw ArgumentsException(error_msg);
        }
    }
    else
    {
        std::string error_msg("Malformed argument \"");
        error_msg += argument.c_str();
        error_msg += "\"";
        throw ArgumentsException(error_msg);
    }
}

// @throws std::bad_alloc, ArgumentsException
void Arguments::mark_required(const char* const key)
{
    CharBuffer search_key(key);
    Entry* const arg_entry = arg_map->get(&search_key);
    if (arg_entry != nullptr)
    {
        arg_entry->mark_required();
    }
    else
    {
        std::string error_msg("Internal error: Attempt to execute method mark_required with nonexistent key=\"");
        error_msg += search_key.c_str();
        error_msg += "\"";
        throw ArgumentsException(error_msg);
    }
}

// @throws std::bad_alloc, ArgumentsException
void Arguments::check_required() const
{
    std::unique_ptr<std::string> error_msg_mgr;
    std::string* error_msg = nullptr;

    ArgMap::NodesIterator map_iter(*arg_map);
    while (map_iter.has_next())
    {
        ArgMap::Node* const map_node = map_iter.next();
        Entry* const arg_entry = map_node->get_value();
        if (arg_entry->is_required() && !arg_entry->is_present())
        {
            if (error_msg == nullptr)
            {
                error_msg_mgr = std::unique_ptr<std::string>(
                    new std::string("The values for the following argument keys were not specified:\n")
                );
                error_msg = error_msg_mgr.get();
            }
            *error_msg += "    ";
            *error_msg += map_node->get_key()->c_str();
            *error_msg += '\n';
        }
    }
    if (error_msg != nullptr)
    {
        throw ArgumentsException(*error_msg);
    }
}

// @throws std::bad_alloc, ArgumentsException
CharBuffer& Arguments::get_value(const char* const key)
{
    CharBuffer search_key(key);
    Entry* const arg_entry = arg_map->get(&search_key);
    if (arg_entry == nullptr)
    {
        std::string error_msg("The value for the argument key \"");
        error_msg += key;
        error_msg += "\" was not specified";
        throw ArgumentsException(error_msg);
    }
    return arg_entry->get_value();
}

int Arguments::compare_keys(const CharBuffer* const key, const CharBuffer* const other)
{
    int rc = 0;
    if (key != nullptr)
    {
        if (other != nullptr)
        {
            rc = (*key).compare_to(*other);
        }
        else
        {
            rc = 1;
        }
    }
    else
    {
        if (other != nullptr)
        {
            rc = -1;
        }
    }
    return rc;
}

// @throws std::bad_alloc
Arguments::Entry::Entry(const size_t value_capacity):
    value(value_capacity)
{
}

Arguments::Entry::~Entry() noexcept
{
}

void Arguments::Entry::mark_required() noexcept
{
    required = true;
}

bool Arguments::Entry::is_required() const noexcept
{
    return required;
}

void Arguments::Entry::mark_present() noexcept
{
    present = true;
}

bool Arguments::Entry::is_present() const noexcept
{
    return present;
}

CharBuffer& Arguments::Entry::get_value() noexcept
{
    return value;
}

Arguments::ArgumentsException::ArgumentsException(const std::string& error_msg):
    message(error_msg)
{
}

Arguments::ArgumentsException::~ArgumentsException() noexcept
{
}

const char* Arguments::ArgumentsException::what() const noexcept
{
    return message.c_str();
}
