#pragma once
#include <jde/db/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/types/MutationQL.h>
//#include "../../../../Framework/source/coroutine/Awaitable.h"

#pragma warning(push)
#pragma warning( disable : 4100 )
namespace Jde::QL{
	struct IMutationAwait; struct MutationQL; struct TableQL;
	struct IGraphQLHook{
		β InsertBefore( const MutationQL&, UserPK, SRCE )ι->up<TAwait<jvalue>>{ return {}; }
		β InsertFailure( const MutationQL&, UserPK, SRCE )ι->up<TAwait<jvalue>>{ return {}; }
		β PurgeBefore( const MutationQL&, UserPK, SRCE )ι->up<TAwait<jvalue>>{ return {}; }
		β PurgeFailure( const MutationQL&, UserPK, SRCE )ι->up<TAwait<jvalue>>{ return {}; }
		β Select( const TableQL&, UserPK, SRCE )ι->up<TAwait<jvalue>>{ return {}; }
		β Start( const MutationQL&, UserPK, SRCE )ι->up<IMutationAwait>{ return {}; }
		β Stop( const MutationQL&, UserPK, SRCE )ι->up<IMutationAwait>{ return {}; }
	};
#pragma warning(pop)

	namespace Hook{
		α Add( up<IGraphQLHook>&& hook )ι->void;
		enum class Operation : uint8{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Purge=0x10, Select=0x20, Start=0x40, Stop=0x80 };
	}

	struct QueryHookAwaits final: TAwait<optional<jvalue>>{
		//GraphQLHookAwait( const MutationQL& _mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		QueryHookAwaits( const TableQL& table_, UserPK userPK, Hook::Operation op, SRCE )ι;
		α await_ready()ι->bool override;
		α await_resume()ε->optional<jvalue>;//null=not handled.
	private:
		//α CollectAwaits( const MutationQL& mutation, UserPK userPK, Hook::Operation op )ι->optional<AwaitResult>;
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		//α AwaitMutation()ι->TAwait<jvalue>::Task;
		optional<jvalue> _readyResult;
		vector<up<TAwait<jvalue>>> _awaitables;
		Hook::Operation _op;
		const TableQL& _ql;
		UserPK _userPK;
	};

	struct IMutationAwait : TAwait<optional<jvalue>>{
		using base=TAwait<optional<jvalue>>;
		IMutationAwait( MutationQL mutation, UserPK userPK, SRCE )ι:base{sl}, _mutation{move(mutation)}, _userPK{userPK}{};
		virtual ~IMutationAwait()=0;
	protected:
		MutationQL _mutation;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	struct MutationAwaits : IMutationAwait{
		using base=IMutationAwait;
		MutationAwaits( MutationQL mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α Execute()ι->IMutationAwait::Task;
		α await_resume()ι->optional<jvalue> override;
	private:
		vector<up<IMutationAwait>> _awaitables;
		Hook::Operation _op;
	};

	namespace Hook{
		α Select( const TableQL& ql, UserPK userId, SRCE )ι->QueryHookAwaits;
		α InsertBefore( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α InsertFailure( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α PurgeBefore( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α PurgeFailure( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α Start( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
		α Stop( const MutationQL& mutation, UserPK userId, SRCE )ι->MutationAwaits;
	};
}