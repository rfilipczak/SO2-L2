#include <iostream>
#include <chrono>
#include <tuple>
#include <algorithm>
#include <syncstream>

#include "iohelp.h"
#include "mythreading.h"


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

    syncout << "[THREAD: " << my::this_thread::id() << "] Start...\n" << std::flush_emit;


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

    syncout << "[THREAD: " << my::this_thread::id() << "] Stop...\n" << std::flush_emit;

    return 0;
}


int main(int argc [[maybe_unused]], char* argv[])
{
    auto [threads_to_create, direction] = setup(argv);
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

    return 0;
}


void print_usage(const std::string& prog_name)
{
    std::cout <<
        "Usage: " << prog_name << " N[" << settings::min_threads_to_create << '-' << settings::max_threads_to_create <<
        "] direction[" << settings::direction_inc << '/' << settings::direcion_dec << "]\n";
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

std::tuple<int, Direction> setup(char** argv)
{
    auto prog_name = iohelp::get_prog_name(argv);
    ++argv; // skip progname

    auto args = iohelp::create_arg_list(argv);

    if (args.size() != 2)
    {
        std::cout << "Invalid number of arguments\n";
        print_usage(prog_name);
        std::exit(EXIT_FAILURE);
    }

    int threads_to_create = 0;

    try
    {
        threads_to_create = std::stoi(args.at(0));
    }
    catch (const std::exception& e)
    {
        std::cout << "Invalid argument: " << args.at(0) << " : " << e.what() << '\n';
        print_usage(prog_name);
        std::exit(EXIT_FAILURE);
    }

    if (threads_to_create < settings::min_threads_to_create || threads_to_create > settings::max_threads_to_create)
    {
        std::cout << "Invalid argument: " << args.at(0) << '\n';
        print_usage(prog_name);
        std::exit(EXIT_FAILURE);
    }

    auto direction = cstr_to_direction(args.at(1));

    if (direction == Direction::Invalid)
    {
        std::cout << "Invalid argument: " << args.at(1) << '\n';
        print_usage(prog_name);
        std::exit(EXIT_FAILURE);
    }

    return std::make_tuple(threads_to_create, direction);
}

