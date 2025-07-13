#include <jde/opc/OpcQLHook.h>
#include "types/OpcClient.h"
#include "auth/UM.h"
#include <jde/opc/uatypes/Logger.h>

#define let const auto

namespace Jde::Opc::Gateway{
	constexpr EOpcLogTags _tags{ EOpcLogTags::Opc | (EOpcLogTags)ELogTags::QL };
	using Jde::QL::Hook::Operation;

	struct HookAwait final: TAwait<jvalue>{
		HookAwait( const QL::MutationQL& m, UserPK executer, Operation op, SL sl )ι: TAwait<jvalue>{sl}, _mutation{m}, _executer{executer}, _op{op}{}
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->TAwait<vector<OpcClient>>::Task;
		α Fix( DB::Key id )ι->TAwait<Access::ProviderPK>::Task;
		const QL::MutationQL& _mutation;
		UserPK _executer;
		Operation _op;
	};

	α HookAwait::Execute()ι->OpcClientAwait::Task{
		try{
			DB::Key id = (_op & Operation::Purge)==Operation::Purge
				? DB::Key{ _mutation.Id<OpcClientPK>() }
				: DB::Key{ Json::AsString(_mutation.Args, "target") };
			optional<uint> rowCount;
			if( _op==(Operation::Insert | Operation::Failure) ){
				auto opcServers = co_await OpcClientAwait{ id };
				if( opcServers.size() ) //assume failed because already exists.
					rowCount = 0;
			}
			if( !rowCount.has_value() )
				Fix( move(id) );
			else
				ResumeScaler( {{"rowCount", *rowCount}} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α HookAwait::Fix( DB::Key id )ι->ProviderCreatePurgeAwait::Task{
		try{
			let insert = _op==(Operation::Insert | Operation::Before) || _op==(Operation::Purge | Operation::Failure);
			co_await ProviderCreatePurgeAwait( move(id), insert );
			ResumeScaler( {{"rowCount", 1}} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α OpcQLHook::InsertBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="clients" || m.TableName()=="opc_clients" ? mu<HookAwait>( m, userPK, Operation::Insert | Operation::Before, sl ) : nullptr;
	}
	α OpcQLHook::InsertFailure( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="clients" || m.TableName()=="opc_clients" ? mu<HookAwait>( m, userPK, Operation::Insert | Operation::Failure, sl ) : nullptr;
	}
	α OpcQLHook::PurgeBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="clients"  || m.TableName()=="opc_clients" ? mu<HookAwait>( m, userPK, Operation::Purge | Operation::Before, sl ) : nullptr;
	}
	α OpcQLHook::PurgeFailure( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="clients" || m.TableName()=="opc_clients" ? mu<HookAwait>( m, userPK, Operation::Purge | Operation::Failure, sl ) : nullptr;
	}
}