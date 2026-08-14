#include <jde/fwk/co/AnyAwait.h>
#include <jde/fwk/co/Await.h>
#include <atomic>
#include <thread>

#define let const auto

namespace Jde::Tests{
	struct AnyAwaitTests : public ::testing::Test{};

	Ω settle( const std::atomic<uint>& completed, uint expected )->bool{
		let deadline = steady_clock::now()+5s;
		while( completed!=expected && steady_clock::now()<deadline )
			std::this_thread::sleep_for( 1ms );
		return completed==expected;
	}

	struct TestException final : Exception{
		TestException( string what, SRCE )ι:Exception{ move(what), {}, sl }{}
		α Move()ι->up<Exception> override{ return mu<TestException>( move(*this) ); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};

	//new-style awaitables - any coroutine can co_await them.
	template<class T>
	struct InlineAny final : AnyAwait<T>{
		InlineAny( T v, SRCE )ι:AnyAwait<T>{sl},_v{move(v)}{}
		α Suspend()ι->void override{ this->Resume( move(_v) ); }	//resumes the awaiter inline from await_suspend - the nested-resume path.
		T _v;
	};
	struct InlineVoidAny final : AnyVoidAwait{
		InlineVoidAny( SRCE )ι:AnyVoidAwait{sl}{}
		α Suspend()ι->void override{ Resume(); }
	};
	template<class T>
	struct ThreadedAny final : AnyAwait<T>{
		ThreadedAny( T v, SRCE )ι:AnyAwait<T>{sl},_v{move(v)}{}
		α Suspend()ι->void override{
			std::thread{ [this]{
				std::this_thread::sleep_for( 1ms );	//force a real suspension before the resume.
				this->Resume( move(_v) );
			} }.detach();
		}
		T _v;
	};
	struct ReadyAny final : AnyAwait<int>{
		ReadyAny( int v, SRCE )ι:AnyAwait<int>{sl}{ _result = v; }
		α await_ready()ι->bool override{ return true; }
		α Suspend()ι->void override{ _suspended = true; }
		bool _suspended{};
	};
	struct ReadyErrorAny final : AnyVoidAwait{
		ReadyErrorAny( SRCE )ι:AnyVoidAwait{sl}{ _error = TestException{"pre-completed"}.Move(); }
		α await_ready()ι->bool override{ return true; }
		α Suspend()ι->void override{ _suspended = true; }
		bool _suspended{};
	};

	//old-style awaitables - bridged with Any().
	struct OldInt final : TAwait<int>{
		OldInt( int v, SRCE )ι:TAwait<int>{sl},_v{v}{}
		α Suspend()ι->void override{ std::thread{ [this]{ Resume( int{_v} ); } }.detach(); }
		int _v;
	};
	struct OldVoid final : VoidAwait{
		OldVoid( SRCE )ι:VoidAwait{sl}{}
		α Suspend()ι->void override{ Resume(); }
	};
	struct OldThrower final : VoidAwait{
		OldThrower( SRCE )ι:VoidAwait{sl}{}
		α Suspend()ι->void override{ ResumeExp( TestException{"old-style boom"} ); }
	};

	//One coroutine awaits int, string, and void awaitables - with the IAwait family each value type would
	//need its own coroutine (the awaitable's ::Task must be the caller's return type).
	Ω mixedVoidTask( std::atomic<uint>& completed, std::atomic<bool>& ok )->VoidTask{
		let i = co_await InlineAny<int>{ 42 };
		let s = co_await InlineAny<string>{ "abc" };
		co_await InlineVoidAny{};
		ok = i==42 && s=="abc";
		++completed;
	}
	Ω mixedTypedTask( std::atomic<uint>& completed, std::atomic<bool>& ok )->TTask<uint32>{
		let i = co_await InlineAny<int>{ 7 };	//awaited value type != the task's result type - the caller's promise is not the mailbox here.
		ok = i==7;
		++completed;
	}
	TEST_F( AnyAwaitTests, MixedTypesInOneCoroutine ){
		std::atomic<uint> completed{}; std::atomic<bool> ok{}, okTyped{};
		mixedVoidTask( completed, ok );
		mixedTypedTask( completed, okTyped );
		ASSERT_TRUE( settle(completed, 2) ) << "a coroutine never completed";
		EXPECT_TRUE( ok.load() );
		EXPECT_TRUE( okTyped.load() );
	}

