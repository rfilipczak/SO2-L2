#include <iostream>
#include <chrono>
#include <tuple>
#include <algorithm>
#include <syncstream>
#include <exception>


#include "iohelp.h"
#include "mythreading.h"


#ifdef OS_LINUX
#include <fmt/format.h>
using fmt::format;
#else
#include <format>
using std::format;
#endif


namespace settings
{
    constexpr int min_threads_to_create = 3;
    constexpr int max_threads_to_create = 100;

    const char* direction_inc = "inc";
    const char* direcion_dec = "dec";

    using namespace std::chrono_literals;
    constexpr auto threads_routine_opening_sleep_time = 1000ms;
    constexpr auto cooldown_after_polling_queue = 10ms;
}


enum class Direction
{
    Inc,
    Dec,
    Invalid
};

void print_usage(const std::string& prog_name);
Direction cstr_to_direction(const std::string& str);
std::tuple<int, Direction> setup(char** argv);


class thread_queue
{
private:
    mutable my::mutex mutex{};
    bool m_sorted = false;
    std::size_t current = 0;
public:
    std::vector<my::thread> threads{};

    thread_queue() = default;

    void push(const my::thread& t)
    {
        my::lock_guard g{ mutex };
        threads.emplace_back(t);
    }

    my::thread::id_t peek() const
    {
        my::lock_guard g{ mutex };
        return threads.at(current).getid();
    }

    void next()
    {
        my::lock_guard g{ mutex };
        ++current;
    }

    void sort()
    {
        my::lock_guard g{ mutex };
        std::sort(std::begin(threads), std::end(threads), [](const auto& a, const auto& b) {return a.getid() < b.getid(); });
        m_sorted = true;
    }

    void reverse_sort()
    {
        my::lock_guard g{ mutex };
        std::sort(std::begin(threads), std::end(threads), [](const auto& a, const auto& b) {return a.getid() > b.getid(); });
        m_sorted = true;
    }

    bool sorted() const
    {
        my::lock_guard g{ mutex };
        return m_sorted;
    }
};


void* routine(void* queue)
{
    std::osyncstream syncout{std::cout};

    syncout << format("[THREAD: {}] Start...\n", my::this_thread::id()) << std::flush_emit;

    my::this_thread::sleep_for(settings::threads_routine_opening_sleep_time);

    ::thread_queue* q = (::thread_queue*)queue;

    while (!q->sorted())
    {
        my::this_thread::sleep_for(settings::cooldown_after_polling_queue);
    }

    while (q->peek() != my::this_thread::id())
    {
        my::this_thread::sleep_for(settings::cooldown_after_polling_queue);
    }

    q->next();

    syncout << format("[THREAD: {}] Stop...\n", my::this_thread::id()) << std::flush_emit;

    return 0;
}


int main(int argc [[maybe_unused]], char* argv[])
{
    int threads_to_create = 0;
    Direction direction = Direction::Invalid;

    try
    {
        std::tie(threads_to_create, direction) = setup(argv);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }
    
    try
    {
        ::thread_queue q{};

        for (int i = 0; i < threads_to_create; ++i)
        {
            my::thread t{ routine, &q };
            q.push(t);
        }

        if (direction == Direction::Inc)
            q.sort();
        else
            q.reverse_sort();

        for (auto& t : q.threads)
            t.join();
    }
    catch (const my::threading_exception& e)
    {
        std::cout << e.what() << '\n';
        return 1;
    }

    return 0;
}


void print_usage(const std::string& prog_name)
{
    std::cout << format(
        "Usage: {} N[{}-{}] direction[{}/{}]\n",
            prog_name,
            settings::min_threads_to_create,
            settings::max_threads_to_create,
            settings::direction_inc,
            settings::direcion_dec
        );
}

Direction cstr_to_direction(const std::string& str)
{
    if (str == settings::direction_inc)
        return Direction::Inc;
    else if (str == settings::direcion_dec)
        return Direction::Dec;
    else
        return Direction::Invalid;
}

class setup_exception : public std::exception
{
private:
    std::string error;
public:
    explicit setup_exception(const std::string& _error)
        : error{_error}
    {
    }
    const char* what() const noexcept override
    {
        return error.c_str();
    }
};

std::tuple<int, Direction> setup(char** argv)
{
    auto prog_name = iohelp::get_prog_name(argv);
    ++argv; // skip progname

    auto args = iohelp::create_arg_list(argv);

    if (args.size() != 2)
    {
        throw ::setup_exception("Invalid number of arguments");
    }

    int threads_to_create = 0;

    try
    {
        threads_to_create = std::stoi(args.at(0));
    }
    catch (const std::exception& e [[maybe_unused]])
    {
        throw ::setup_exception(format("Invalid argument: {}", args.at(0)));
    }

    if (threads_to_create < settings::min_threads_to_create || threads_to_create > settings::max_threads_to_create)
    {
        throw ::setup_exception(format("Invalid argument: {}", args.at(0)));
    }

    auto direction = cstr_to_direction(args.at(1));

    if (direction == Direction::Invalid)
    {
        throw ::setup_exception(format("Invalid argument: {}", args.at(1)));
    }

    return std::make_tuple(threads_to_create, direction);
}

