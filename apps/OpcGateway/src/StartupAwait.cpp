#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/opc/opc.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/opc/uatypes/UAClient.h>
#include "WebServer.h"

#define let const auto
namespace Jde::Opc{
	static sp<QL::LocalQL> _localQL;
	static sp<Access::AccessListener> _listener;
	α Gateway::QL()ι->QL::LocalQL&{ return *_localQL; }
	α Gateway::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return _localQL->Schemas(); }
}

namespace Jde::Opc::Gateway{
	sp<Access::Authorize> _authorizer = ms<Access::Authorize>();

	α StartupAwait::Execute()ι->VoidAwait<>::Task{
		try{
			auto remoteAcl = App::Client::RemoteAcl();
			auto schema = DB::GetAppSchema( "gateway", remoteAcl );
			_localQL = QL::Configure( {schema}, _authorizer );
			Opc::Configure( schema );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema, _localQL );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *schema, _localQL );

			Crypto::CryptoSettings settings{ "http/ssl" };
			if( !fs::exists(settings.PrivateKeyPath) ){
				settings.CreateDirectories();
				Crypto::CreateKeyCertificate( settings );
			}
			Opc::StartWebServer( move(_webServerSettings) );
			auto accessSchema = DB::GetAppSchema( "access", remoteAcl );
			co_await App::Client::ConnectAwait{ {accessSchema} };

			_listener = ms<Access::AccessListener>( App::Client::QLServer() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});
			co_await Access::Client::Configure( accessSchema, {schema}, App::Client::QLServer(), UserPK{UserPK::System}, _authorizer, _listener );
			Process::AddShutdownFunction( [](bool terminate){Opc::UAClient::Shutdown(terminate);} );
			QL::Hook::Add( mu<Opc::OpcQLHook>() );
			Information( ELogTags::App, "---Started {}---", Process::ProductName() );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}