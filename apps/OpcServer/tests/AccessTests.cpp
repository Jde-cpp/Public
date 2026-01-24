#include "../src/access/UAAccess.h"
#define let const auto

namespace Jde::Opc::Server::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct AccessTests : ::testing::Test{
	protected:
		constexpr static UA_UInt32 _readerAllowed = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_CURRENTREAD | UA_ACCESSLEVELMASK_HISTORYREAD;
		constexpr static UA_UInt32 _writerAdded =  UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_CURRENTWRITE | UA_ACCESSLEVELMASK_HISTORYWRITE | UA_ACCESSLEVELMASK_STATUSWRITE | UA_ACCESSLEVELMASK_TIMESTAMPWRITE;
		constexpr static UA_UInt32 _writerAllowed =  _readerAllowed | _writerAdded;
		constexpr static UA_UInt32 _adminAdded = UA_ACCESSLEVELMASK_SEMANTICCHANGE;
		constexpr static UA_UInt32 _adminAllowed = _writerAllowed | _adminAdded;
		constexpr static UA_UInt32 _readerDenied = _writerAdded | _adminAdded;
		constexpr static UA_UInt32 _writerDenied = _adminAdded;
		constexpr static UA_UInt32 _adminDenied = 0;

		Ω SetUpTestCase()ι->void{
			Server::Initialize( ServerId(), GetSchemaPtr() );
			_ua = &Server::GetUAServer();
			_app = AppClient();
			string target{ "testReaders" };
			auto role = _app->QuerySync<jobject>( "role( target: $target ){ id }", {{"target",target}} );
			jobject user;
			uint32 userId, roleId;

			let haveRole = !role.empty();
			if( !haveRole ){
				_app->QuerySync<jobject>( "createAcl( identity:{id:$testProgUser}, permissionRight:{ allowed:$allowed, denied:0, resource:{schemaName: $schemaName, target:$nodeResTarget}} )",
					{ {"testProgUser", AppClient()->UserPK().Value}, {"allowed", underlying(Access::ERights::All)}, {"schemaName", _resource}, {"nodeResTarget", "nodeIds"} } );
				_app->QuerySync<jobject>( "restoreResource( target:$target, criteria:null ){id}", {{"target","nodeIds"}} );
				role = _app->QuerySync<jobject>( "createRole( target:$target, name:$name ){id}", {{"target",target}, {"name", target+" name"}} );
				roleId = Json::AsNumber<uint>(role["id"]);
				user = _app->QuerySync<jobject>( "createUser( target:$target, name:$name ){id}", {{"target","readerUser"}, {"name","Reader User"}} );
				userId = Json::AsNumber<uint>(user["id"]);
				jobject vars{ {"roleId", roleId}, {"allowed", _readerAllowed}, {"denied", _readerDenied}, {"schema", _resource}, {"criteria", "ns=4;i=6020"}, {"resourceName", "InitialOperationDate"} };
				string query{ "addRole( id:$roleId, permissionRight:{allowed:$allowed, denied:$denied, resource:{schemaName:$schema, target:\"nodeIds\", criteria:$criteria, name:$resourceName}} )" };
				_app->QuerySync<jvalue>( move(query), move(vars) );
				_app->QuerySync<jvalue>( "createAcl( identity:{ id:$userId }, role:{id:$roleId} )", {{"userId", userId}, {"roleId", roleId}} );
			}
			else{
				roleId = Json::AsNumber<uint>(role["id"]);
				user = _app->QuerySync<jobject>( "user( target: $target ){ id }", {{"target","readerUser"}} );
				userId = Json::AsNumber<uint>(user["id"]);
			}
			_users.emplace( "readerUser", userId );
			_roles.emplace( move(target),  roleId );
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
		UAAccess::SessionContext ctx{ "", TimePoint::max(), 0, _users.at("readerUser") };
		let nodeId = UA_NODEID_NUMERIC( 4, 6020 );
		let accessLevel = UAAccess::GetUserAccessLevel( _ua->Ptr(), nullptr, nullptr, &ctx, &nodeId, nullptr );
		EXPECT_EQ( accessLevel, _readerAllowed );
	}
}
