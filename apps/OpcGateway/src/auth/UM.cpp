#include "UM.h"
#include "../UAClient.h"
#include <jde/ql/IQL.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/appClient.h>

#define let const auto

namespace Jde::Opc::Gateway{
	α ProviderSelectAwait::Select()ι->TAwait<jobject>::Task{
		try{
			let query = Ƒ( "provider(target:\"{}\", providerTypeId:{}){{ id }}", _opcId, (uint8)Access::EProviderType::OpcServer );
			let j = co_await *App::Client::QLServer()->QueryObject( query, App::Client::AppServiceUserPK() );
			let providerId = Json::FindNumber<Access::ProviderPK>( j, "id" ).value_or(0);
			ResumeScaler( providerId );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderCreatePurgeAwait::Execute( OpcClientPK opcPK )ι->TAwait<vector<OpcClient>>::Task{
		try{
			auto server = co_await OpcClientAwait{ opcPK, true };
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
		let q = Jde::format( "createProvider( target:\"{}\", providerType:\"OpcServer\" ){{id}}", target );
		try{
			let j = co_await *App::Client::QLServer()->QueryObject( q, App::Client::AppServiceUserPK() );
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
			co_await *App::Client::QLServer()->Query( q, App::Client::AppServiceUserPK() );
			ResumeScaler( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Suspend()ι->void{
		if( _opcKey.IsPrimary() )
			Execute( _opcKey.PK() );
		else if( _insert )
			Insert( _opcKey.NK() );
		else
			Purge( _opcKey.NK() );
	}
}