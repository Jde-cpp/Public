#include <jde/ql/QLHook.h>
#include <jde/framework/collections/Vector.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::QL{
	Vector<up<IQLHook>> _hooks;
	α Hook::Add( up<IQLHook>&& hook )ι->void{
		_hooks.push_back( move(hook) );
	}
	using Hook::Operation;
	QueryHookAwaits::QueryHookAwaits( const TableQL& ql, UserPK userPK_, Operation op_, SL sl )ι:
		TAwait<optional<jvalue>>{ sl },
		_op{op_},
		_ql{ ql },
		_userPK{ userPK_ }
	{}

	α QueryHookAwaits::await_ready()ι->bool{
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			if( auto p = hook.Select( _ql, _userPK, _sl ); p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty();
	}
	α QueryHookAwaits::await_resume()ε->optional<jvalue>{
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
			catch( IException& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
		Resume( results.size()==1 ? move(results[0]) : jvalue{results} );
	}

	MutationAwaits::MutationAwaits( MutationQL mutation, UserPK executer, Hook::Operation op, SL sl )ι:
		MutationAwaits{ mutation, executer, op, 0, sl }
	{}

	MutationAwaits::MutationAwaits( MutationQL m, UserPK executer, Hook::Operation op, uint pk, SL sl )ι:
		base{ sl },
		_mutation{ m },
		_op{ op },
		_pk{ pk },
		_userPK{ executer }
	{}
#pragma warning(disable: 4063)
	α MutationAwaits::await_ready()ι->bool{
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			up<TAwait<jvalue>> p;
			switch( _op ){
				using enum Hook::Operation;
				case (Add | Before): p = hook.AddBefore( _mutation, _userPK ); break;
				case Add: p = hook.Add( _mutation, _userPK ); break;
				case (Add | After): p = hook.AddAfter( _mutation, _userPK ); break;
				//case (Remove | Before): p = hook.RemoveBefore( _mutation, _userPK ); break;
				case Remove: p = hook.Remove( _mutation, _userPK ); break;
				case (Remove | After): p = hook.RemoveAfter( _mutation, _userPK ); break;
				case (Insert | Before): p = hook.InsertBefore( _mutation, _userPK, _sl ); break;
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
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty();
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
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α MutationAwaits::await_resume()ε->optional<jarray>{
		return Promise()
			? TAwait<optional<jarray>>::await_resume()
			: optional<jarray>{};
	}
	α Hook::Select( const TableQL& ql, UserPK executer, SL sl )ι->QueryHookAwaits{ return QueryHookAwaits{ ql, executer, Operation::Select, sl }; };

	α Hook::AddBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add|Operation::Before, sl }; }
	α Hook::Add( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add, sl }; }
	α Hook::AddAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Add|Operation::After, sl }; }
	//α Hook::RemoveBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove|Operation::Before, sl }; }
	α Hook::Remove( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove, sl }; }
	α Hook::RemoveAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Remove|Operation::After, sl }; }
	α Hook::InsertBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::Before, sl }; }
	α Hook::InsertAfter( uint pk, const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::After, pk, sl }; }
	α Hook::InsertFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Insert|Operation::Failure, sl }; }
	α Hook::PurgeBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::Before, sl }; }
	α Hook::PurgeAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::After, sl }; }
	α Hook::PurgeFailure( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Purge|Operation::Failure, sl }; }
	α Hook::Start( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Start, sl }; }
	α Hook::Stop( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Stop, sl }; }
	α Hook::UpdateBefore( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Stop, sl }; }
	α Hook::UpdateAfter( const MutationQL& m, UserPK executer, SL sl )ι->MutationAwaits{ return MutationAwaits{ m, executer, Operation::Update|Operation::After, sl }; }
}