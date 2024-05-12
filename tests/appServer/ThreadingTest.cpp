#include "gtest/gtest.h"
#include "../../../AppServer/source/LogClient.h"
#include "../../../Framework/source/DateTime.h"
//#include "../../../Framework/source/log/server/ServerSink.h"

#define var const auto
namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	class ThreadingTest : public ::testing::Test{
	protected:
		ThreadingTest() {}
		~ThreadingTest() override{}

		void SetUp() override {}
		void TearDown() override {}
	};

	TEST_F( ThreadingTest, Main ){
		auto threadRun = []( uint iThread ){
			Jde::Threading::SetThreadDscrptn( std::to_string(iThread) );
			for( uint i=0; i< 1<<6; ++i ){
				string message = "Message";
				if( i%7 )
					message += "threadIndex='{}'";
				if( i%7>1 )
					message += ", date='{}'";
				if( i%7>3 )
					message += ", threadId='{}'";
				if( i%7>4 )
					message += ", threadName='{}'";
				if( i%7>5 )
					message += ", index='{}'";
				if( i%7==6 )
					message += ", random={}";
				vector<string> params; params.reserve( i%7 );
				for( uint iVariable=0; iVariable<i%7; ++iVariable ){
					if( iVariable==0 )
						params.push_back( std::to_string(iThread) );
					else if( iVariable==1 )
						params.push_back( ToIsoString(Clock::now()) );
					else if( iVariable==2 )
						params.push_back( std::to_string(Threading::GetThreadId()) );
					else if( iVariable==3 )
						params.push_back( Threading::GetThreadDescription() );
					else if( iVariable==4 )
						params.push_back( std::to_string(i) );
					else if( iVariable==5 )
						params.push_back( std::to_string(std::rand()) );
				}
				Logging::Message msg{ ELogLevel::Debug, message };
				//Logging::LogClient::Instance().Log( msg, params );
				std::this_thread::yield();
			}
		};

		vector<jthread> threads;
		for( uint i=0;i<1; ++i )
			threads.push_back( jthread{ [i,threadRun]{threadRun(i);} } );
		for( auto& t : threads )
			t.join();
		DBG( "Exit"sv );
	}
}