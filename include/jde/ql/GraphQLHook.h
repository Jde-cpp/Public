#pragma once
#include <jde/db/usings.h>
#include <jde/framework/coroutine/Await.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

#pragma warning(push)
#pragma warning( disable : 4100 )
namespace Jde::QL{
	struct IMutationAwait; struct MutationQL; struct TableQL;
	struct IGraphQLHook{
		β InsertBefore( const MutationQL&, UserPK, SRCE )ι->up<IAwait>{ return {}; }
		β InsertFailure( const MutationQL&, UserPK, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeBefore( const MutationQL&, UserPK, SRCE )ι->up<IAwait>{ return {}; }
		β PurgeFailure( const MutationQL&, UserPK, SRCE )ι->up<IAwait>{ return {}; }
		β Select( const TableQL&, UserPK, SRCE )ι->up<IAwait>{ return {}; }
		β Start( sp<MutationQL>, UserPK, SRCE )ι->up<IMutationAwait>{ return {}; }
		β Stop( sp<MutationQL>, UserPK, SRCE )ι->up<IMutationAwait>{ return {}; }
	};
#pragma warning(pop)

	namespace Hook{
		α Add( up<IGraphQLHook>&& hook )ι->void;
		enum class Operation : uint8{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Purge=0x10, Select=0x20, Start=0x40, Stop=0x80 };
	}

	//deprecated
	struct GraphQLHookAwait final: AsyncReadyAwait{
		GraphQLHookAwait( const MutationQL& _mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		GraphQLHookAwait( const TableQL& table_, UserPK userPK, Hook::Operation op, SRCE )ι;
	private:
		α CollectAwaits( const MutationQL& mutation, UserPK userPK, Hook::Operation op )ι->optional<AwaitResult>;
		α CollectAwaits( const TableQL& ql, UserPK userPK, Hook::Operation op )ι->optional<AwaitResult>;
		α Await( HCoroutine h )ι->Task;
		α AwaitMutation( HCoroutine h )ι->Task;
		vector<up<IAwait>> _awaitables;
	};

	struct IMutationAwait : TAwait<uint>{
		using base=TAwait<uint>;
		IMutationAwait( sp<MutationQL> mutation, UserPK userPK, SRCE )ι:base{sl}, _mutation{mutation}, _userPK{userPK}{};
		virtual ~IMutationAwait()=0;
	protected:
		sp<MutationQL> _mutation;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	struct MutationAwaits : IMutationAwait{
		using base=IMutationAwait;
		MutationAwaits( sp<MutationQL> mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
	private:
		vector<up<IMutationAwait>> _awaitables;
		Hook::Operation _op;
	};

	namespace Hook{
		α Select( const TableQL& ql, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α InsertBefore( const MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α InsertFailure( const MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α PurgeBefore( const MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α PurgeFailure( const MutationQL& mutation, UserPK userId, SRCE )ι->GraphQLHookAwait;
		α Start( sp<MutationQL> mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α Stop( sp<MutationQL> mutation, UserPK userId, SRCE )ι->MutationAwaits;
	};
}