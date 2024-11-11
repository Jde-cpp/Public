#pragma once
#include <jde/framework/io/json.h>

namespace Jde::DB{ struct AppSchema; struct IDataSource; struct Table; }
namespace Jde::Access::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	//template<class... Args>
	//α Trace( const fmt::format_string<Args const&...>&& m, Args&&... args )ι->void{ Jde::Trace{ _tags, FWD(m), FWD(args)... }; }

	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α GetTable( str name )ι->sp<DB::Table>;
	α DS()ι->DB::IDataSource&;

	α Add( const DB::Table& table, uint pk, vector<uint> members, UserPK userPK )ε->void;
	α AddToGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void;
	α Remove( const DB::Table& table, uint groupId, vector<uint> members, UserPK userPK )ε->void;
	α RemoveFromGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void;

	α Create( str table, sv target, UserPK userPK, str input={} )ε->uint;
	α Delete( str table, uint id, UserPK userPK )ε->jobject;
	α Get( str table, str target, UserPK userPK, sv cols={}, bool includeDeleted=false )ε->jobject;
	Ξ GetId( const jobject& j )ε->uint{ return Json::AsNumber<uint>( j, "id" ); }
	α GetGroup( str target, UserPK userPK )ε->jobject;
	α GetRoot()ε->UserPK;
	α GetUser( str target, UserPK userPK, bool includeDeleted=false, EProviderType providerId=Access::EProviderType::Google )ε->jobject;

	α Purge( str table, uint id, UserPK userPK )ε->jobject;
	α PurgeGroup( GroupPK id, UserPK userPK )ε->void;
	α PurgeUser( UserPK userId, UserPK userPK )ε->void;

	α Select( sv table, uint id, UserPK userPK, sv cols={}, bool includeDeleted=false )ε->jobject;
	α Select( sv table, str target, UserPK userPK, sv cols={}, bool includeDeleted=false )ε->jobject;
	α SelectGroup( str target, UserPK userPK, bool includeDeleted=false )ε->jobject;
	α SelectResource( str target, UserPK userPK, bool includeDeleted=true )ε->jobject;
	α SelectUser( str target, UserPK userPK, bool includeDeleted=false )->jobject;

	α TestAdd( str tableName, uint groupId, vector<uint> members, UserPK userPK )->void;
	α TestRemove( str tableName, uint groupId, vector<uint> members, UserPK userPK )->void;
	α TestCrud( str table, str target, UserPK userPK )ε->uint;
	α TestDelete( str table, uint id, UserPK userPK )ε->void;
	α TestPurge( str table, uint id, UserPK userPK )ε->void;
	α TestUpdateName( str table, uint id, UserPK userPK, sv update="newName" )ε->void;
}
