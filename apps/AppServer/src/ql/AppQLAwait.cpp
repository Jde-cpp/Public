#include "AppQLAwait.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/server/awaits/AclAwait.h>
#include <jde/access/server/awaits/RoleAwait.h>
#include <jde/web/server/SettingQL.h>
#include "../LocalClient.h"
#include "ConnectionQLAwait.h"
#define let const auto

namespace Jde::App::Server{
	α AppQLAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			if( _queries.IsQueries() ){
				_raw = _raw && _queries.Queries().size()==1;
				jvalue y = _raw ? jvalue{} : jobject{};
				for( auto& q : _queries.Queries() ){
					q.ReturnRaw = true;
					let memberName = q.ReturnName();
					jvalue queryResult;
					if( q.JsonName.starts_with("connection") )
						queryResult = co_await ConnectionQLAwait{ move(q), UserPK(), _sl };
					else if( q.DBTableName()=="acl" )
						queryResult = co_await Access::Server::AclQLSelectAwait{ q, UserPK(), _sl };
					else if( q.JsonName.starts_with("role") && (q.FindTable("roles") || q.FindTable("permissionRights")) )
						queryResult = co_await Access::Server::RoleSelectAwait{ q, UserPK(), _sl };
					else if( q.JsonName.starts_with( "setting" ) )
						queryResult = co_await Web::Server::SettingQLAwait{ q, AppClient(), _sl };
					else
						queryResult = co_await QL::QLAwait( move(q), UserPK(), _sl );
					if( _raw )
						y = queryResult;
					else
						y.get_object()[memberName] = move( queryResult );
				}
				Resume( move(y) );
			}
			else if( _queries.IsMutation() ){
				jarray results;
				for( auto& m : _queries.Mutations() ){
					m.ReturnRaw = _raw;
					using enum QL::EMutationQL;
					if( m.TableName()=="acl" && (m.Type==Purge || m.Type==Create) )
						results.push_back( co_await Access::Server::AclQLAwait{m, UserPK(), _sl} );
					else if( (m.Type==Add || m.Type==Remove) && m.TableName()=="roles" )
						results.push_back( co_await Access::Server::RoleMutationAwait{move(m), UserPK(), _sl} );
					else
						results.push_back( co_await QL::QLAwait(move(m), UserPK(), _sl) );
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( move(y) );
			}
			else{
				Resume( co_await QL::QLAwait(move(_queries), UserPK(), _sl) );
			}
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}