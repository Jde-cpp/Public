#include <jde/iot/IotGraphQL.h>
#include <jde/iot/types/OpcServer.h>
#include <jde/iot/UM.h>
#include <jde/io/Json.h>
#include "../../../Framework/source/io/ServerSink.h"
#include "../../../Framework/source/db/GraphQL.h"

#define var const auto

namespace Jde::Iot{
	static sp<LogTag> _logTag = Logging::Tag( "iot.graphQL" );
	using Jde::DB::GraphQL::Hook::Operation;

	α Query( const DB::MutationQL& m, Operation op, HCoroutine h )ι->Task{
		try{
			variant<OpcPK,string> id;
			if( (op & Operation::Purge)==Operation::Purge )
				id = Json::Getε<OpcPK>( m.Args, "id" );
			else
				id = Json::Getε<string>( m.Input(), "target" );
			var insert = op==(Operation::Insert | Operation::Before) || op==(Operation::Purge | Operation::Failure);
			up<uint> result = ( co_await ProviderAwait{move(id), insert} ).UP<uint>();
			Resume( move(result), move(h) );
		}
		catch( Exception& e ){
			Resume( move(e), move(h) );
		}
	}
	
	struct IotGraphQLAwait final: AsyncAwait{
		IotGraphQLAwait( const DB::MutationQL& m, Operation op_, SL sl )ι:
			AsyncAwait{ [&, op=op_](HCoroutine h){ Query(m, op, move(h));}, sl, "IotGraphQLAwait" }
		{}
	};

	α IotGraphQL::InsertBefore( const DB::MutationQL& m, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, Operation::Insert | Operation::Before, sl ) : nullptr;
	}
	α IotGraphQL::InsertFailure( const DB::MutationQL& m, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, Operation::Insert | Operation::Failure, sl ) : nullptr;
	}
	α IotGraphQL::PurgeBefore( const DB::MutationQL& m, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, Operation::Purge | Operation::Before, sl ) : nullptr;
	}
	α IotGraphQL::PurgeFailure( const DB::MutationQL& m, SL sl )ι->up<IAwait>{
		return m.JsonName=="opcServer" ? mu<IotGraphQLAwait>( m, Operation::Purge | Operation::Failure, sl ) : nullptr;
	}
}