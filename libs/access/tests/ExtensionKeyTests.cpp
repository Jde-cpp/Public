//ql-review3 #24: users extends identities, and users.identity_id is that extension's pk - a value the insert about to run
//produces, never one the client picks.  InsertAwait honoured `identityId:N`, `id:N` and `identity:{id:N}` for it, so a caller
//with Create on users could bind their new row - their login_name - to somebody else's identity, while the identities row the
//same mutation inserted was left an orphan.  There is still no transaction (#11), so nothing rolls that orphan back.
#include "gtest/gtest.h"
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	struct ExtensionKeyTests : ::testing::Test{
		α SetUp()->void override{ _victim = UserPK{ GetId(GetUser("review24-victim", GetRoot())) }; }
		α TearDown()->void override{ PurgeUser( _victim, GetRoot() ); }
		Ω loginOf( UserPK identity )ε->string{
			let user = Select( "user", identity.Value, GetRoot(), "id loginName", true );
			return user.empty() || !user.contains("loginName") || user.at("loginName").is_null() ? string{} : string{ user.at("loginName").as_string() };
		}
		//the created row is looked up by target rather than read out of the mutation's result: a multi-statement insert
		//answers with one entry per statement, and which one carries the id is not this test's subject.
		α create( str extraArg )ε->uint{
			let target = "review24-"+std::to_string( ++_n );
			let m = "mutation createUser( "+extraArg+"name:\""+target+"\", target:\""+target+"\", loginName:\""+target+"-login\", providerId:1 )";
			QL().QuerySync<jvalue>( m, {}, GetRoot() );
			let id = GetId( Select("user", target, GetRoot(), "id", true) );
			_created.push_back( UserPK{id} );
			return id;
		}
		α TearDownCreated()ε->void{ for( let u : _created ) PurgeUser( u, GetRoot() ); }
		UserPK _victim;
		vector<UserPK> _created;
		uint _n{};
	};

	TEST_F( ExtensionKeyTests, ClientSuppliedExtensionKeyIsIgnored ){
		let victimLogin = loginOf( _victim );
		//`id:N` is deliberately not here - see the review note: that branch still seeds the *base* table's pk, because the
		//schema's own enum seed (createRights( name:"None", id:0 )) inserts through it.
		for( let& arg : {"identityId:"+std::to_string(_victim.Value)+", ", "identity:{id:"+std::to_string(_victim.Value)+"}, "} ){
			let id = create( arg );
			EXPECT_NE( id, _victim.Value ) << "the new row took the victim's identity: " << arg;
			EXPECT_EQ( loginOf(_victim), victimLogin ) << "the victim's login was overwritten: " << arg;
			EXPECT_FALSE( loginOf(UserPK{(uint32)id}).empty() ) << "the new identity has no users row - it was left an orphan: " << arg;
		}
		TearDownCreated();
	}
	//the control: the shape the ui sends still works, and still gets a fresh identity.
	TEST_F( ExtensionKeyTests, PlainCreateStillWorks ){
		let first = create( "" );
		let second = create( "" );
		EXPECT_NE( first, second );
		EXPECT_FALSE( loginOf(UserPK{(uint32)first}).empty() );
		TearDownCreated();
	}
}
