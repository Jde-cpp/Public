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
		β Select( const TableQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β InsertBefore( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β InsertAfter( const MutationQL&, UserPK, uint pk, SRCE )ι->HookResult{ return {}; }
		β InsertFailure( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β UpdateBefore( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β UpdateAfter( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }//includes delete/restore.
		β PurgeBefore( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β PurgeAfter( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β PurgeFailure( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }

		β AddBefore( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }
		β Add( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }
		β AddAfter( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }
//		β RemoveBefore( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }
		β Remove( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }
		β RemoveAfter( const MutationQL& mutation, UserPK executer, SRCE )ι->HookResult{ return {}; }

		β Start( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
		β Stop( const MutationQL&, UserPK, SRCE )ι->HookResult{ return {}; }
	};
#pragma warning(pop)

	namespace Hook{
		α Add( up<IQLHook>&& hook )ι->void;
		enum class Operation : uint16{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Update=0x10, Purge=0x20, Select=0x40, Start=0x80, Stop=0x100, Add=0x200, Remove=0x400 };
	}

	struct QueryHookAwaits final: TAwait<optional<jvalue>>{
		//GraphQLHookAwait( const MutationQL& _mutation, UserPK executer, Hook::Operation op, SRCE )ι;
		QueryHookAwaits( const TableQL& table_, UserPK executer, Hook::Operation op, SRCE )ι;
		α await_ready()ι->bool override;
		α await_resume()ε->optional<jvalue>;//null=not handled.
	private:
		//α CollectAwaits( const MutationQL& mutation, UserPK executer, Hook::Operation op )ι->optional<AwaitResult>;
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
		IMutationAwait( MutationQL mutation, UserPK executer, SRCE )ι:base{sl}, _mutation{move(mutation)}, _userPK{executer}{};
		virtual ~IMutationAwait()=0;
	protected:
		MutationQL _mutation;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	struct MutationAwaits : TAwait<optional<jarray>>{
		using base=TAwait<optional<jarray>>;
		MutationAwaits( MutationQL mutation, UserPK executer, Hook::Operation op, SRCE )ι;
		MutationAwaits( MutationQL mutation, UserPK executer, Hook::Operation op, uint pk=0, SRCE )ι;
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
		α Select( const TableQL& ql, UserPK executer, SRCE )ι->QueryHookAwaits;
		α InsertBefore( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α InsertAfter( uint pk, const MutationQL&, UserPK, SRCE )ι->MutationAwaits;
		α InsertFailure( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α UpdateBefore( const MutationQL&, UserPK, SRCE )ι->MutationAwaits;
		α UpdateAfter( const MutationQL&, UserPK executer, SRCE )ι->MutationAwaits;//includes delete/restore.
		α PurgeBefore( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α PurgeAfter( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α PurgeFailure( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;


		α AddBefore( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α Add( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α AddAfter( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		//α RemoveBefore( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α Remove( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α RemoveAfter( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;

		α Start( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α Stop( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
	};

	//need awaitable to throw an exception
	struct ExceptionAwait final : TAwait<jvalue>{
		ExceptionAwait( up<IException>&& e, SRCE )ι:TAwait<jvalue>{ sl }, _exception{ move(e) }{}
		α await_ready()ι->bool override{ return true; }
		α Suspend()ι->void override{}
		α await_resume()ε->jvalue override{ _exception->Throw(); return {}; }
		up<IException> _exception;
	};
}