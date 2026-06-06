#include "../src/access/UAAccess.h"
#define let const auto

namespace Jde::Opc::Server::Tests{
	using UAAccess::EOpcAccessLevel;
	constexpr ELogTags _tags{ ELogTags::Test };
	struct AccessTests : ::testing::Test{
	protected:
		constexpr static EOpcAccessLevel _readerAllowed = EOpcAccessLevel::Read | EOpcAccessLevel::HistoryRead;
		constexpr static EOpcAccessLevel _writerAdded =  EOpcAccessLevel::Write | EOpcAccessLevel::HistoryWrite | EOpcAccessLevel::StatusWrite | EOpcAccessLevel::TimestampWrite;
		constexpr static EOpcAccessLevel _writerAllowed =  _readerAllowed | _writerAdded;
		constexpr static EOpcAccessLevel _adminAdded = EOpcAccessLevel::SemanticChange;
		constexpr static EOpcAccessLevel _adminAllowed = _writerAllowed | _adminAdded;
		constexpr static EOpcAccessLevel _readerDenied = _writerAdded | _adminAdded;
		constexpr static EOpcAccessLevel _writerDenied = _adminAdded;
		constexpr static EOpcAccessLevel _adminDenied = EOpcAccessLevel::AllAccess;

		Ω AddRole( const string& target, EOpcAccessLevel allowed, EOpcAccessLevel denied )ε->void{
			let userTarget = Ƒ("{}User", target);
			let user = _app->QuerySync<jobject>( "createUser( target:$target, name:$name ){id}", {{"target", userTarget}, {"name", userTarget+" name"}} );
			let userId = Json::AsNumber<uint>( user.at("id") );
			_users.emplace( userTarget, userId );

			let roleTarget = DB::Names::Capitalize( target );
			let role = _app->QuerySync<jobject>( "createRole( target:$target, name:$name ){id}", {{"target",roleTarget}, {"name", roleTarget+" name"}} );
			let roleId = Json::AsNumber<Access::RolePK>( role.at("id") );
			_roles.emplace( target, roleId );

			jobject vars{ {"roleId", roleId}, {"allowed", underlying(allowed)}, {"denied", underlying(denied)}, {"schema", _resource} };
			string query{ "addRole( id:$roleId, permissionRight:{allowed:$allowed, denied:$denied, resource:{schemaName:$schema, target:\"nodeIds\"}} )" };
			_app->QuerySync<jvalue>( move(query), move(vars) );

			vars = { {"roleId", roleId}, {"allowed", underlying(allowed) & ~UA_ACCESSLEVELMASK_HISTORYREAD}, {"denied", 0}, {"schema", _resource}, {"criteria", "ns=4;i=6020"}, {"resourceName", "SignalOn"} }; //Examples/Stacklights/ExampleStacklight/Lamp1/SignalOn
			query = "addRole( id:$roleId, permissionRight:{allowed:$allowed, denied:$denied, resource:{schemaName:$schema, target:\"nodeIds\", criteria:$criteria, name:$resourceName}} )";
			_app->QuerySync<jvalue>( move(query), move(vars) );

			_app->QuerySync<jvalue>( "createAcl( identity:{ id:$userId }, role:{id:$roleId} )", {{"userId", userId}, {"roleId", roleId}} );
		}
		Ω SetUpTestCase()ε->void{
			Server::Initialize( ServerId(), GetSchemaPtr() );
			_ua = &Server::GetUAServer();
			_app = AppClient();

			let jroles = _app->QuerySync<jarray>( "roles(){ id target }", {} );
			for( let& jrole : jroles )
				_roles.emplace( jrole.at("target").get_string(), jrole.at("id").to_number<Access::RolePK>() );
			if( !_roles.contains("opcTestReaders") ){
				_app->QuerySync<jvalue>( "createAcl( identity:{id:$testProgUser}, permissionRight:{ allowed:$allowed, denied:0, resource:{schemaName: $schemaName, target:$nodeResTarget}} )",
					{ {"testProgUser", AppClient()->UserPK().Value}, {"allowed", underlying(EOpcAccessLevel::All)}, {"schemaName", _resource}, {"nodeResTarget", "nodeIds"} } );
				_app->QuerySync<jvalue>( "restoreResource( target:$target, criteria:null ){id}", {{"target","nodeIds"}} );

				AddRole( "reader", _readerAllowed, _readerDenied );
			}
			else{
				let jusers = _app->QuerySync<jarray>( "users(){ id, target }", {} );
				for( let& juser : jusers )
					_users.emplace( juser.at("target").get_string(), juser.at("id").to_number<UserPK::Type>() );
			}
			if( !_roles.contains("opcTestWritters") )
				AddRole( "writer", _writerAllowed, _writerDenied );
			if( !_roles.contains("opcTestAdmins") )
				AddRole( "admin", _adminAllowed, _adminDenied );
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
		UAAccess::SessionContext ctx{ "", TimePoint::max(), 0, {_users.at("readerUser")} };
		let nodeId = UA_NODEID_NUMERIC( 4, 6020 );
		let accessLevel = (EOpcAccessLevel)UAAccess::GetUserAccessLevel( _ua->Ptr(), nullptr, nullptr, &ctx, &nodeId, nullptr );
		EXPECT_EQ( accessLevel, _readerAllowed );
	}
	TEST_F( AccessTests, Query ){
		auto q = "roles{ id name permissionRight{id allowed denied resource(schemaName:$schemaName, target:$target, criteria:$criteria){id criteria}} }";
		jobject vars{ {"schemaName", "opc.default"}, {"target", "nodeIds"}, {"criteria", jarray{jvalue{}}} };
		TRACE( "{}", q );
		TRACE( "{}", serialize(vars) );
		let result = _app->QuerySync<jvalue>( move(q), move(vars) );
		TRACE( "{}", serialize(result) );
	}
}
