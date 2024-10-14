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

	enum class ERights : uint8{ None=0, Read=1, Subscribe=2, Update=4, Purge=8, Execute=10, Rights=0x20 };
	constexpr array<sv,7> RightsStrings = { "None", "Read", "Subscribe", "Update", "Purge", "Execute", "Rights" };
}
