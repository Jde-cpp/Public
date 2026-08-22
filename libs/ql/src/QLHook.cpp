#include <jde/ql/QLHook.h>
//#include <jde/fwk/collections/Vector.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::QL{
	vector<up<IQLHook>> _hooks;
	α Hook::Add( up<IQLHook>&& hook )ι->void{
		_hooks.push_back( move(hook) );
	}
	using Hook::Operation;
	QueryHookAwaits::QueryHookAwaits( const TableQL& ql, UserPK userPK_, SL sl )ι:
		TAwait<optional<jvalue>>{ sl },
		_ql{ ql },
		_userPK{ userPK_ }
	{}

	//#13: the hooks are ι by declaration, but that is a promise this loop used to take on faith - a hook that threw took the
	//process down from a noexcept frame, past every try/catch on the way in.  Now it fails the query instead: the exception is
	//parked and rethrown from await_resume, which is what the caller is already prepared for.
	α QueryHookAwaits::await_ready()ι->bool{
		//sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size() );
		for( auto ppHook = _hooks.begin(); !_exception && ppHook!=_hooks.end(); ++ppHook ){
			auto& hook = **ppHook;
			try{
				if( auto p = hook.Select( _ql, _userPK, _sl ); p )
					_awaitables.emplace_back( move(p) );
			}
			catch( Exception& e ){ _exception = e.Move(); }
			catch( runtime_error& e ){ _exception = mu<Exception>( move(e) ); }
		}
		return _exception || _awaitables.empty();
	}
	α QueryHookAwaits::await_resume()ε->optional<jvalue>{
		if( _exception )
			_exception->Throw();
		return Promise()
			? TAwait<optional<jvalue>>::await_resume()
			: optional<jvalue>{};
	}

	α QueryHookAwaits::Execute()ι->TAwait<jvalue>::Task{
		jarray results;
		for( auto& awaitable : _awaitables ){
			try{
				results.push_back( co_await *awaitable );
			}
			catch( runtime_error& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
		Resume( results.size()==1 ? move(results[0]) : jvalue{results} );
	}


	MutationAwaits::MutationAwaits( MutationQL m, UserPK executer, Hook::Operation op, uint pk, SL sl )ι:
		base{ sl },
		_mutation{ move(m) }, //the by-value parameter is ours: Hook::Start/Stop's copy off the caller's const& is the only one needed.
		_op{ op },
		_pk{ pk },
		_userPK{ executer }
	{}
#pragma warning(disable: 4063)
	α MutationAwaits::await_ready()ι->bool{
		//sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size() );
		for( auto ppHook = _hooks.begin(); !_exception && ppHook!=_hooks.end(); ++ppHook ){
			auto& hook = **ppHook;
			up<TAwait<jvalue>> p;
			try{
			switch( _op ){
				using enum Hook::Operation;
				case (Add | Before): p = hook.AddBefore( _mutation, _userPK ); break;
				case Add: p = hook.Add( _mutation, _userPK ); break;
				case (Add | After): p = hook.AddAfter( _mutation, _userPK ); break;
				//case (Remove | Before): p = hook.RemoveBefore( _mutation, _userPK ); break;
				case Remove: p = hook.Remove( _mutation, _userPK ); break;
				case (Remove | After): p = hook.RemoveAfter( _mutation, _userPK ); break;
				case (Insert | Before): p = hook.InsertBefore( _mutation, _userPK ); break;
				case (Insert | After): p = hook.InsertAfter( _mutation, _userPK, _pk ); break;
				case (Insert | Failure): p = hook.InsertFailure( _mutation, _userPK ); break;
				case (Purge | Before): p = hook.PurgeBefore( _mutation, _userPK ); break;
				case (Purge | After): p = hook.PurgeAfter( _mutation, _userPK ); break;
				case (Purge | Failure): p = hook.PurgeFailure( _mutation, _userPK ); break;
				case (Update | Before): p = hook.UpdateBefore( _mutation, _userPK ); break;
				case (Update | After): p = hook.UpdateAfter( _mutation, _userPK ); break;
				case Start: p = hook.Start( _mutation, _userPK ); break;
				case Stop: p = hook.Stop( _mutation, _userPK ); break;
			}
			}//#13: same boundary as QueryHookAwaits above - a throwing hook fails the mutation, it does not end the process.
			catch( Exception& e ){ _exception = e.Move(); }
			catch( runtime_error& e ){ _exception = mu<Exception>( move(e) ); }
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _exception || _awaitables.empty();
	}
	α MutationAwaits::Suspend()ι->void{
		Execute();
	}
	α MutationAwaits::Execute()ι->IMutationAwait::Task{
		try{
			jarray y;
 			for( auto& awaitable : _awaitables )
				y.push_back( co_await *awaitable );
			Resume( y );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α MutationAwaits::await_resume()ε->optional<jarray>{
		if( _exception )
			_exception->Throw();
		return Promise()
			? TAwait<optional<jarray>>::await_resume()
			: optional<jarray>{};
	}
	α Hook::Select( const TableQL& ql, UserPK executer, SL sl )ι->QueryHookAwaits{ return QueryHookAwaits{ ql, executer, sl }; };

	α Hook::AddBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add|Operation::Before, 0, sl }; }
	α Hook::Add( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add, 0, sl }; }
	α Hook::AddAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add|Operation::After, 0, sl }; }
	//α Hook::RemoveBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove|Operation::Before, 0, sl }; }
	α Hook::Remove( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove, 0, sl }; }
	α Hook::RemoveAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove|Operation::After, 0, sl }; }
	α Hook::InsertBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::Before, 0, sl }; }
	α Hook::InsertAfter( uint pk, const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::After, pk, sl }; }//#51: named and forwarded - it used to be dropped here.
	α Hook::InsertFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::Failure, 0, sl }; }
	α Hook::PurgeBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::Before, 0, sl }; }
	α Hook::PurgeAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::After, 0, sl }; }
	α Hook::PurgeFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::Failure, 0, sl }; }
	α Hook::Start( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Start, 0, sl }; }
	α Hook::Stop( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Stop, 0, sl }; }
	α Hook::UpdateBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Update|Operation::Before, 0, sl }; }
	α Hook::UpdateAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Update|Operation::After, 0, sl }; }
}