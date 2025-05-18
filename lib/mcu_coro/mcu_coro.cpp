
#include "mcu_coro.hpp"
#ifdef  __arm
#include "cmsis_compiler.h"
#endif

#ifdef ARDUINO
#include <Arduino.h>
#else
extern "C" uint32_t millis(void);
#endif

#include <utility>

namespace mcucoro{
    executor& executor::system_executor()
    {
        static executor instance;
        return instance;
    }

    void executor::post_from_isr(callable fn)
    {
        active_tasks_from_isr.push_back(std::move(fn));
    }

    void executor::post(callable fn)
    {
        active_tasks.push_back(std::move(fn));
    }

    void executor::poll_one()
    {
        decltype(active_tasks) to_be_run;

        #ifdef ESP_PLATFORM
        taskDISABLE_INTERRUPTS();
        #elif defined(__arm)
        __disable_irq();
        #endif
        if (!active_tasks_from_isr.empty())
        {
            to_be_run = std::move(active_tasks_from_isr);
            #ifdef ESP_PLATFORM
            taskENABLE_INTERRUPTS();
            #elif defined(__arm)
            __enable_irq();
            #endif

        }
        else
        {
            #ifdef ESP_PLATFORM
            taskENABLE_INTERRUPTS();
            #elif defined(__arm)
            __enable_irq();
            #endif

            clean_sleepers();
            to_be_run = std::move(active_tasks);
        }

        for( auto & T : to_be_run)
        {
            T();
        }
    }

    void executor::poll()
    {
        poll_one();
    }

    void executor::add_timed_sleeper(uint32_t ms, callable fn)
    {
        auto point = millis() + ms;
        sleepers.emplace( point, std::move(fn));
    }

    void executor::clean_sleepers()
    {
        auto execute_and_delete_and_advance_next = [this](auto it)
        {
            auto old_it = it;
            it ++;
            active_tasks.push_back(std::move(old_it->second));
            sleepers.erase(old_it);
            return it;
        };

        auto now = millis();
        for (auto it = sleepers.begin(); it != sleepers.end();)
        {
            if (now >= it->first)
            {
                if ((now - it->first) < UINT16_MAX)
                {
                    it = execute_and_delete_and_advance_next(it);
                    continue;
                }
            }
            else if (now < it->first)
            {
                if ((now - it->first) < UINT16_MAX)
                {
                    it = execute_and_delete_and_advance_next(it);
                    continue;
                }
            }
            it ++;
        }
    }

}