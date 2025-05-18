
#pragma once

#include <map>
#include <list>
#include <map>
#include <cstdint>
#include "myfunc.hpp"

namespace mcucoro
{
    class executor
    {
    public:
        // run for ever
        void run();
        // returns if no more active task.
        void poll();
        void poll_one();

        void post(callable fn);
        void post_from_isr(callable fn);

        static executor& system_executor();

        void add_timed_sleeper(uint32_t ms, callable fn);

    protected:
        void clean_sleepers();

    private:
        std::list<callable> active_tasks_from_isr;
        std::list<callable> active_tasks;

        std::multimap<uint32_t, callable> sleepers;
    };

    // delay ms and execute fn
    static inline void delay_ms(int ms, callable fn)
    {
        executor::system_executor().add_timed_sleeper(ms, std::move(fn));
    }

    static inline void yield(callable fn)
    {
        executor::system_executor().post(std::move(fn));
    }

    static inline void post_from_isr(callable fn)
    {
        executor::system_executor().post_from_isr(std::move(fn));
    }

    static inline void post(callable fn)
    {
        executor::system_executor().post(std::move(fn));
    }

    template <typename T>
    static inline void post(T&& fn)
    {
        executor::system_executor().post(callable(fn));
    }

} // namespace mcu
