#include <jde/access/server/accessServer.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/IQL.h>
#include <jde/ql/LocalQL.h>
#include <jde/access/awaits/ConfigureAwait.h>
#include "serverInternal.h"
#include "hooks/AclHook.h"
#include "hooks/GroupHook.h"
#include "hooks/RoleHook.h"
#include "hooks/UserHook.h"
#include "../awaits/ResourceLoadAwait.h"
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
	//α Server::GetSchema()ι->DB::AppSchema&{ ASSERT(_schema); return _schema; }
	//α Server::SetSchema( sp<DB::AppSchema> schema )ι->void{ /*ASSERT(!_schema);*/ _schema = schema; }

	α Server::Configure( vector<sp<DB::AppSchema>>&& schemas, sp<QL::LocalQL> localQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait{
		auto accessSchema = find_if( schemas, []( const sp<DB::AppSchema>& x ){ return x->Name=="access"; } );
		THROW_IF( accessSchema==schemas.end(), "Access schema not found in schemas" );
		SetSchema( *accessSchema );
		_ql = localQL;
		QL::Hook::Add( mu<AclHook>() );//select, insertBefore
		QL::Hook::Add( mu<GroupHook>() );//add before
		QL::Hook::Add( mu<RoleHook>() );//add remove
		QL::Hook::Add( mu<UserHook>() );//select
		return ConfigureAwait{ localQL, move(schemas), authorizer, executer, move(listener) };
	}
}
