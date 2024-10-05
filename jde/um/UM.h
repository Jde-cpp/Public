#pragma once
#ifndef UM_H
#define UM_H
#include <jde/Exports.h>
#include <jde/TypeDefs.h>
#include <jde/Exception.h>
#include <jde/coroutine/Await.h>
#include <jde/db/usings.h>
#include "../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::QL{ struct MutationQL; enum class EMutationQL : uint8; }

#define Φ Γ auto
namespace Jde::UM{
	using ApiPK=uint;
	enum class EProviderType : uint8{
		None = 0,
		Google=1,
		Facebook=2,
		Amazon=3,
		Microsoft=4,
		VK=5,
		Key = 6,
		Certificate = 7,
		OpcServer = 8
	};
	constexpr array<sv,9> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "Certificate", "OpcServer" };

	enum class EAccess : uint8{ None=0, Administer=1, Write=2, Read=4 };
//	Ξ operator &( EAccess a, EAccess b )ι{ return static_cast<EAccess>( static_cast<uint8>(a) & static_cast<uint8>(b) ); }

	struct LoginAwait final : TAwait<UserPK>{
		using base = TAwait<UserPK>;
		LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α LoginTask()ε->Jde::Task;
		vector<unsigned char> _modulus; vector<unsigned char> _exponent; string _name; string _target; string _description;
	};

	Φ Configure()ε->void;
	α IsTarget( sv url )ι->bool;
	α Login( string loginName, uint providerId, string opcServer={}, SRCE )ι->AsyncAwait;
	α TestRead( str tableName, UserPK userId )ε->void;
	α TestAccess( EAccess access, UserPK userId, ApiPK apiId )ε->void;
	α ApplyMutation( const QL::MutationQL& m, UserPK id )ε->void;
	α Query( string q, UserPK userPK, SRCE )ε->json;
	struct IAuthorize;
	Φ AddAuthorizer( UM::IAuthorize* p )ι->void;
	Φ FindAuthorizer( sv table )ι->IAuthorize*;
	struct IAuthorize{
		IAuthorize( sv table ):TableName{table}{ UM::AddAuthorizer( this ); }
		virtual ~IAuthorize()=0;
		β CanRead( uint /*pk*/, UserPK /*userId*/ )ι->bool { return true; }
		β CanPurge( uint /*pk*/, UserPK /*userId*/ )ι->bool{ return true; }
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
	};
}
#undef Φ
#endif