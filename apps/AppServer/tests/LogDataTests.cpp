#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "helpers.h"
#include "../src/LogData.h"
#define let const auto

//the connections bookkeeping in LogData.cpp: AddConnection's proc-minted pks and EndConnection's deleted stamp.
namespace Jde::App::Server::Tests{

	struct LogDataTests : ::testing::Test{
		Ω ConnectionsTable()->string{ return Server::AppSchema()->GetView("connections").DBName; }
		Ω LiveCount( App::ConnectionPK connectionId )->uint{
			auto rows = Server::AppSchema()->DS()->Select( {Ƒ("select count(*) from {} where connection_id=? and deleted is null", ConnectionsTable()), {DB::Value{connectionId}}} );
			return rows.empty() ? 0u : (uint)rows[0].GetInt( 0 );
		}
	};

	TEST_F( LogDataTests, AddConnectionMintsPks ){
		let [program, instance, connection] = App::AddConnection( "Tests.LogData", "mints", "logdata-host", 111 );
		EXPECT_NE( program, 0u );
		EXPECT_NE( instance, 0u );
		EXPECT_NE( connection, 0u );
		EXPECT_EQ( LiveCount(connection), 1u );
	}

	//same program+instance+host reuses the program/instance rows; every call is a fresh connection row and - a
	//reconnect implies the old socket died - the insert proc stamps the instance's prior live connections deleted.
	TEST_F( LogDataTests, AddConnectionReusesProgramAndInstance ){
		let [program1, instance1, connection1] = App::AddConnection( "Tests.LogData", "reuses", "logdata-host", 112 );
		let [program2, instance2, connection2] = App::AddConnection( "Tests.LogData", "reuses", "logdata-host", 112 );
		EXPECT_EQ( program1, program2 );
		EXPECT_EQ( instance1, instance2 );
		EXPECT_NE( connection1, connection2 );
		EXPECT_EQ( LiveCount(connection1), 0u ) << "a reconnect supersedes the instance's previous connection";
		EXPECT_EQ( LiveCount(connection2), 1u );

		let [program3, instance3, connection3] = App::AddConnection( "Tests.LogData", "reuses-other", "logdata-host", 113 );
		EXPECT_EQ( program1, program3 );
		EXPECT_NE( instance1, instance3 );
		EXPECT_NE( connection2, connection3 );
	}

	//EndConnection ends one *connection* - another instance's live connection must not be stamped.
	TEST_F( LogDataTests, EndConnectionStampsDeleted ){
		let [program, instance, connection] = App::AddConnection( "Tests.LogData", "ends", "logdata-host", 114 );
		let [program2, instance2, survivor] = App::AddConnection( "Tests.LogData", "ends-other", "logdata-host", 115 );
		ASSERT_EQ( LiveCount(connection), 1u );
		App::EndConnection( connection );//fire-and-forget task - poll for the stamp.
		for( let expiration = steady_clock::now()+std::chrono::seconds{5}; LiveCount(connection) && steady_clock::now()<expiration; )
			std::this_thread::sleep_for( std::chrono::milliseconds{50} );
		EXPECT_EQ( LiveCount(connection), 0u );
		EXPECT_EQ( LiveCount(survivor), 1u ) << "only the ended connection may be stamped";
	}
}
