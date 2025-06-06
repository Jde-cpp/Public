#pragma once
#ifndef ACCESS_USINGS_H
#define ACCESS_USINGS_H
namespace Jde::Access{
	struct GroupPK final : PK<uint32>{};
	using ProviderPK=uint32;
	//using PermissionIdentityPK=uint32;
	using PermissionPK=uint32;
	using PermissionRightsPK=PermissionPK;
	using RolePK=PermissionPK;
	using ResourcePK=uint16;
	using PermissionRole=variant<PermissionPK,RolePK>;

	struct IdentityPK{
		IdentityPK( Jde::UserPK pk )ι:Value{pk}{}
		IdentityPK( Access::GroupPK pk )ι:Value{pk}{}
		using Type=Jde::UserPK::Type;
		α IsUser()Ι->bool{ return Value.index()==0; }
		α UserPK()Ι->Jde::UserPK{ return get<Jde::UserPK>(Value); }
		α GroupPK()Ι->Access::GroupPK{ return get<Access::GroupPK>(Value); }
		α Underlying()Ι->Type{ return Value.index()==0 ? UserPK().Value : GroupPK().Value; }
		α operator!=( IdentityPK rhs )Ι->bool{ return Value!=rhs.Value; }
		α operator<( IdentityPK rhs )Ι->bool{ return Underlying() < rhs.Underlying(); }

		variant<Jde::UserPK,Access::GroupPK> Value;
	};

	enum class EProviderType : ProviderPK{
		None = 0,
		Google=1,
		Facebook=2,
		Amazon=3,
		Microsoft=4,
		VK=5,
		Key = 6,
		OpcServer = 7
	};

	enum class ERights : uint8{
		None=0,
		Create=0x1,
		Read=0x2,
		Update=0x4,
		Delete=0x8,
		Purge=0x10,
		Administer=0x20,
		Subscribe=0x40,
		Execute=0x80,
		All = 0xFF
	};
	constexpr array<sv,9> RightsStrings = { "None", "Create", "Read", "Update", "Delete", "Purge", "Administer", "Subscribe", "Execute" };
	Ξ ToRight( sv x )ι->ERights{ return ToFlag<ERights>( RightsStrings, x ).value_or(ERights::None); }
	Ξ ToRights( const jarray& a )ι->Access::ERights{
		using enum Access::ERights;
		Access::ERights rights{ None };
		for( auto v : a )
			rights |= Access::ToRight( v.is_string() ? v.get_string() : "None" );
		return rights;
	}
	Ξ ToString( ERights r )ι->string{ return FromEnumFlag<ERights>( RightsStrings, r );  }
}
#endif