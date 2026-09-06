#include <jde/ql/QLHook.h>

namespace Jde::QL{
	vector<up<IQLHook>> _hooks;
	α Hook::Add( up<IQLHook>&& hook )ι->void{
		_hooks.push_back( move(hook) );
	}
	α Hook::Collect( const function<IQLHook::HookResult(IQLHook&)>& ask, vector<up<TAwait<jvalue>>>& awaitables )ι->up<Exception>{
		//sl l{ _hooks.Mutex };
		up<Exception> refusal;
		awaitables.reserve( _hooks.size() );
		for( auto ppHook = _hooks.begin(); !refusal && ppHook!=_hooks.end(); ++ppHook ){
			try{
				if( auto p = ask(**ppHook); p )
					awaitables.emplace_back( move(p) );
			}
			catch( Exception& e ){ refusal = e.Move(); }
			catch( runtime_error& e ){ refusal = mu<Exception>( move(e) ); }
		}
		return refusal;
	}

	MutationAwaits::MutationAwaits( MutationQL m, UserPK executer, MutationHook hook, SL sl )ι:
		base{ executer, sl },
		_mutation{ move(m) }, //the by-value parameter is ours: Hook::Start/Stop's copy off the caller's const& is the only one needed.
		_hook{ move(hook) }
	{}

	α Hook::Select( const TableQL& ql, UserPK executer, SL sl )ι->QueryHookAwaits{ return QueryHookAwaits{ ql, executer, sl }; }

	α Hook::AddBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::AddBefore, sl }; }
	α Hook::Add( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::Add, sl }; }
	α Hook::AddAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::AddAfter, sl }; }
	α Hook::Remove( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::Remove, sl }; }
	α Hook::RemoveAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::RemoveAfter, sl }; }
	α Hook::InsertBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::InsertBefore, sl }; }
	α Hook::InsertAfter( uint pk, const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{//#51: named and forwarded - it used to be dropped here.
		return MutationAwaits{ m, executer, [pk]( IQLHook& h, const MutationQL& m2, UserPK e, SL sl2 )ι{ return h.InsertAfter( m2, e, pk, sl2 ); }, sl };
	}
	α Hook::InsertFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::InsertFailure, sl }; }
	α Hook::PurgeBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::PurgeBefore, sl }; }
	α Hook::PurgeAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::PurgeAfter, sl }; }
	α Hook::PurgeFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::PurgeFailure, sl }; }
	α Hook::Start( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::Start, sl }; }
	α Hook::Stop( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::Stop, sl }; }
	α Hook::UpdateBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::UpdateBefore, sl }; }
	α Hook::UpdateAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, &IQLHook::UpdateAfter, sl }; }
}