	Ω adapterBridges( std::atomic<uint>& completed, std::atomic<bool>& ok )->VoidTask{
		let i = co_await Any( OldInt{7} );	//rvalue - the adapter owns the inner awaitable.
		OldInt local{ 9 };
		let j = co_await Any( local );	//lvalue - the adapter holds a reference.
		co_await Any( OldVoid{} );
		ok = i==7 && j==9;
		++completed;
	}
	TEST_F( AnyAwaitTests, AdapterBridgesExistingAwaitables ){
		std::atomic<uint> completed{}; std::atomic<bool> ok{};
		adapterBridges( completed, ok );
		ASSERT_TRUE( settle(completed, 1) ) << "the adapter never resumed its awaiter";
		EXPECT_TRUE( ok.load() );
	}

	//The relay is promise.SetExp -> glue catch -> adapter ResumeExp -> await_resume rethrow; every hop uses
	//the virtual Move/Throw, so the concrete type must survive to the handler.
	Ω exceptionSurvives( std::atomic<uint>& completed, std::atomic<bool>& ok )->VoidTask{
		try{
			co_await Any( OldThrower{} );
		}
		catch( TestException& ){ ok = true; }
		catch( Exception& ){ ok = false; }	//sliced somewhere in the relay.
		++completed;
	}
	TEST_F( AnyAwaitTests, ExceptionSubclassSurvivesTheAdapter ){
		std::atomic<uint> completed{}; std::atomic<bool> ok{};
		exceptionSurvives( completed, ok );
		ASSERT_TRUE( settle(completed, 1) ) << "the exception path never resumed the awaiter";
		EXPECT_TRUE( ok.load() ) << "TestException arrived sliced to a base type";
	}

	Ω crossThread( std::atomic<uint>& completed, std::atomic<bool>& ok )->VoidTask{
		let s = co_await ThreadedAny<string>{ "from another thread" };
		let i = co_await Any( OldInt{3} );	//the old-style inner also resumes from its own thread.
		ok = s=="from another thread" && i==3;
		++completed;
	}
	TEST_F( AnyAwaitTests, CrossThreadResume ){
		std::atomic<uint> completed{}; std::atomic<bool> ok{};
		crossThread( completed, ok );
		ASSERT_TRUE( settle(completed, 1) ) << "a cross-thread resume was lost";
		EXPECT_TRUE( ok.load() );
	}

	//await_ready()==true consumes the pre-set result/error without ever suspending - possible because the
	//storage lives in the awaitable; the IAwait family has nowhere to park a ready value.
	Ω preCompleted( std::atomic<uint>& completed, std::atomic<bool>& ok, std::atomic<bool>& suspendCalled )->VoidTask{
		ReadyAny ready{ 5 };
		let i = co_await ready;
		bool threw{};
		ReadyErrorAny error;
		try{
			co_await error;
		}
		catch( TestException& ){ threw = true; }
		ok = i==5 && threw;
		suspendCalled = ready._suspended || error._suspended;
		++completed;
	}
	TEST_F( AnyAwaitTests, PreCompletedShortCircuits ){
		std::atomic<uint> completed{}; std::atomic<bool> ok{}, suspendCalled{};
		preCompleted( completed, ok, suspendCalled );
		ASSERT_TRUE( settle(completed, 1) );
		EXPECT_TRUE( ok.load() );
		EXPECT_FALSE( suspendCalled.load() ) << "await_ready()==true must skip Suspend entirely";
	}
}
