#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#ifdef JDE_BLOCKS_EXPORTS
	#ifdef _MSC_VER
		#define 🚪 __declspec( dllexport )  auto
	#else
		#define 🚪 __attribute__((visibility("default")))  auto
	#endif
#else
	#ifdef _MSC_VER
		#define 🚪 __declspec( dllimport ) auto
	#else
		#define 🚪 auto
	#endif
#endif
namespace Jde::Markets
{
	struct MyOrder; struct Tick; struct Account; struct Contract; struct TwsClient;
	namespace MBlockly
	{
		struct Account;
		struct IBlockly;
		namespace CApi
		{
			struct Function{ std::string Id; std::string Name; std::string Description; std::string Path; std::string Xml; size_t Crc; };
		}
	}
}
extern "C"
{
	🚪 MBlocklyVersion()noexcept->char*;
	🚪 MBlocklyAllocatedList()noexcept->std::vector<Jde::Markets::MBlockly::CApi::Function>*;
	🚪 MBlocklyCreateAllocated( std::string_view id, long orderId, uint32_t contractId/*, std::shared_ptr<Jde::Markets::TwsClient> pTws*/ )noexcept->std::shared_ptr<Jde::Markets::MBlockly::IBlockly>*;
}
#undef 🚪
