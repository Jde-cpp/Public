#include <jde/access/awaits/ConfigureAwait.h>

#include <jde/db/meta/AppSchema.h>
#include <jde/access/Authorize.h>
#include <jde/access/awaits/EventsSubscribeAwait.h>
#include "IdentityLoadAwait.h"
#include "AclLoadAwait.h"
#include "ResourceLoadAwait.h"
#include "RoleLoadAwait.h"


#define let const auto

namespace Jde::Access{
	struct Loader final{
		Ω Resources( ConfigureAwait& await )ι->TAwait<ResourcePermissions>::Task;
	private:
		Ω Acl( ConfigureAwait& await )->AclLoadAwait::Task;
		Ω Roles( ConfigureAwait& await )ι->RoleLoadAwait::Task;
		Ω Subscribe( ConfigureAwait& await )ι->EventsSubscribeAwait::Task;
	};

	α Loader::Subscribe( ConfigureAwait& await )ι->EventsSubscribeAwait::Task{
		try{
			vector<string> schemaNames;
			for( let& schema : await.Schemas )
				schemaNames.push_back( await.OpcServerInstance.size() ? Ƒ("{}.{}", schema->Name, await.OpcServerInstance ) : schema->Name );
			co_await EventsSubscribeAwait{ await.QlServer, schemaNames, await.Executer, await.Listener };
			await.Resume();
		}
		catch( exception& e ){
			await.ResumeExp( move(e) );
		}
	}


	α Loader::Acl( ConfigureAwait& await )->AclLoadAwait::Task{
		try{
			let acl = co_await AclLoadAwait{ await.QlServer, await.Executer };
			ul l{ await.Authorizer->Mutex };
			await.Authorizer->Acl = move( acl );
			await.Authorizer->SetUserPermissions( {}, l );
			Subscribe( await );
		}
		catch( exception& e ){
			await.ResumeExp( move(e) );
		}
	}

  α Loader::Roles( ConfigureAwait& await )ι->RoleLoadAwait::Task{
		try{
			auto roles = co_await RoleLoadAwait{ await.QlServer, await.Executer };
			ul l{ await.Authorizer->Mutex };
			await.Authorizer->Roles = move( roles );
			l.unlock();
			Acl( await );
		}
		catch( exception& e ){
			await.ResumeExp( move(e) );
		}
	}

	α Loader::Resources( ConfigureAwait& await )ι->TAwait<ResourcePermissions>::Task{
		try{
			auto loaded = co_await ResourceLoadAwait{ await.QlServer, await.Schemas, await.OpcServerInstance, await.Executer };
			ul l{ await.Authorizer->Mutex };
			for( let& [pk, resource] : loaded.Resources ){
				if( !resource.IsDeleted ){
					auto& targetResources = await.Authorizer->SchemaResources.try_emplace( resource.Schema ).first->second;
					auto& criteras = targetResources.try_emplace( resource.Target ).first->second;
					criteras.try_emplace( resource.Criteria, pk );
				}
				await.Authorizer->Resources.emplace( pk, move(resource) );
			}
			await.Authorizer->Permissions = move( loaded.Permissions );
			l.unlock();
			Roles( await );
		}
		catch( exception& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ConfigureAwait::SyncResources()ι->VoidTask{
		try{
			co_await ResourceSyncAwait{ QlServer, Schemas, OpcServerInstance, Executer };
			LoadUsers();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	};

	α ConfigureAwait::LoadUsers()ι->TAwait<Identities>::Task{
		try{
			auto identities = co_await IdentityLoadAwait{ QlServer, Executer };
			ul l{ Authorizer->Mutex };
			Authorizer->Users = std::move( identities.Users );
			Authorizer->Groups = std::move( identities.Groups );
			l.unlock();
			Loader::Resources( *this );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}