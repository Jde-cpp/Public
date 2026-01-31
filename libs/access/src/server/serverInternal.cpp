#include <jde/access/server/accessServer.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/LocalQL.h>
#include <jde/access/awaits/ConfigureAwait.h>
#include "serverInternal.h"
#include "jde/access/server/awaits/AclAwait.h"
#include "jde/access/server/awaits/RoleAwait.h"
#include "awaits/GroupAwait.h"
#include "awaits/UserAwait.h"
#include "../accessInternal.h"


namespace Jde::Access{
	static sp<QL::LocalQL> _ql;
	α Server::AccessSchema()ι->DB::AppSchema&{ return GetSchema(); }
	α Server::LocalQL()ι->QL::LocalQL&{ ASSERT( _ql );  return *_ql; }
	α Server::Authorizer()ι->Access::Authorize&{ return LocalQL().Authorizer(); }

	α Server::Authenticate( str loginName, uint providerId, str opcServer, SL sl )ι->AuthenticateAwait{
		return AuthenticateAwait{ loginName, providerId, opcServer, sl };
	}
	α Server::DS()ι->DB::IDataSource&{ return LocalQL().DS(); }
	α Server::GetTablePtr( str name, SL sl )ε->sp<DB::View>{ return LocalQL().GetTablePtr(name, sl); }
	α Server::GetTable( str name, SL sl )ε->const DB::View&{ return LocalQL().GetTable(name, sl); }

	α Server::Configure( vector<sp<DB::AppSchema>>&& schemas, sp<QL::LocalQL> localQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait{
		auto accessSchema = find_if( schemas, []( const sp<DB::AppSchema>& x ){ return x->Name=="access"; } );
		THROW_IF( accessSchema==schemas.end(), "Access schema not found in schemas" );
		SetSchema( *accessSchema );
		_ql = localQL;
		QL::Hook::Add( mu<GroupHook>() );//add before
		return ConfigureAwait{ localQL, move(schemas), authorizer, executer, move(listener), {} };
	}
	α Server::CustomQuery( QL::TableQL& q, UserPK executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> y;
		if( q.DBTableName()=="acl" )
			y = mu<AclQLSelectAwait>( q, executer, sl );
		else if( q.JsonName.starts_with("role") && (q.FindTable("roles") || q.FindTable("permissionRights")) )
			y = mu<RoleAwait>( q, executer, sl );
		else if( q.JsonName.starts_with( "user" ) && q.FindTable("groupings") )
			y = mu<UserAwait>( q, executer, sl );
		else if( q.JsonName.starts_with( "grouping" ) )
			y = mu<GroupAwait>( q, executer, sl );
		return y;
	}
	α Server::CustomMutation( QL::MutationQL& m, UserPK executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> y;
		using enum QL::EMutationQL;
		if( m.TableName()=="acl" && (m.Type==Purge || m.Type==Create) )
			y = mu<Access::Server::AclQLAwait>(move(m), executer, sl);
		else if( (m.Type==Add || m.Type==Remove) && m.TableName()=="roles" )
			y = mu<Access::Server::RoleMAwait>(move(m), executer, sl);
		return y;
	}
}