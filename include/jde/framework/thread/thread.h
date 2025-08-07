#pragma once
#ifndef THREAD_H
#define THREAD_H
#include <string_view>
#include <shared_mutex>

#define Φ Γ α
namespace Jde{
	enum class ELogLevel : int8;
	Φ SetThreadDscrptn( std::thread& thread, sv description )ι->void;
	Φ SetThreadDscrptn( sv description )ι->void;
	Φ ThreadDscrptn()ι->const char*;
	Φ ThreadId()ι->uint;
}
namespace Jde::Threading{
	using namespace std::literals;
	typedef uint HThread;
	struct ThreadParam{ string Name; HThread AppHandle; };
	enum class EThread : int{
		Application = 0,
		Startup = 1,
		LogServer = 2,
		CoroutinePool = 3,
		AppSpecific = 1 << 10
	};

	Φ GetThreadId()ι->uint;
	Φ GetAppThreadHandle()ι->uint;
	Φ SetThreadInfo( const ThreadParam& param )ι->void;
	α BumpThreadHandle()ι->HThread;
	Φ SetThreadHandle( HThread handle )ι->void;

	//taken from https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action-second-edition/chapter-8/v-7/1
	class ThreadCollection{ //TODO refactor [re]move this
	public:
		explicit ThreadCollection( std::vector<std::thread>& threads ):
			_threads( threads)
		{}
		~ThreadCollection()
		{
			for( auto& thread : _threads )
			{
				if( thread.joinable() )
					thread.join();
			}
		}
		private:
			std::vector<std::thread>& _threads;
	};
}
#undef Φ
#endif