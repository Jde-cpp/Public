#pragma once

#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include "exports.h"
#include "usings.h"
#include <jde/db/meta/AppSchema.h>
#include <jde/framework/coroutine/Await.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::QL{ struct MutationQL; enum class EMutationQL : uint8; }

#define Φ ΓA auto
namespace Jde::Access{
	struct IAcl;
	struct ConfigureAwait : VoidAwait<>{
		ConfigureAwait( vec<AppPK> appPKs )ι;
		α Suspend()ι->void override;
		vector<AppPK> AppPKs;
	};
	Φ Configure( sp<DB::AppSchema> schema, vec<AppPK> appPKs )ε->ConfigureAwait;
	α LocalAcl()ι->sp<IAcl>;
	α Authenticate( string loginName, uint providerId, string opcServer, SRCE )ι->AsyncAwait;
	//α IsTarget( sv url )ι->bool;
	//α ApplyMutation( const QL::MutationQL& m, UserPK id )ε->void;
	//α Query( string q, UserPK userPK, SRCE )ε->json;
	//Φ AddAuthorizer( UM::IAcl* p )ι->void;
	//Φ FindAuthorizer( sv table )ι->IAcl*;
/*	struct GroupAuthorize : IAcl{
		GroupAuthorize():IAcl{"um_groups"}{}
		β CanPurge( uint pk, UserPK )ι->bool override{ return pk!=1 && pk!=2; };
		β Test( QL::EMutationQL, UserPK, SL )ε->void override{};//TODO Remove
		sv TableName;
	};*/
}
#undef Φ
