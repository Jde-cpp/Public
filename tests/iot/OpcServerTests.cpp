#include "gtest/gtest.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include "helpers.h"

#define var const auto
namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag("tests") };

	class OpcServerTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{};
		static uint OpcProviderId;
	};
	uint OpcServerTests::OpcProviderId{};

	TEST_F( OpcServerTests, Crud ){
		var id = Iot::CreateOpcServer();
		
		var selectAll = "query{ opcServers { id name attributes created updated deleted target description certificateUri isDefault url } }";
		var selectAllJson = DB::Query( selectAll, 0 );
		TRACE( "{}", selectAllJson.dump() );		


		var readJson = Iot::SelectOpcServer( id );
		var readId = readJson["id"].get<int>();
		ASSERT_EQ( id, readId );

		var update = Jde::format( "{{ mutation {{ updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }} }}", readId, "newDescription" );
		var updateJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		TRACE( "{}", updateJson.dump() );		

		var del = Jde::format( "{{mutation {{ deleteUser('id':{}) }} }}", readId );
		var deleteJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		TRACE( "{}", deleteJson.dump() );

		Iot::PurgeOpcServer( id );
	}

	
	//
	//
	//
	//{ mutation { deleteOpcServer("id":1) } }
}