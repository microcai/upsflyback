
#pragma once

#include <type_traits>
#include <utility>

/**
    std::function should be copy-constructable which is not suitable for awaitable object which is move-only,
    so let me write this wrapper that is std::function alike.
*/

struct callable_base
{
    virtual ~callable_base(){}
    virtual void call() = 0;
};

template<typename T>
struct callable_impl : public callable_base
{
    T holder;

    void call() override
    {
        holder();
    }

    ~callable_impl()
    {}

    callable_impl(callable_impl&) = delete;
    callable_impl(callable_impl&&) = delete;

    callable_impl(T&& holder)
        : holder(std::forward<T&&>(holder))
    {}
};

struct callable
{
    void operator()() const
    {
        if (impl)
            impl->call();
    }

    template<typename T>
    callable(T&& function)
    {
        if constexpr (std::is_move_constructible_v<decltype(function)>)
        {
            impl = new callable_impl<std::remove_reference_t<T>>(std::move(function));
        }
        else
        {
            impl = new callable_impl<typename std::remove_reference<T>::type>(function);
        }
    }

    callable(const callable&) = delete;
    callable(callable&) = delete;
    callable(callable&& o)
        : impl(o.impl)
    {
        o.impl = nullptr;
    }

    callable()
        : impl(nullptr)
    {}

    ~callable()
    {
        if (impl)
            delete impl;
    }

    callable_base * impl;
};
