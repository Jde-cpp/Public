#include <jde/access/awaits/ConfigureAwait.h>

#include <jde/access/Authorize.h>
//#include <jde/access/types/Group.h>
//#include <jde/access/types/Role.h>
//#include <jde/access/types/User.h>
#include "../accessInternal.h"
#include "IdentityLoadAwait.h"
#include "AclLoadAwait.h"
#include "ResourceLoadAwait.h"
#include "RoleLoadAwait.h"

#define let const auto

namespace Jde::Access{

	Ω loadAcl( ConfigureAwait& await )->AclLoadAwait::Task{
		try{
			let acl = co_await AclLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Acl = move( acl );
			Authorizer().SetUserPermissions( {}, l );
			await.Resume();
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

  Ω loadRoles( ConfigureAwait& await )ι->RoleLoadAwait::Task{
		try{
			auto roles = co_await RoleLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Roles = move( roles );
			l.unlock();
			loadAcl( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	Ω loadResources( ConfigureAwait& await )ι->ResourceLoadAwait::Task{
		try{
			auto loaded = co_await ResourceLoadAwait{ await.QlServer, await.SchemaNames, await.Executer };
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
			loadRoles( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α loadUsers( ConfigureAwait& await )ι->IdentityLoadAwait::Task{
		try{
			auto identities = co_await IdentityLoadAwait{ await.QlServer, await.Executer };
			ul l{ Authorizer().Mutex };
			Authorizer().Users = std::move( identities.Users );
			Authorizer().Groups = std::move( identities.Groups );
			l.unlock();
			loadResources( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ConfigureAwait::Suspend()ι->void{
		loadUsers( *this );
	};
}