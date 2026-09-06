#pragma once
#include <jde/fwk/co/AnyAwait.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>

namespace Jde::DB{ struct Table; }
namespace Jde::QL{
	//What the crud ops (Insert/Update/Purge/AddRemove) share:  the mutation, its table and executer, the refusal await_ready parks
	//before anything suspends, and the one place a result reaches subscribers.  An AnyAwait: any coroutine can await an op, and a
	//refusal - or nothing to do - completes it inside await_ready natively, in _error/_result, with no promise involved.
	struct IMutationAwait : AnyAwait<jvalue>{
		using base=AnyAwait<jvalue>;
		IMutationAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι:
			base{ sl }, _mutation{ move(mutation) }, _table{ move(table) }, _userPK{ userPK }{}
		virtual ~IMutationAwait()=0;
	protected:
		α Resume( jvalue&& v )ι->void;//publishes, then hands the value to the awaiter - the caller's last use of `this`.
		α Publish( const jvalue& v )Ι->void;
		α Refuse( Exception&& e )ι->void{ _error = e.Move(); }//#25: raised in await_ready (an in-memory acl check needs no suspension), rethrown by the awaiter's co_await.
		α Refused()Ι->bool{ return _error!=nullptr; }

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
	};
	inline IMutationAwait::~IMutationAwait(){}

	//#47: a statement that matched nothing is not an event.  OnMutation never looked at the result - an integer rowCount is not
	//an object, so `available` was the args alone and the id the client sent went straight to the listeners.  `deleteUser( id:5,
	//name:"nomatch" )` ands the extra arg into the where clause, updates 0 rows, and still had AccessListener mark user 5
	//deleted in memory - "User is deleted" for every later request by 5, until restart, with the row untouched.  The insert
	//half:  its result is one object per statement, and no row means no statement inserted one - publishing `null` told
	//subscribers a row they could not identify had appeared.  (OnMutation reads a non-empty array's first element itself.)
	Ξ IMutationAwait::Publish( const jvalue& v )Ι->void{
		const uint rows = v.is_number() ? v.to_number<uint>() : v.is_array() ? (uint)v.get_array().size() : 1;
		if( rows )
			Subscriptions::OnMutation( _mutation, v );
	}
	Ξ IMutationAwait::Resume( jvalue&& v )ι->void{
		Publish( v );
		base::Resume( move(v) );
	}
}