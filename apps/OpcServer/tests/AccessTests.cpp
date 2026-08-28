#include "../src/access/UAAccess.h"
#include "../src/access/OpcAuthorize.h"
#include "../src/ql/OpcQL.h"
#include <jde/ql/ql.h>
#define let const auto

namespace Jde::Opc::Server::Tests{
	using Access::ERights;
	constexpr ELogTags _tags{ ELogTags::Test };
	//access-review3 #4:  node ACLs are stored in the generic ERights vocabulary - what node-access.ts writes - and OpcAuthorize
	//translates them to UA access-level bits (ToAccess).  These seeds used to be UA bits, which the old C-cast passed through.
	struct AccessTests : ::testing::Test{
	protected:
		constexpr static ERights _readerAllowed = ERights::Read;
		constexpr static ERights _writerAllowed = ERights::Read | ERights::Update;
		constexpr static ERights _adminAllowed = _writerAllowed | ERights::Administer;
		constexpr static ERights _readerDenied = ERights::Update | ERights::Delete | ERights::Administer;
		constexpr static ERights _writerDenied = ERights::Administer;
		constexpr static ERights _adminDenied = ERights::None;

		Ω addRole( const string& target, ERights allowed, ERights denied )ε->void{
			let userTarget = Ƒ( "{}User", target );

			auto user = _app->QuerySync( "user(target:$target){id}", {{"target", userTarget}} );
			if( user.empty() )
				user = _app->QuerySync<jobject>( "createUser( target:$target, name:$name ){id}", {{"target", userTarget}, {"name", userTarget+" name"}} );
			let userId = Json::AsNumber<uint>( user.at("id") );
			_users.emplace( userTarget, userId );

			let roleTarget = DB::Names::Capitalize( target );
			//auto role = _app->QuerySync("role(target:$target){id}", {{"target", roleTarget}});
			//if( role.empty() )
			auto role = _app->QuerySync<jobject>( "createRole( target:$target, name:$name ){id}", {{"target",roleTarget}, {"name", roleTarget+" name"}} );
			let roleId = Json::AsNumber<Access::RolePK>( role.at("id") );
			_roles.emplace( target, roleId );

			jobject vars{ {"roleId", roleId}, {"allowed", underlying(allowed)}, {"denied", underlying(denied)}, {"schema", _resource} };
			string query{ "addRole( id:$roleId, permissionRight:{allowed:$allowed, denied:$denied, resource:{schemaName:$schema, target:\"nodeIds\"}} )" };
			_app->QuerySync<jvalue>( move(query), move(vars) );

			vars = { {"roleId", roleId}, {"allowed", underlying(allowed)}, {"denied", underlying(denied)}, {"schema", _resource}, {"criteria", "ns=4;i=6020"}, {"resourceName", "SignalOn"} }; //Examples/Stacklights/ExampleStacklight/Lamp1/SignalOn - the same rights as the root: the generic vocabulary has no separate history-read right to withhold here.
			query = "addRole( id:$roleId, permissionRight:{allowed:$allowed, denied:$denied, resource:{schemaName:$schema, target:\"nodeIds\", criteria:$criteria, name:$resourceName}} )";
			_app->QuerySync<jvalue>( move(query), move(vars) );

			_app->QuerySync<jvalue>( "createAcl( identity:{ id:$userId }, role:{id:$roleId} )", {{"userId", userId}, {"roleId", roleId}} );
		}
		Ω SetUpTestCase()ε->void{
			Server::Initialize( ServerId(), GetSchemaPtr() );
			_ua = &Server::GetUAServer();
			_app = AppClient();

			let nodeTarget = jobject{ {"target","nodeIds"} };
			_app->QuerySync<jvalue>( "deleteResource( target:$target, criteria:null )", nodeTarget );
			let jroles = _app->QuerySync<jarray>( "roles(){ id target }", {} );
			for( let& jrole : jroles )
				_roles.emplace( jrole.at("target").get_string(), jrole.at("id").to_number<Access::RolePK>() );
			if( !_roles.contains("opcTestReaders") ){
				_app->QuerySync<jvalue>( "createAcl( identity:{id:$testProgUser}, permissionRight:{ allowed:$allowed, denied:0, resource:{schemaName: $schemaName, target:$nodeResTarget}} )",
					{ {"testProgUser", AppClient()->UserPK().Value}, {"allowed", underlying(ERights::All)}, {"schemaName", _resource}, {"nodeResTarget", "nodeIds"} } );
				addRole( "reader", _readerAllowed, _readerDenied );
			}
			else{
				let jusers = _app->QuerySync<jarray>( "users(){ id, target }", {} );
				for( let& juser : jusers )
					_users.emplace( juser.at("target").get_string(), juser.at("id").to_number<UserPK::Type>() );
			}
			if( !_roles.contains("opcTestWritters") )
				addRole( "writer", _writerAllowed, _writerDenied );
			if( !_roles.contains("opcTestAdmins") )
				addRole( "admin", _adminAllowed, _adminDenied );
			_app->QuerySync<jvalue>( "restoreResource( target:$target, criteria:null )", nodeTarget );
		}
		Ω TearDownTestCase()ι->void{}
		α SetUp()ι->void{}
//	private:
		static sp<App::Client::IAppClient> _app;
		static inline flat_map<string, uint> _roles;
		static inline flat_map<string, uint> _users;
		static string _resource;
		static UAServer* _ua;
	};
	UAServer* AccessTests::_ua{ nullptr };
	sp<App::Client::IAppClient> AccessTests::_app{ AppClient() };
	string AccessTests::_resource{ "opc."+Settings::FindString("/opcServer/resource").value_or("test") };

