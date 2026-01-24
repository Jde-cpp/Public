#include "UM.h"
#include <jde/ql/IQL.h>
#include <jde/app/client/AppClientSocketSession.h>
//#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include "../StartupAwait.h"
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
	α ProviderSelectAwait::Select()ι->TAwait<jobject>::Task{
		try{
			let query = Ƒ( "provider(name:\"{}\", providerTypeId:{}){{ id }}", _opcId, (uint8)Access::EProviderType::OpcServer );
			auto appClient = AppClient();
			let j = co_await *appClient->QLServer()->QueryObject( query, {}, appClient->UserPK() );
			let providerId = Json::FindNumber<Access::ProviderPK>( j, "id" ).value_or(0);
			ResumeScaler( providerId );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderCreatePurgeAwait::Execute( ServerCnnctnPK opcPK )ι->TAwait<vector<ServerCnnctn>>::Task{
		try{
			auto server = co_await ServerCnnctnAwait{ opcPK, true };
			THROW_IF( server.empty(), "[{}]Could not find OpcServer", opcPK );
			if( _insert )
				Insert( move(server.front().Target) );
			else
				Purge( move(server.front().Target) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Insert( str target )ι->TAwait<jobject>::Task{
		let q = Ƒ( "createProvider( target:\"{}\", providerType:\"OpcServer\" ){{id}}", target );
		try{
			auto appClient = AppClient();
			let j = co_await *appClient->QLServer()->QueryObject( q, {}, appClient->UserPK() );
			let newPK = QL::AsId<Access::ProviderPK>( j );
			ResumeScaler( newPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderCreatePurgeAwait::Purge( str target )ι->ProviderSelectAwait::Task{
		try{
			let providerPK = co_await ProviderSelectAwait{ target };
			Purge( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Purge( Access::ProviderPK providerPK )ι->TAwait<jvalue>::Task{
		let q = Ƒ( "purgeProvider( id:{} )", providerPK );
		try{
			auto appClient = AppClient();
			co_await *appClient->QLServer()->Query( q, {}, appClient->UserPK() );
			ResumeScaler( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Suspend()ι->void{
		if( _opcKey.IsPK() )
			Execute( _opcKey.PK() );
		else if( _insert )
			Insert( _opcKey.NK() );
		else
			Purge( _opcKey.NK() );
	}
}