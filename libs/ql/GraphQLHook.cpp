#include <jde/ql/GraphQLHook.h>
#include <jde/framework/collections/Vector.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::QL{
	Vector<up<IGraphQLHook>> _hooks;
	α Hook::Add( up<IGraphQLHook>&& hook )ι->void{
		_hooks.push_back( move(hook) );
	}
	using Hook::Operation;
/*#pragma warning(disable:4063)
	QueryHookAwaits::QueryHookAwaits( const MutationQL& mutation, UserPK userPK_, Operation op_, SL sl )ι:
		TAwait<jvalue>{ sl },
		_op{op_},
		_ql{ mutation },
		_userPK{ userPK_ }
	{}
*/
	QueryHookAwaits::QueryHookAwaits( const TableQL& ql, UserPK userPK_, Operation op_, SL sl )ι:
		TAwait<optional<jvalue>>{ sl },
		_op{op_},
		_ql{ ql },
		_userPK{ userPK_ }
	{}

//[&, userPK=userPK_, op=op_](){return CollectAwaits(ql, userPK, op);},
//			[&](HCoroutine h){Await(h);}, sl, "QueryHookAwaits" }

	α QueryHookAwaits::await_ready()ι->bool{
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			if( auto p = hook.Select( _ql, _userPK ); p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty();
	}
	α QueryHookAwaits::await_resume()ε->optional<jvalue>{
		return Promise()
			? TAwait<optional<jvalue>>::await_resume()
			: optional<jvalue>{};
	}
/*
	α QueryHookAwaits::CollectAwaits( const MutationQL& mutation, UserPK userPK, Operation op )ι->optional<AwaitResult>{
		up<TAwait<jvalue>> p;
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			switch( op ){ using enum Hook::Operation;
				case (Insert | Before): p = hook.InsertBefore( mutation, userPK ); break;
				case (Insert | Failure): p = hook.InsertFailure( mutation, userPK ); break;
				case (Purge | Before): p = hook.PurgeBefore( mutation, userPK ); break;
				case (Purge | Failure): p = hook.PurgeFailure( mutation, userPK ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty() ? optional<AwaitResult>{up<uint>{}} : nullopt;
	}

			//[&, userPK=userPK_, op=op_](){return CollectAwaits(mutation, userPK, op);},
			//[&](HCoroutine h){AwaitMutation(h);}, sl, "QueryHookAwaits" }
	α QueryHookAwaits::AwaitMutation()ι->TAwait<jvalue>::Task{
		jarray y;
		for( auto& awaitable : _awaitables ){
			try{
				y.push_back( co_await *awaitable );
			}
			catch( IException& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
		Resume( move(y) );
	}
*/
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

	MutationAwaits::MutationAwaits( MutationQL mutation, UserPK userPK, Hook::Operation op, SL sl )ι:
		base{ mutation, userPK, sl },
		 _op{op}
	{}

	α MutationAwaits::await_ready()ι->bool{
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			up<IMutationAwait> p;
			switch( _op ){
				case Operation::Start: p = hook.Start( _mutation, _userPK ); break;
				case Operation::Stop: p = hook.Stop( _mutation, _userPK ); break;
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
		jarray y;
		try{
 			for( auto& awaitable : _awaitables ){
				if( auto result = co_await *awaitable; result )
					y.push_back( *result );
			}
			Resume( y.size()==1 ? move(y[0]) : jvalue{y} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α MutationAwaits::await_resume()ι->optional<jvalue>{
		return Promise()
			? TAwait<optional<jvalue>>::await_resume()
			: optional<jvalue>{};
	}

	α Hook::Select( const TableQL& ql, UserPK userPK, SL sl )ι->QueryHookAwaits{ return QueryHookAwaits{ ql, userPK, Operation::Select, sl }; };
	α Hook::InsertBefore( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Insert|Operation::Before, sl }; }
	α Hook::InsertFailure( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Insert|Operation::Failure, sl }; }
	α Hook::PurgeBefore( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Purge|Operation::Before, sl }; }
	α Hook::PurgeFailure( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Purge|Operation::Failure, sl }; }
	α Hook::Start( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Start, sl }; }
	α Hook::Stop( const MutationQL& mutation, UserPK userPK, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userPK, Operation::Stop, sl }; }

}