	TEST_F( AccessTests, UserAccess ){
		let nodeId = UA_NODEID_NUMERIC( 4, 6020 );
		let accessLevel = [&]( const string& user ){
			UAAccess::SessionContext ctx{ "", TimePoint::max(), 0, {(UserPK::Type)_users.at(user)} };
			return (EAccess)UAAccess::GetUserAccessLevel( _ua->Ptr(), nullptr, nullptr, &ctx, &nodeId, nullptr );
		};
		EXPECT_EQ( accessLevel("readerUser"), ToAccess(_readerAllowed) ); //Read|HistoryRead...
		EXPECT_EQ( underlying(accessLevel("readerUser")) & UA_ACCESSLEVELMASK_WRITE, 0u ); //...and not WRITE, which the cast handed a reader (ERights::Read is 0x2).
		EXPECT_EQ( accessLevel("writerUser"), ToAccess(_writerAllowed) ); //+Write|HistoryWrite; cast raw, Read|Update=0x6 was WRITE|HISTORYREAD.
		EXPECT_EQ( accessLevel("adminUser"), ToAccess(_adminAllowed) ); //EAccess::All
	}

	//appserver-review3 #13:  the AppServer's delegated admin check, answered here (OpcServerQL's adminCheck →
	//OpcAuthorize::TestAdminNode):  who may grant on a node is whoever administers the resource governing it - its own row when
	//it has one, else the nearest configured ancestor's, else root.  The criteria row is created by SetUpTestCase after
	//AssignRights ran, so on a fresh db it is unmapped and inherits the root;  on a persisted one it is its own resource - the
	//roles grant the same rights on both, so the answers hold either way.
	TEST_F( AccessTests, DelegatedAdminAnswer ){
		auto isAdmin = [&]( const string& user, const string& criteria )->bool{
			jobject vars{ {"user", _users.at(user)}, {"resource", "nodeIds"}, {"criteria", criteria} };
			auto q = QL::Parse( "adminCheck( user:$user ){isAdmin resource( resource:$resource, criteria:$criteria )}", move(vars), {} );//as AppClientSocketSession::ClientQuery parses it: no schema.
			let y = _app->ClientQuery( move(q), _app->UserPK() )->await_resume();
			return Json::AsObject( y ).at( "adminCheck" ).at( "isAdmin" ).as_bool();
		};
		EXPECT_TRUE( isAdmin("adminUser", "") ) << "Administer on the root";
		EXPECT_FALSE( isAdmin("readerUser", "") );
		EXPECT_FALSE( isAdmin("writerUser", "") ) << "Administer denied explicitly";
		EXPECT_TRUE( isAdmin("adminUser", "ns=4;i=6020") );
		EXPECT_FALSE( isAdmin("readerUser", "ns=4;i=6020") );
		EXPECT_TRUE( isAdmin("adminUser", "ns=4;i=6021") ) << "unmapped - inherits what protects it";
		EXPECT_FALSE( isAdmin("readerUser", "ns=4;i=6021") );
	}

