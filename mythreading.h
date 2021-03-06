#ifndef MY_THREADING_H
#define MY_THREADING_H

#include <utility>
#include <vector>
#include <chrono>
#include <exception>
#include <string>

#ifdef __unix__         
#define OS_LINUX
#elif defined(_WIN32) || defined(WIN32) 
#define OS_WINDOWS
#else
#error "unsupported OS"
#endif

#ifdef OS_LINUX
#include <pthread.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <fmt/format.h>
#else
#include <windows.h>
#include <format>
#endif

namespace my
{
	class threading_exception : public std::exception
	{
	private:
		std::string error;
	public:
		explicit threading_exception(std::string  _error)
			: error{std::move(_error)}
		{
		}
		[[nodiscard]] const char* what() const noexcept override
		{
			return error.c_str();
		}
	};

	struct mutex
	{
#ifdef OS_LINUX
		pthread_mutex_t m{};
		mutex()
		{
			if (pthread_mutex_init(&m, nullptr) != 0)
			{
				throw threading_exception(fmt::format("pthread_mutex_init failed : {}", std::strerror(errno)));
			}
		}
		void lock()
		{
			if (pthread_mutex_lock(&m) != 0)
			{
				throw threading_exception(fmt::format("pthread_mutex_lock failed : {}", std::strerror(errno)));
			}
		}
		void unlock()
		{
			if (pthread_mutex_unlock(&m) != 0)
			{
				throw threading_exception(fmt::format("pthread_mutex_unlock failed : {}", std::strerror(errno)));
			}
		}
		~mutex()
		{
			if (pthread_mutex_destroy(&m) != 0)
			{
				std::cerr << fmt::format("pthread_mutex_destroy failed : {}\n", std::strerror(errno));
			}
		}
#else // OS_WINDOWS
		HANDLE m{};
		mutex()
		{
			m = CreateMutex(NULL, FALSE, NULL);
			if (m == NULL)
			{
				throw threading_exception(std::format("CreateMutex failed : {}", GetLastError()));
			}
		}
		void lock()
		{
			if (WaitForSingleObject(m, INFINITE) == WAIT_FAILED)
			{
				throw threading_exception(std::format("WaitForSingleObject failed : {}", GetLastError()));
			}
		}
		void unlock()
		{
			if (ReleaseMutex(m) == 0)
			{
				throw threading_exception(std::format("ReleaseMutex failed : {}", GetLastError()));
			}
		}
		~mutex()
		{
			if (CloseHandle(m) == 0)
			{
				throw threading_exception(std::format("CloseHandle failed : {}", GetLastError()));
			}
			
		}
#endif
	};

	struct lock_guard
	{
		my::mutex* mutex;
		explicit lock_guard(my::mutex& m)
			: mutex{ &m }
		{
			try
			{
				mutex->lock();
			}
			catch (const threading_exception& e)
			{
				throw;
			}
		}
		~lock_guard()
		{
			try
			{
				mutex->unlock();
			}
			catch (const threading_exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	};

	struct [[maybe_unused]] unique_lock
	{
		my::mutex* mutex;

        [[maybe_unused]] explicit unique_lock(my::mutex& m)
			: mutex{ &m }
		{
			try
			{
				mutex->lock();
			}
			catch (const threading_exception& e)
			{
				throw;
			}
		}
		void lock() const
		{
			try
			{
				mutex->lock();
			}
			catch (const threading_exception& e)
			{
				throw;
			}
		}
		void unlock() const
		{
			try
			{
				mutex->unlock();
			}
			catch (const threading_exception& e)
			{
				throw;
			}
		}
		~unique_lock()
		{
			try
			{
				mutex->unlock();
			}
			catch (const threading_exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	};

	class thread
	{
		using routine_t = void* (*) (void*);

#ifdef OS_LINUX
	public:
		using id_t = pthread_t;
	private:
		routine_t routine;
		pthread_t id{};
		void* arg;
	public:
		thread(routine_t _routine, void* _arg)
			: routine{ _routine }, arg{ _arg }
		{
			if (pthread_create(&id, nullptr, routine, arg) != 0)
			{
				throw threading_exception(fmt::format("pthread_create failed : {}", std::strerror(errno)));
			}
		}

		thread(const thread& other) = default;

		[[nodiscard]] pthread_t getid() const
		{
			return id;
		}

		void join() const
		{
			if (pthread_join(id, nullptr) != 0)
			{
				throw threading_exception(fmt::format("pthread_join failed : {}", std::strerror(errno)));
			}
		}
		
		bool operator !=(const thread& that) const
		{
			return this->id != that.id;
		}
#else // OS_WINDOWS
	public:
		using id_t = DWORD;
	private:
		routine_t routine;
		HANDLE thread_handle{};
		id_t id{};
		void* arg;

	public:
		thread(routine_t _routine, void* _arg)
			: routine{ _routine }, arg{ _arg }
		{
			thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)routine, arg, 0, &id);
			if (thread_handle == NULL)
			{
				throw threading_exception(std::format("CreateThread failed : {}", GetLastError()));
			}
		}

		thread(const thread& other)
			: routine{ other.routine }, thread_handle{ other.thread_handle }, id{ other.id }, arg{ other.arg }
		{
		}

		[[nodiscard]] id_t getid() const
		{
			return id;
		}

		void join()
		{
			if (WaitForSingleObject(thread_handle, INFINITE) == WAIT_FAILED)
			{
				throw threading_exception(std::format("WaitForSingleObject failed : {}", GetLastError()));
			}
			if (CloseHandle(thread_handle) == 0)
			{
				throw threading_exception(std::format("CloseHandle failed : {}", GetLastError()));
			}
		}

		bool operator !=(const thread& that) const
		{
			return this->id != that.id;
		}
#endif
	};

	namespace this_thread
	{
		thread::id_t id()
		{
#ifdef OS_LINUX
			return pthread_self();
#else // OS_WINDOWS
			return GetCurrentThreadId();
#endif
		}

		void sleep_for(std::chrono::milliseconds time)
		{
#ifdef OS_LINUX
			using namespace std::chrono_literals;
			using namespace std::chrono;

			struct timespec ts{};
			ts.tv_sec = static_cast<long>(duration_cast<seconds>(time).count());
			ts.tv_nsec = static_cast<long>(duration_cast<nanoseconds>(time % 1000ms).count());

			nanosleep(&ts, nullptr);
#else // OS_WINDOWS
			Sleep(static_cast<DWORD>(time.count()));
#endif
		}
	}
}
#endif // MY_THREADING_H
