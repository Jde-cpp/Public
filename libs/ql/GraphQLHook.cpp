#include <jde/ql/GraphQLHook.h>
#include <jde/framework/collections/Vector.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde::QL{
	Vector<up<IGraphQLHook>> _hooks;
	α Hook::Add( up<IGraphQLHook>&& hook )ι->void{
		_hooks.push_back( move(hook) );
	}
	using Hook::Operation;
#pragma warning(disable:4063)
	α GraphQLHookAwait::CollectAwaits( const MutationQL& mutation, UserPK userPK, Operation op )ι->optional<AwaitResult>{
		up<IAwait> p;
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

	α GraphQLHookAwait::CollectAwaits( const TableQL& ql, UserPK userPK, Operation op )ι->optional<AwaitResult>{
		up<IAwait> p;
		sl l{ _hooks.Mutex };
		_awaitables.reserve( _hooks.size(l) );
		for( auto ppHook = _hooks.begin(l); ppHook!=_hooks.end(l); ++ppHook ){
			auto& hook = **ppHook;
			switch( op ){
				case Operation::Select: p = hook.Select( ql, userPK ); break;
			}
			if( p )
				_awaitables.emplace_back( move(p) );
		}
		return _awaitables.empty() ? optional<AwaitResult>{up<jobject>()} : nullopt;
	}

	α GraphQLHookAwait::AwaitMutation( HCoroutine h )ι->Task{
		uint y{};
		for( auto& awaitable : _awaitables ){
			try{
				y+=*( co_await *awaitable ).UP<uint>();
			}
			catch( IException& e ){
				Resume( move(e), h );
				co_return;
			}
		}
		Resume( mu<uint>(y), h );
	}

	α GraphQLHookAwait::Await( HCoroutine h )ι->Task{
		up<jobject> pResult;
		for( auto& awaitable : _awaitables ){
			try{
				pResult = awaitp( jobject, *awaitable );
			}
			catch( IException& e ){
				Resume( move(e), h );
				co_return;
			}
		}
		Resume( move(pResult), h );
	}

	GraphQLHookAwait::GraphQLHookAwait( const MutationQL& mutation, UserPK userPK_, Operation op_, SL sl )ι:
		AsyncReadyAwait{
			[&, userPK=userPK_, op=op_](){return CollectAwaits(mutation, userPK, op);},
			[&](HCoroutine h){AwaitMutation(h);}, sl, "GraphQLHookAwait" }
	{}

	GraphQLHookAwait::GraphQLHookAwait( const TableQL& ql, UserPK userPK_, Operation op_, SL sl )ι:
		AsyncReadyAwait{ [&, userPK=userPK_, op=op_](){return CollectAwaits(ql, userPK, op);},
			[&](HCoroutine h){Await(h);}, sl, "GraphQLHookAwait" }
	{}

	MutationAwaits::MutationAwaits( sp<MutationQL> mutation, UserPK userPK, Hook::Operation op, SL sl )ι:
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
		uint result{};
		for( auto& awaitable : _awaitables ){
			up<IException> pException;
			[&]()->IMutationAwait::Task {
				try{
					result+=co_await *awaitable;
				}
				catch( IException& e ){
					pException = e.Move();
				}
			}();
			if( pException ){
				Promise()->SetError( move(*pException) );
				break;
			}
		}
		if( !Promise()->Error() )
			Promise()->SetValue( move(result) );
		_h.resume();
	}

	α Hook::Select( const TableQL& ql, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ ql, userPK, Operation::Select, sl }; };
	α Hook::InsertBefore( const MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Insert|Operation::Before, sl }; }
	α Hook::InsertFailure( const MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Insert|Operation::Failure, sl }; }
	α Hook::PurgeBefore( const MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Purge|Operation::Before, sl }; }
	α Hook::PurgeFailure( const MutationQL& mutation, UserPK userPK, SL sl )ι->GraphQLHookAwait{ return GraphQLHookAwait{ mutation, userPK, Operation::Purge|Operation::Failure, sl }; }
	α Hook::Start( sp<MutationQL> mutation, UserPK userId, SL sl )ι->MutationAwaits{ return MutationAwaits{ mutation, userId, Operation::Start, sl }; }
	α Hook::Stop( sp<MutationQL> mutation, UserPK userId, SL sl )ι->MutationAwaits{; return MutationAwaits{ mutation, userId, Operation::Stop, sl }; }

}