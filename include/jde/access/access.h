#pragma once

#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include "exports.h"
#include "usings.h"
#include <jde/db/meta/Schema.h>
#include <jde/framework/coroutine/Await.h>
// #include <jde/db/usings.h>
// #include "../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::QL{ struct MutationQL; enum class EMutationQL : uint8; }

#define Φ ΓA auto
namespace Jde::Access{
	struct IAuthorize;

	struct ConfigureAwait : VoidAwait<>{
		ConfigureAwait( vec<AppPK> appPKs )ι;
		α Suspend()ι->void override;
		vector<AppPK> AppPKs;
	};
	Φ Configure( sp<DB::Schema> schema, vec<AppPK> appPKs )ε->ConfigureAwait;
	//α IsTarget( sv url )ι->bool;
	//α ApplyMutation( const QL::MutationQL& m, UserPK id )ε->void;
	//α Query( string q, UserPK userPK, SRCE )ε->json;
	//Φ AddAuthorizer( UM::IAuthorize* p )ι->void;
	//Φ FindAuthorizer( sv table )ι->IAuthorize*;
/*	struct IAuthorize{
		IAuthorize( sv table ):TableName{table}{ UM::AddAuthorizer( this ); }
		virtual ~IAuthorize()=0;
		β CanRead( uint /*pk* /, UserPK /*userId* / )ι->bool { return true; }
		β CanPurge( uint /*pk* /, UserPK /*userId* / )ι->bool{ return true; }
		α TestPurge( uint pk, UserPK userId, SRCE )ε->void;
		β Test( QL::EMutationQL ql, UserPK userId, SRCE )ε->void=0;
		//β Invalidate( SRCE )ε->void=0;
		sv TableName;
	};
	inline IAuthorize::~IAuthorize(){};
	struct GroupAuthorize : IAuthorize{
		GroupAuthorize():IAuthorize{"um_groups"}{}
		β CanPurge( uint pk, UserPK )ι->bool override{ return pk!=1 && pk!=2; };
		β Test( QL::EMutationQL, UserPK, SL )ε->void override{};//TODO Remove
		sv TableName;
	};*/
}
#undef Φ
