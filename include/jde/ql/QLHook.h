#pragma once
#include <jde/db/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/types/MutationQL.h>
//#include "../../../../Framework/source/coroutine/Awaitable.h"

#pragma warning(push)
#pragma warning( disable : 4100 )
namespace Jde::QL{
	struct IMutationAwait; struct MutationQL; struct TableQL;
	struct IQLHook{
		using HookResult=up<TAwait<jvalue>>;
		β AddBefore( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
		β Add( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
		β AddAfter( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
//		β RemoveBefore( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
		β Remove( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
		β RemoveAfter( const MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult{ return {}; }
		β InsertBefore( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β InsertAfter( const MutationQL&, UserPK, uint pk, SRCE )ι->HookResult{ return {}; }
		β InsertFailure( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β PurgeBefore( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β PurgeFailure( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β Select( const TableQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β Start( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β Stop( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β UpdateAfter( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
	};
#pragma warning(pop)

	namespace Hook{
		α Add( up<IQLHook>&& hook )ι->void;
		enum class Operation : uint16{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Update=0x10, Purge=0x20, Select=0x40, Start=0x80, Stop=0x100, Add=0x200, Remove=0x400 };
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

	struct IMutationAwait : TAwait<jvalue>{
		using base=TAwait<jvalue>;
		IMutationAwait( MutationQL mutation, UserPK userPK, SRCE )ι:base{sl}, _mutation{move(mutation)}, _userPK{userPK}{};
		virtual ~IMutationAwait()=0;
	protected:
		MutationQL _mutation;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	struct MutationAwaits : TAwait<optional<jarray>>{
		using base=TAwait<optional<jarray>>;
		MutationAwaits( MutationQL mutation, UserPK userPK, Hook::Operation op, SRCE )ι;
		MutationAwaits( MutationQL mutation, UserPK userPK, Hook::Operation op, uint pk=0, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α Execute()ι->IMutationAwait::Task;
		α await_resume()ε->optional<jarray> override;
	private:
		vector<up<TAwait<jvalue>>> _awaitables;
		MutationQL _mutation;
		Hook::Operation _op;
		uint _pk;
		UserPK _userPK;
	};

	namespace Hook{
		α Select( const TableQL& ql, UserPK userPK, SRCE )ι->QueryHookAwaits;
		α AddBefore( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α Add( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α AddAfter( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		//α RemoveBefore( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α Remove( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α RemoveAfter( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α InsertBefore( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α InsertAfter( uint pk, const MutationQL&, UserPK, SRCE )ι->MutationAwaits;
		α InsertFailure( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α PurgeBefore( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α PurgeFailure( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α Start( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α Stop( const MutationQL& m, UserPK userPK, SRCE )ι->MutationAwaits;
		α UpdateAfter( const MutationQL&, UserPK userPK, SRCE )ι->MutationAwaits;
	};
}