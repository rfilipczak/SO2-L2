#ifndef MY_THREADING_H
#define MY_THREADING_H

#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <algorithm>


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
#include <time.h>
#else
#include <windows.h>
#endif

namespace my
{
	struct mutex
	{
#ifdef OS_LINUX
		pthread_mutex_t m;
		mutex()
		{
			pthread_mutex_init(&m, NULL);
		}
		void lock()
		{
			pthread_mutex_lock(&m);
		}
		void unlock()
		{
			pthread_mutex_unlock(&m);
		}
		~mutex()
		{
			pthread_mutex_destroy(&m);
		}
#else // OS_WINDOWS
		HANDLE m;
		mutex()
		{
			m = CreateMutex(NULL, FALSE, NULL);
		}
		void lock()
		{
			WaitForSingleObject(m, INFINITE);
		}
		void unlock()
		{
			ReleaseMutex(m);
		}
		~mutex()
		{
			CloseHandle(m);
		}
#endif
	};

	struct lock_guard
	{
		my::mutex* mutex;
		lock_guard(my::mutex& m)
			: mutex{ &m }
		{
			mutex->lock();
		}
		~lock_guard()
		{
			mutex->unlock();
		}
	};

	struct unique_lock
	{
		my::mutex* mutex;
		unique_lock(my::mutex& m)
			: mutex{ &m }
		{
			mutex->lock();
		}
		void lock()
		{
			mutex->lock();
		}
		void unlock()
		{
			mutex->unlock();
		}
		~unique_lock()
		{
			mutex->unlock();
		}
	};

	class thread
	{
		using Routine = void* (*) (void*);

#ifdef OS_LINUX
	public:
		using id_t = pthread_t;
	private:
		Routine routine;
		pthread_t id;
		void* arg;
	public:
		thread(Routine _routine, void* _arg)
			: routine{ _routine }, arg{ _arg }
		{
			pthread_create(&id, NULL, routine, arg);
		}

		thread(const thread& other)
			: routine{ other.routine }, id{ other.id }, arg{ other.arg }
		{
		}

		[[nodiscard]] pthread_t getid() const
		{
			return id;
		}

		inline void join()
		{
			pthread_join(id, NULL);
		}
		
		bool operator !=(const thread& that)
		{
			return this->id != that.id;
		}
#else // OS_WINDOWS
	public:
		using id_t = DWORD;
	private:
		Routine routine;
		HANDLE threadHandle;
		id_t id;
		void* arg;

	public:
		thread(Routine _routine, void* _arg)
			: routine{ _routine }, arg{ _arg }
		{
			threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)routine, arg, 0, &id);
		}

		thread(const thread& other)
			: routine{ other.routine }, threadHandle{ other.threadHandle }, id{ other.id }, arg{ other.arg }
		{
		}

		[[nodiscard]] id_t getid() const
		{
			return id;
		}

		inline void join()
		{
			WaitForSingleObject(threadHandle, INFINITE);
			CloseHandle(threadHandle);
		}

		bool operator !=(const thread& that)
		{
			return this->id != that.id;
		}
#endif
	};

	class thread_queue
	{
	private:
		mutable my::mutex mutex{};
		std::atomic<bool> m_sorted = false;
		std::atomic<int> current = 0;
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

			struct timespec ts;
			ts.tv_sec = static_cast<long>(duration_cast<seconds>(time).count());
			auto nsec = static_cast<long>(duration_cast<nanoseconds>(time % 1000ms).count());
			ts.tv_nsec = (nsec > 999999999L) ? (999999999L) : (nsec); // man nanosleep for justification

			nanosleep(&ts, NULL);
#else // OS_WINDOWS
			Sleep(static_cast<DWORD>(time.count()));
#endif
		}
	}
}
#endif // MY_THREADING_H
