#pragma once
#include <jde/db/usings.h>
#include <jde/fwk/co/AnyAwait.h>
#include <jde/ql/types/MutationQL.h>

namespace Jde::QL{
	struct MutationQL; struct TableQL;
	struct IQLHook{
		using HookResult=up<TAwait<jvalue>>;
		virtual ~IQLHook() = default;

		β Select( const TableQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β InsertBefore( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β InsertAfter( const MutationQL&, UserPK, uint, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β InsertFailure( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β UpdateBefore( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β UpdateAfter( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }//includes delete/restore.
		β PurgeBefore( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β PurgeAfter( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β PurgeFailure( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }

		β AddBefore( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β Add( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β AddAfter( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β Remove( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β RemoveAfter( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }

		β Start( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
		β Stop( const MutationQL&, UserPK, SL=SRCE_CUR )ι->HookResult{ return {}; }
	};

	namespace Hook{
		α Add( up<IQLHook>&& hook )ι->void;
		enum class Operation : uint16{ Before=0x1, After=0x2, Failure=0x4, Insert=0x8, Update=0x10, Purge=0x20, Start=0x80, Stop=0x100, Add=0x200, Remove=0x400 };//not the dispatch key any more - a tag OpcQLHook keeps for what it was asked.
		//#13: the hooks are ι by declaration, but that is a promise this loop used to take on faith - a hook that threw took the
		//process down from a noexcept frame, past every try/catch on the way in.  Now it fails the request instead: the refusal is
		//returned (and asking stops there), parked by the caller and rethrown from its await_resume, which is what the awaiter is
		//already prepared for.  Everything the hooks handed back is in `awaitables`.
		α Collect( const function<IQLHook::HookResult(IQLHook&)>& ask, vector<up<TAwait<jvalue>>>& awaitables )ι->up<Exception>;
	}

	//What asking every registered hook has in common:  the collection in await_ready, the refusal parked there, and the coroutine
	//that awaits whatever the hooks returned.  Ask() names the virtual asked; Fold() shapes the answers.  An AnyAwait, so an op
	//awaits it from its own frame, and "nothing claimed it" or a refusal completes it inside await_ready - the base's await_resume
	//hands back the nullopt or rethrows.
	template<class TResult>
	struct HookAwaits : AnyAwait<TResult>{
		using base=AnyAwait<TResult>;
		HookAwaits( UserPK userPK, SRCE )ι:base{ sl }, _userPK{ userPK }{}
		α await_ready()ι->bool override;
	protected:
		β Ask( IQLHook& hook )ε->IQLHook::HookResult=0;//ε, not ι: a hook that lies about its specification must reach Collect's catch, not terminate.
		β Fold( jarray&& results )ι->TResult=0;
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		UserPK _userPK;
	private:
		vector<up<TAwait<jvalue>>> _awaitables;
	};

	Ŧ HookAwaits<T>::await_ready()ι->bool{
		if( auto refusal = Hook::Collect( [this](IQLHook& h){ return Ask(h); }, _awaitables ); refusal )
			base::_error = move( refusal );//#13: rethrown by the awaiter's co_await.
		else if( _awaitables.empty() )
			base::_result = T{};//nullopt: nothing claimed it.
		return base::_error || base::_result;
	}
	Ŧ HookAwaits<T>::Execute()ι->TAwait<jvalue>::Task{
		jarray results;
		try{
			for( auto& awaitable : _awaitables )
				results.push_back( co_await *awaitable );
		}
		catch( runtime_error& e ){
			base::ResumeExp( move(e) );
			co_return;
		}
		base::Resume( Fold(move(results)) );
	}

	struct QueryHookAwaits final: HookAwaits<optional<jvalue>>{
		using base=HookAwaits<optional<jvalue>>;
		QueryHookAwaits( const TableQL& table, UserPK executer, SRCE )ι:base{ executer, sl }, _ql{ table }{}
	private:
		α Ask( IQLHook& hook )ε->IQLHook::HookResult override{ return hook.Select( _ql, _userPK, _sl ); }
		α Fold( jarray&& results )ι->optional<jvalue> override{ return results.size()==1 ? move(results[0]) : jvalue{ move(results) }; }//one answer is the answer; several are a list.
		const TableQL& _ql;
	};

	//The IQLHook virtual a MutationAwaits asks - a member pointer for the fifteen that take (mutation, executer, sl), a lambda for
	//InsertAfter, which also takes the pk.  This is what the Operation bitmask used to be switched back into, case by case.
	using MutationHook = function<IQLHook::HookResult( IQLHook&, const MutationQL&, UserPK, SL )>;
	struct MutationAwaits final: HookAwaits<optional<jarray>>{
		using base=HookAwaits<optional<jarray>>;
		MutationAwaits( MutationQL mutation, UserPK executer, MutationHook hook, SRCE )ι;
	private:
		α Ask( IQLHook& h )ε->IQLHook::HookResult override{ return _hook( h, _mutation, _userPK, _sl ); }
		α Fold( jarray&& results )ι->optional<jarray> override{ return move( results ); }
		MutationQL _mutation;
		MutationHook _hook;
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
		α Remove( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α RemoveAfter( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;

		α Start( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
		α Stop( const MutationQL& m, UserPK executer, SRCE )ι->MutationAwaits;
	};
}