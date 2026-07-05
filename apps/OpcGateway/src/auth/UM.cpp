#include "UM.h"
#include <jde/ql/IQL.h>
#include <jde/app/client/AppClientSocketSession.h>
//#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include "../StartupAwait.h"
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
	α ProviderAwait::Execute()ι->TAwait<jobject>::Task{
		try{
			constexpr auto q = "provider(name:$opcTarget){ id }";
			jobject vars{ {"opcTarget", _opcId} };
			let j = co_await *AppClient()->Query( q, move(vars) );
			let providerId = Json::FindNumber<Access::ProviderPK>( j, "id" ).value_or(0);
			ResumeScaler( providerId );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderMAwait::Execute( ServerCnnctnPK opcPK )ι->TAwait<vector<ServerCnnctn>>::Task{
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
	α ProviderMAwait::Insert( str target )ι->TAwait<jobject>::Task{
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

	α ProviderMAwait::Purge( str target )ι->ProviderAwait::Task{
		try{
			let providerPK = co_await ProviderAwait{ target };
			if( !providerPK )
				ResumeScaler( providerPK );
			else
				Purge( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderMAwait::Purge( Access::ProviderPK providerPK )ι->TAwait<jvalue>::Task{
		ASSERT( providerPK );
		jobject vars{ {"id", providerPK} };
		constexpr auto q = "purgeProvider( id:$id )";
		try{
			auto appClient = AppClient();
			co_await *appClient->QLServer()->Query( q, move(vars), appClient->UserPK() );
			ResumeScaler( providerPK );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderMAwait::Suspend()ι->void{
		if( _opcKey.IsPK() )
			Execute( _opcKey.PK() );
		else if( _insert )
			Insert( _opcKey.NK() );
		else
			Purge( _opcKey.NK() );
	}
}