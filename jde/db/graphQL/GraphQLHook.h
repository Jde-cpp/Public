#pragma once
#include <jde/db/usings.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"


namespace Jde::DB{ struct MutationQL; struct TableQL; }
namespace Jde::DB::GraphQL{
	struct IMutationAwait;
	struct IGraphQLHook{
		β InsertBefore( const DB::MutationQL& mutation, UserPK userPK, SRCE )ι->up<IAwait>{ return {}; }
		β InsertFailure( const DB::MutationQL& mutation, UserPK userPK, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeBefore( const DB::MutationQL& mutation, UserPK userPK, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeFailure( const DB::MutationQL& mutation, UserPK userPK, SRCE )ι->up<IAwait>{ return {}; }
		β Select( const DB::TableQL& query, UserPK userPK, SRCE )ι->up<IAwait>{ return {}; }
		β Start( sp<DB::MutationQL> mu, UserPK userPK, SRCE )ι->up<IMutationAwait>{ return {}; }
		β Stop( sp<DB::MutationQL> mu, UserPK userPK, SRCE )ι->up<IMutationAwait>{ return {}; }
	};

	namespace Hook{
		α Add( up<GraphQL::IGraphQLHook>&& hook )ι->void;
		enum class Operation : uint8{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Purge=0x10, Select=0x20, Start=0x40, Stop=0x80 };
	}

	//deprecated
	struct GraphQLHookAwait final: AsyncReadyAwait{
		GraphQLHookAwait( const DB::MutationQL& _mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		GraphQLHookAwait( const DB::TableQL& table_, UserPK userPK, Hook::Operation op, SRCE )ι;
	private:
		α CollectAwaits( const DB::MutationQL& mutation, UserPK userPK, Hook::Operation op )ι->optional<AwaitResult>;
		α CollectAwaits( const DB::TableQL& ql, UserPK userPK, Hook::Operation op )ι->optional<AwaitResult>;
		α Await( HCoroutine h )ι->Task;
		α AwaitMutation( HCoroutine h )ι->Task;
		vector<up<IAwait>> _awaitables;
	};

	struct IMutationAwait : TAwait<uint>{
		using base=TAwait<uint>;
		IMutationAwait( sp<DB::MutationQL> mutation, UserPK userPK, SRCE )ι:base{sl}, _mutation{mutation}, _userPK{userPK}{};
		virtual ~IMutationAwait()=0;
	protected:
		sp<DB::MutationQL> _mutation;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	//collection of IMutationAwait
	struct MutationAwaits : IMutationAwait{
		using base=IMutationAwait;
		MutationAwaits( sp<DB::MutationQL> mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		α await_ready()ι->bool override;
		α await_suspend( base::Handle h )ι->void override;
	private:
		vector<up<IMutationAwait>> _awaitables;
		Hook::Operation _op;
	};

	namespace Hook{
		α Select( const DB::TableQL& ql, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α InsertBefore( const DB::MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α InsertFailure( const DB::MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α PurgeBefore( const DB::MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α PurgeFailure( const DB::MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α Start( sp<DB::MutationQL> mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α Stop( sp<DB::MutationQL> mutation, UserPK userId, SRCE )ι->MutationAwaits;
	};
}