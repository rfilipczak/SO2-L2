#include <iostream>
#include <chrono>
#include <tuple>


#include "iohelp.h"
#include "mythreading.h"


using namespace std::chrono_literals;


namespace settings
{
    constexpr int min_threads_to_create = 3;
    constexpr int max_threads_to_create = 100;

    const char* direction_inc = "inc";
    const char* direcion_dec = "dec";

    constexpr auto threads_routine_opening_sleep_time = 1000ms;
    constexpr auto cooldown_after_polling_queue = 100ms;
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


void* routine(void* queue)
{
    static my::mutex mutex;

    my::unique_lock l{ mutex };
    std::cout << "[THREAD: " << my::this_thread::id() << "] Start..." << std::endl;
    l.unlock();

    my::this_thread::sleep_for(settings::threads_routine_opening_sleep_time);

    my::thread_queue* q = (my::thread_queue*)queue;

    while (!q->sorted())
    {
        my::this_thread::sleep_for(settings::cooldown_after_polling_queue);
    }

    while (q->peek() != my::this_thread::id())
    {
        my::this_thread::sleep_for(settings::cooldown_after_polling_queue);
    }

    l.lock();
    q->next();
    std::cout << "[THREAD: " << my::this_thread::id() << "] Stop..." << std::endl;
    l.unlock();

    return 0;
}


int main(int argc [[maybe_unused]], char* argv[])
{
    auto [threads_to_create, direction] = setup(argv);
    my::thread_queue q{};

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

