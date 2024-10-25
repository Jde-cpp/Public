#pragma once
namespace Jde::Access{
	using IdentityPK=uint32;
	using UserPK=IdentityPK;
	using AppPK=uint16;

	enum class EProviderType : uint8{
		None = 0,
		Google=1,
		Facebook=2,
		Amazon=3,
		Microsoft=4,
		VK=5,
		Key = 6,
		OpcServer = 7
	};
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };

	enum class ERights : uint8{ None=0, Create=1, Read=2, Subscribe=4, Update=8, Purge=0x10, Execute=0x20, Rights=0x40 };
	constexpr array<sv,8> RightsStrings = { "None", "Create", "Read", "Subscribe", "Update", "Purge", "Execute", "Rights" };
}
