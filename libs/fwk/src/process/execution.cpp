#include <jde/fwk/process/execution.h>
#include <boost/asio.hpp>
#include <jde/fwk/settings.h>
#include <jde/fwk/process/thread.h>
#include <jde/fwk/utils/Vector.h>

namespace Jde::IO{
	constexpr int CompletionSignal{ SIGUSR2 };
}

#define let const auto
namespace Jde{
	namespace asio = boost::asio;
	α ThreadCount()ι->int{ return std::max(Settings::FindNumber<int>( "/workers/executor/threads" ).value_or(std::thread::hardware_concurrency()), 1); }
	sp<asio::io_context> _ioc;
  up<asio::io_context::strand> _ioStrand;
	up<asio::executor_work_guard<asio::io_context::executor_type>> _keepAlive;
}
α Jde::Executor()ι->sp<asio::io_context>{
	if( !_ioc ){
		_ioc = ms<asio::io_context>( ThreadCount() );
		_ioStrand = mu<asio::io_context::strand>( *_ioc );
		_keepAlive = mu<asio::executor_work_guard<asio::io_context::executor_type>>( _ioc->get_executor() );
	}
	return _ioc;
}

namespace Jde{
	static up<Vector<IShutdown*>> _shutdowns;
	α Execution::AddShutdown( IShutdown* shutdown )ι->void{
		if( !_shutdowns )
			_shutdowns = mu<Vector<IShutdown*>>();
	 	_shutdowns->push_back( shutdown );
	}

	struct CancellationSignals final{
		α Emit( asio::cancellation_type ct = asio::cancellation_type::all )ι->void;
		α Slot()ι->asio::cancellation_slot;
		α Signal()ι->sp<asio::cancellation_signal>;
		α Add( sp<asio::cancellation_signal> s )ι->void{ lg _(_mtx); _sigs.push_back( s ); }
		α Clear()ι->void{ lg _(_mtx); _sigs.clear(); }
		α Size()ι->uint{ lg _(_mtx); return _sigs.size(); }
	private:
		vector<sp<asio::cancellation_signal>> _sigs;
		mutex _mtx;
	};
	CancellationSignals _cancelSignals;
	α Execution::AddCancelSignal( sp<asio::cancellation_signal> s )ι->void{ _cancelSignals.Add( s ); }

	α CancelSignals()->CancellationSignals&{ return _cancelSignals; }

	struct ExecutorContext final : IShutdown{
		ExecutorContext()ι:
			_thread{ [&](){
				TRACET( ELogTags::Test, "Ex[0]" );
				sigset_t set;
				sigemptyset( &set );
				sigaddset( &set, IO::CompletionSignal );
				pthread_sigmask( SIG_BLOCK, &set, nullptr );
				Execute();
			}}{
			_started.wait( false );
			INFOT( ELogTags::App, "Created executor threadCount: {}.", ThreadCount() );
			Process::AddShutdown( this );
		}
		~ExecutorContext()ι{ if( !Process::Finalizing() ) Process::RemoveShutdown( this ); _cancelSignals.Clear(); }
		α Shutdown( bool terminate )ι->void override;
		Ω Started()ι->bool{ return _started.test(); }
		Ω Ioc()ι->sp<asio::io_context>{ return _ioc; }
	private:
		Ω Execute()ι->void;
		std::jthread _thread;
		static atomic_flag _started;
		static atomic_flag _stopped;
	};
	up<ExecutorContext> _pExecutorContext;
	atomic_flag ExecutorContext::_started{};
	atomic_flag ExecutorContext::_stopped{};
	α Execution::Run()->void{
		if( !ExecutorContext::Started() ){
			auto keepAlive = Executor();
			_pExecutorContext = mu<ExecutorContext>();
		}
	}

	α CancellationSignals::Emit( asio::cancellation_type ct )ι->void{
		lg _(_mtx);
		for( uint i=0; i<_sigs.size(); ++i ){
			TRACET( ELogTags::App, "Emitting cancellation signal {}.", i );
			_sigs[i]->emit( ct );
		}
	}
	α CancellationSignals::Slot()ι->asio::cancellation_slot{
		return Signal()->slot();
	}
	α CancellationSignals::Signal()ι->sp<asio::cancellation_signal>{
		lg _(_mtx);
		auto p = find_if( _sigs, [](auto& sig){ return !sig->slot().has_handler();} );
		return p == _sigs.end() ? _sigs.emplace_back( ms<asio::cancellation_signal>() ) : *p;
	}
	α ExecutorContext::Shutdown( bool terminate )ι->void{
		DBGT( ELogTags::App, "Executor Shutdown: instances: {}.", _ioc.use_count() );
		_keepAlive->reset();
		_keepAlive = nullptr;
		if( _shutdowns )
			_shutdowns->erase( [=](auto p){p->Shutdown(terminate);} );
		if( _ioc && terminate )
			_ioc->stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
		else{
			_cancelSignals.Emit( asio::cancellation_type::all );
			_stopped.wait( false );
		}
		//Process::RemoveShutdown( this ); deadlock
	}
	α ExecutorContext::Execute()ι->void{
		SetThreadDscrptn( "Ex[0]" );
		vector<std::jthread> v; v.reserve( ThreadCount() - 1 );
		auto ioc = _ioc; //keep alive
		for( auto i = ThreadCount() - 1; i > 0; --i ){
			v.emplace_back( [=]{
				sigset_t set;
				sigemptyset( &set );
				sigaddset( &set, IO::CompletionSignal );
				pthread_sigmask( SIG_BLOCK, &set, nullptr );
				TRACET( ELogTags::Test, "Ex[{}]", i );
				SetThreadDscrptn( Ƒ("Ex[{}]", i) );
				ioc->run();
			});
		}
		TRACET( ELogTags::App, "Executor Started: instances: {}.", ioc.use_count() );
		_stopped.clear();
		_started.test_and_set();
		_started.notify_all();
		ioc->run();
		_ioc.reset();
		_started.clear();
		if( _shutdowns )
			_shutdowns->erase( [=](auto p){ p->Shutdown( false ); } );
		INFOT( ELogTags::App, "Executor Stopped: instances: {}.", ioc.use_count() );
		for( auto& t : v )
			t.join();
		DBGT( ELogTags::App, "Removing Executor remaining instances: {}.", ioc.use_count()-1 );
		ioc.reset(); //need to clear out client connections.
		_cancelSignals.Clear();
		_started.clear();
		_stopped.test_and_set();
		_stopped.notify_all();
	}
}
α Jde::Post( function<void()> f )ι->void{
	auto ctx = Executor();
	asio::post( *ctx, f );
	Execution::Run();
}
α Jde::PostIO( function<void()> f )ι->void{
	Executor();
	asio::post( *_ioStrand, f );
	Execution::Run();
}
α Jde::Post( VoidAwait::Handle&& h )ι->void{
	auto ctx = Executor();
	asio::post( *ctx, [h](){h.resume();} );
	h = nullptr;
}