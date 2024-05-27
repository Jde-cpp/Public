#pragma once
#include "../../../../Framework/source/coroutine/Awaitable.h"


namespace Jde::DB{ struct MutationQL; }
namespace Jde::DB::GraphQL{

	struct IGraphQLHook{
		β InsertBefore( const DB::MutationQL& mutation, SRCE )ι->up<IAwait>{ return {}; }
		β InsertFailure( const DB::MutationQL& mutation, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeBefore( const DB::MutationQL& mutation, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeFailure( const DB::MutationQL& mutation, SRCE )ι->up<IAwait>{ return {}; }
	};

	namespace Hook{
		α Add( up<GraphQL::IGraphQLHook>&& hook )ι->void;	
		enum class Operation : uint8{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Purge=0x10 };
		constexpr α operator|( Operation a, Operation b )ι->Operation{ return (Operation)( (int)a | (int)b ); }
		constexpr α operator&( Operation a, Operation b )ι->Operation{ return (Operation)( (int)a & (int)b ); }
	}

	struct GraphQLHookAwait final: AsyncReadyAwait{
		GraphQLHookAwait( const DB::MutationQL& _mutation, Hook::Operation op, SRCE )ι;

	private:
		α CollectAwaits( const DB::MutationQL& mutation, Hook::Operation op )ι->optional<AwaitResult>;
		α Await( HCoroutine h )ι->Task;
		vector<up<IAwait>> _awaitables;
	};
	
	namespace Hook{
		α InsertBefore( const DB::MutationQL& mutation, SRCE )ι->GraphQLHookAwait;
		α InsertFailure( const DB::MutationQL& mutation, SRCE )ι->GraphQLHookAwait;
		α PurgeBefore( const DB::MutationQL& mutation, SRCE )ι->GraphQLHookAwait;
		α PurgeFailure( const DB::MutationQL& mutation, SRCE )ι->GraphQLHookAwait;
	};

}