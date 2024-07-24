#include <jde/iot/IotGraphQL.h>
#include <jde/iot/types/OpcServer.h>
#include <jde/iot/UM.h>
#include <jde/io/Json.h>
//#include "../../../Framework/source/io/ServerSink.h"
#include "../../../Framework/source/db/GraphQL.h"

#define var const auto

namespace Jde::Iot{
	static sp<LogTag> _logTag{ Logging::Tag( "iot.graphQL" ) };
	using Jde::DB::GraphQL::Hook::Operation;

	α Query( const DB::MutationQL& m, UserPK /*userPK*/, Operation op, HCoroutine h )ι->Task{
		try{
			variant<OpcPK,OpcNK> id;
			if( (op & Operation::Purge)==Operation::Purge )
				id = Json::Getε<OpcPK>( m.Args, "id" );
			else
				id = Json::Getε<OpcNK>( m.Input(), "target" );
			optional<uint> rowCount;
			if( op==(Operation::Insert | Operation::Failure) ){
				auto pOpcServer = ( co_await OpcServer::Select(get<1>(id)) ).UP<OpcServer>();
				if( pOpcServer ) //assume failed because already exists.
					rowCount = 0;
			}
			if( !rowCount ){
				var insert = op==(Operation::Insert | Operation::Before) || op==(Operation::Purge | Operation::Failure);
				rowCount = await( uint, ProviderAwait(move(id), insert) );
			}
			Resume( mu<uint>(*rowCount), move(h) );
		}
		catch( Exception& e ){
			Resume( move(e), move(h) );
		}
	}

	struct IotGraphQLAwait final: AsyncAwait{
		IotGraphQLAwait( const DB::MutationQL& m, UserPK userPK_, Operation op_, SL sl )ι:
			AsyncAwait{ [&, userPK=userPK_, op=op_](HCoroutine h){ Query(m, userPK, op, move(h));}, sl, "IotGraphQLAwait" }
		{}
	};

	α IotGraphQL::InsertBefore( const DB::MutationQL& m, UserPK userPK, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, userPK, Operation::Insert | Operation::Before, sl ) : nullptr;
	}
	α IotGraphQL::InsertFailure( const DB::MutationQL& m, UserPK userPK, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, userPK, Operation::Insert | Operation::Failure, sl ) : nullptr;
	}
	α IotGraphQL::PurgeBefore( const DB::MutationQL& m, UserPK userPK, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, userPK, Operation::Purge | Operation::Before, sl ) : nullptr;
	}
	α IotGraphQL::PurgeFailure( const DB::MutationQL& m, UserPK userPK, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, userPK, Operation::Purge | Operation::Failure, sl ) : nullptr;
	}
}