#include <jde/access/awaits/ConfigureAwait.h>

#include <jde/access/Authorize.h>
#include "../accessInternal.h"
#include "IdentityLoadAwait.h"
#include "AclLoadAwait.h"
#include "ResourceLoadAwait.h"
#include "RoleLoadAwait.h"
#include "EventsSubscribeAwait.h"

#define let const auto

namespace Jde::Access{
	struct Loader final{
		Ω Users( ConfigureAwait& await )ι->IdentityLoadAwait::Task;
	private:
		Ω Acl( ConfigureAwait& await )->AclLoadAwait::Task;
		Ω Roles( ConfigureAwait& await )ι->RoleLoadAwait::Task;
		Ω Resources( ConfigureAwait& await )ι->ResourceLoadAwait::Task;
		Ω Subscribe( ConfigureAwait& await )ι->EventsSubscribeAwait::Task;
	};

	α Loader::Subscribe( ConfigureAwait& await )ι->EventsSubscribeAwait::Task{
		try{
			co_await EventsSubscribeAwait{ await.QlServer, await.Executer };
			await.Resume();
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}


	α Loader::Acl( ConfigureAwait& await )->AclLoadAwait::Task{
		try{
			let acl = co_await AclLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Acl = move( acl );
			Authorizer().SetUserPermissions( {}, l );
			Subscribe( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

  α Loader::Roles( ConfigureAwait& await )ι->RoleLoadAwait::Task{
		try{
			auto roles = co_await RoleLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Roles = move( roles );
			l.unlock();
			Acl( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α Loader::Resources( ConfigureAwait& await )ι->ResourceLoadAwait::Task{
		try{
			auto loaded = co_await ResourceLoadAwait{ await.QlServer, await.Schemas, await.Executer };
			ul l{ Authorizer().Mutex };
			for( let& [pk, resource] : loaded.Resources ){
				if( resource.Filter.empty() && !resource.IsDeleted ){
					auto& namePK = Authorizer().SchemaResources.emplace( resource.Schema, flat_map<string,ResourcePK>{} ).first->second;
					namePK.emplace( resource.Target, pk );
				}
				Authorizer().Resources.emplace( pk, move(resource) );
			}
			Authorizer().Permissions = move( loaded.Permissions );
			//Trace( ELogTags::Test, "{}", (uint)Authorizer().Permissions.at(10).Allowed );
			l.unlock();
			Roles( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α Loader::Users( ConfigureAwait& await )ι->IdentityLoadAwait::Task{
		try{
			auto identities = co_await IdentityLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Users = std::move( identities.Users );
			Authorizer().Groups = std::move( identities.Groups );
			l.unlock();
			Resources( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ConfigureAwait::Suspend()ι->void{
		Loader::Users( *this );
	};
}