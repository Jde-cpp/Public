#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#ifdef JDE_BLOCKS_EXPORTS
	#ifdef _MSC_VER
		#define Φ __declspec( dllexport )  auto
	#else
		#define Φ __attribute__((visibility("default")))  auto
	#endif
#else
	#ifdef _MSC_VER
		#define Φ __declspec( dllimport ) auto
	#else
		#define Φ auto
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
	Φ MBlocklyVersion()noexcept->const char*;
	Φ MBlocklyAllocatedList()noexcept->std::vector<Jde::Markets::MBlockly::CApi::Function>*;
	Φ MBlocklyCreateAllocated( std::string_view id, long orderId, uint32_t contractId/*, std::shared_ptr<Jde::Markets::TwsClient> pTws*/ )noexcept->std::shared_ptr<Jde::Markets::MBlockly::IBlockly>*;
}
#undef Φ