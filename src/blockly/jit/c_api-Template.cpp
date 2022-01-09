#include "c_api.h"
[[jde::Include]]
#include <memory>

extern "C"
{
	const char* MBlocklyVersion()noexcept{ return [[jde::Version]]; }

	#define NS Jde::Markets::MBlockly::Blocks
	#define ADD_LISTX(x) { using NS::x; p->push_back( {std::string{x::Id}, std::string{x::Name}, std::string{x::Description}, std::string{x::Path}, std:: string{x::Xml}, x::Crc} ); }
	std::vector<Jde::Markets::MBlockly::CApi::Function>* MBlocklyAllocatedList()noexcept
	{
		auto p = new std::vector<Jde::Markets::MBlockly::CApi::Function>();
		[[jde::AddList]]
		return p;
	}

	#define CREATEX(x) if( blockId==NS::x::Id ){ using NS::x; p = new std::shared_ptr<Jde::Markets::MBlockly::IBlockly>( std::make_shared<x>(orderId, contractId) ); }
	std::shared_ptr<Jde::Markets::MBlockly::IBlockly>* MBlocklyCreateAllocated( std::string_view blockId, long orderId, uint32_t contractId )noexcept
	{
		std::shared_ptr<Jde::Markets::MBlockly::IBlockly>* p = nullptr;

		[[jde::Create]]
		return p;
	}
}