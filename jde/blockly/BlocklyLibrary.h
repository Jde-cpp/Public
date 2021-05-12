#pragma once
#include "Exports.h"
#include <jde/coroutine/Task.h>
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#include "./types/proto/blockly.pb.h"
#pragma warning( default : 4996 )
#pragma warning( default : 4244 )


namespace Jde::Markets
{
	struct MyOrder; struct Tick; struct Account; struct Contract; struct TwsClient;
	namespace MBlockly{ struct Account; struct IBlockly; }
}

namespace Jde::Blockly
{
	static constexpr uint32 IdenticalCrcLogId = 2152742774;
	JDE_BLOCKLY void SetBasePath( path path )noexcept;
	JDE_BLOCKLY void Copy( sv from, Proto::Function to )noexcept(false);
	JDE_BLOCKLY void Save( const Proto::Function& fnctn )noexcept(false);
	JDE_BLOCKLY void Delete( sv id )noexcept(false);
	JDE_BLOCKLY up<Proto::Functions> Load()noexcept(false);
	JDE_BLOCKLY up<Proto::Function> Load( sv id )noexcept(false);
	JDE_BLOCKLY void Build( const string& id )noexcept(false);
	JDE_BLOCKLY void DeleteBuild( sv id )noexcept(false);
	JDE_BLOCKLY void Enable( const string& id )noexcept(false);
	JDE_BLOCKLY void Disable( sv id )noexcept(false);
	JDE_BLOCKLY sp<Markets::MBlockly::IBlockly>* CreateAllocatedExecutor( std::string_view blockId, long orderId, uint32_t contractId );
	//	JDE_BLOCKLY void Execute( std::string_view id, const Markets::MyOrder& order/*, const Jde::Markets::Tick& tick, const Jde::Markets::MBlockly::Account& account*/, const Jde::Markets::Contract& contract, std::shared_ptr<Jde::Markets::TwsClient> pTws )noexcept(false);
	//JDE_BLOCKLY void Execute( std::string_view id, const Markets::MyOrder& order, const Jde::Markets::Contract& contract, std::shared_ptr<Jde::Markets::TwsClient> pTws )noexcept(false);
}