	//the mapping on its own, without a server.
	TEST( ToAccessTests, GenericRightsTranslateToAccessLevelBits ){
		static_assert( ToAccess(ERights::All)==EAccess::All );
		static_assert( ToAccess(ERights::None)==EAccess::None );
		EXPECT_EQ( ToAccess(ERights::Read), EAccess::Read | EAccess::HistoryRead );
		EXPECT_EQ( underlying(ToAccess(ERights::Read)) & UA_ACCESSLEVELMASK_WRITE, 0u );
		EXPECT_NE( underlying(ToAccess(ERights::Update)) & UA_ACCESSLEVELMASK_WRITE, 0u );
		EXPECT_EQ( underlying(ToAccess(ERights::Update)) & UA_ACCESSLEVELMASK_HISTORYREAD, 0u );
		EXPECT_EQ( underlying(ToAccess(ERights::Create)) & UA_ACCESSLEVELMASK_READ, 0u );
		EXPECT_EQ( ToAccess(ERights::Create | ERights::Purge | ERights::Subscribe | ERights::Execute), EAccess::None );
		EXPECT_EQ( ToAccess(ERights::Delete), EAccess::HistoryWrite );
		EXPECT_EQ( ToAccess(ERights::Administer), EAccess::StatusWrite | EAccess::TimestampWrite | EAccess::SemanticChange );
	}
	TEST_F( AccessTests, Query ){
		auto q = "roles{ id name permissionRight{id allowed denied resource(schemaName:$schemaName, target:$target, criteria:$criteria){id criteria}} }";
		jobject vars{ {"schemaName", "opc.default"}, {"target", "nodeIds"}, {"criteria", jarray{jvalue{}}} };
		TRACE( "{}", q );
		TRACE( "{}", serialize(vars) );
		let result = _app->QuerySync<jvalue>( move(q), move(vars) );
		TRACE( "{}", serialize(result) );
	}

	//ql-review3 #40: `nodeIds.guid` is EType::Guid, which ColumnQL::QLType has no graphql spelling for, so introspectFields
	//threw and `__type` answered "Query failed." for the whole document - for NodeId and, through Extends, for every node table.
	//This is the only schema in the repo with a Guid column, which is why the pin lives here;  the ql-side unit is
	//IntrospectionTests.AColumnWithNoQLTypeIsOmittedRatherThanFailingTheType.  Server::QL(), not _app: OpcServerAppClient routes
	//everything but log settings to the app server, whose schema has no node tables.
	TEST( IntrospectionTests, NodeIdTypeAnswersWithoutItsGuidColumn ){
		let y = Server::QL().QuerySync<jobject>( R"(__type(name:"NodeId"){ name fields{ name } })", {}, UserPK{UserPK::System} );
		EXPECT_EQ( Json::AsSV(y, "name"), "NodeId" );
		flat_set<string> names;
		for( let& f : Json::AsArray(y, "fields") )
			names.emplace( Json::AsSV(Json::AsObject(f), "name") );
		ASSERT_FALSE( names.empty() );
		EXPECT_FALSE( names.contains("guid") ) << Str::Join( names, "," );  //no graphql spelling - left out, not fatal.
		EXPECT_FALSE( names.contains("bytes") ) << Str::Join( names, "," ); //varbinary, skipped before this finding too.
	}
}
