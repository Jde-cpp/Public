#include "globals.h"
#include <jde/access/access.h>
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Test };
	using namespace Tests;

	uint _userId{};

	class AuthTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"AuthTests::OpcServer1"};
		static uint OpcProviderId;
	};
	std::condition_variable_any _cv;
	std::shared_mutex _mtx;
	uint AuthTests::OpcProviderId{};

	α AuthTests::SetUpTestCase()->void{
		if( DS().Scaler<uint>("select count(*) from um_providers where target=?", {DB::Value{string{OpcServer}}})==0 )
			DS().Execute( "insert into um_providers(provider_type_id, target) values(?, ?)", {DB::Value{(uint)EProviderType::OpcServer}, DB::Value{string{OpcServer}}} );
		OpcProviderId = *DS().Scaler<uint>( "select id from um_providers where target=?", {{string{OpcServer}}} );
	}

	α Login( str loginName, uint providerId, string opcServer )->Task{
		up<UserPK> pUserId = ( co_await Authenticate( loginName, providerId, opcServer) ).UP<UserPK>();
		_userId = *pUserId;
		std::shared_lock l{ _mtx };
		_cv.notify_one();
	}

	TEST_F( AuthTests, Login_Existing ){
		const string user{ "Login_Existing" };
		let provider = Access::EProviderType::Google;
		let userId = CreateUser( user, (uint)provider );
		Authenticate( user, (uint)provider, {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId );
	}

	TEST_F( AuthTests, Login_New ){
		const string user{ "Login_New" };
		let provider = Access::EProviderType::Google;
		Authenticate( user, (uint)provider, {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( _userId );
	}

TEST_F( AuthTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let userId = CreateUser( user, (uint)OpcProviderId );
		Authenticate( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId );
	}

	TEST_F( AuthTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		Authenticate( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( _userId );
	}
}