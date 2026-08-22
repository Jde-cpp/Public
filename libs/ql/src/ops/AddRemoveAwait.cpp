#include "AddRemoveAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLHook.h>
#include <jde/ql/LocalSubscriptions.h>
#include "../types/QLColumn.h"

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };

	AddRemoveAwait::AddRemoveAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SL sl )ι:
		base{ sl },
		_mutation{ move(mutation) },
		_table{ table },
		_userPK{ userPK }
	{}

	Ω getChildParentParams( sp<DB::Column> parentCol, sp<DB::Column> childCol, const jobject& input )ε->ChildParentParams{
		ChildParentParams params{ parentCol, childCol };
		let parentColName = DB::Names::ToJson( parentCol->Name );
		if( let p = Json::FindNumber<uint>(input, parentColName); p )
			params.ParentParam = DB::Value{ parentCol->Type, *p };
		else if( let p = Json::FindNumber<uint>(input, "id"); p )
			params.ParentParam = DB::Value{ parentCol->Type, *p };
		else
			THROW( "Could not find '{}' or id in '{}'", parentColName, serialize(input) );
		let childColName = DB::Names::ToJson( childCol->Name );
		if( let p = Json::FindNumber<uint>(input, childColName); p )
			params.ChildParams.emplace_back( DB::Value{childCol->Type, *p} );
		else if( let p = Json::FindArray(input, childColName); p ){
			for( let& v : *p )
				params.ChildParams.emplace_back( DB::Value{childCol->Type, Json::AsNumber<uint>(v)} );
		}
		else
			THROW( "Could not find '{}' in '{}'", childColName, serialize(input) );

		return params;
	};

	α AddRemoveAwait::Add()ι->DB::ExecuteAwait::Task{
		let& map = *_table->Map;
		let& parentId = map.Parent->Name; let& childId =map.Child->Name;
		//#49: the column list has to grow with the placeholders.  It was fixed at the two mapped columns while `input:{…}` appended
		//a `?` per extra value, so any use of the feature emitted `insert into t(a,b)values(?,?,?)` - a column-count error, every
		//time.  Nothing in tree sends `input:` on an add, which is why a branch that could never work went unnoticed.
		string extraColumns;
		string extraParamsString;
		vector<DB::Value> extraParams;
		try{
			let input = _mutation.ExtrapolateVariables();
			if( auto defParams = Json::FindObject(input, "input"); defParams ){
				for( let& [name,value] : *defParams ){
					if( name=="id" || name==parentId || name==childId )
						continue;
					auto pColumn = _table->GetColumnPtr( DB::Names::FromJson(name) );
					extraColumns += ","+pColumn->Name;
					extraParamsString += ",?";
					extraParams.emplace_back( DB::Value{pColumn->Type, value} );
				}
			}
			let sql = Ƒ( "insert into {}({},{}{})values(?,?{})", _table->SqlName(), parentId, childId, extraColumns, extraParamsString );
			uint result{};
			for( let& p : _params.ChildParams ){
				vector<DB::Value> params{ _params.ParentParam, p };
				params.insert( end(params), begin(extraParams), end(extraParams) );
				result += co_await _table->Schema->DS()->Execute( {sql, params}, _sl );
			}
			AddAfter( result );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::AddAfter( jvalue v )ι->MutationAwaits::Task{
		try{
			co_await Hook::AddAfter( _mutation, _userPK );
			Resume( move(v) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α AddRemoveAwait::Remove()->DB::ExecuteAwait::Task{
		let& map = *_table->Map;
		let sql = Ƒ( "delete from {} where {}=? and {}=?", _table->SqlName(), map.Parent->Name, map.Child->Name );
		uint result{};
		try{
			for( let& p : _params.ChildParams ){
				vector<DB::Value> params{ _params.ParentParam, p };
				result += co_await _table->Schema->DS()->Execute( {sql, params}, _sl );
			}
			RemoveAfter( result );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::RemoveAfter( jvalue v )ι->MutationAwaits::Task{
		try{
			co_await Hook::RemoveAfter( _mutation, _userPK );
			Resume( move(v) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::AddBefore()ι->MutationAwaits::Task{
		try{
			co_await Hook::AddBefore( _mutation, _userPK );
			Add();
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::AddHook()ι->MutationAwaits::Task{
		try{
			auto y = co_await Hook::Add(_mutation, _userPK);
			THROW_IF( !y, "Hook::Add returned null." );
			TRACE( "AddHook::Add returned '{}'", serialize(*y) );
			Resume( move(*y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::RemoveHook()ι->MutationAwaits::Task{
		try{
			auto y = co_await Hook::Remove( _mutation, _userPK );
			THROW_IF( !y, "Hook::RemoveHook returned null." );
			Resume( move(*y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::Suspend()ι->void{
		try{
			_table->Authorize( Access::ERights::Update, _userPK, _sl );
			if( _mutation.Type==EMutationQL::Add && _table->AddProc.size() ){
				AddHook();
				return;
			}
			if( _mutation.Type==EMutationQL::Remove && _table->RemoveProc.size() ){
				RemoveHook();
				return;
			}
			THROW_IF( !_table->Map, "'{}' does not support add/remove.", _table->Name );
			_params = getChildParentParams( _table->Map->Parent, _table->Map->Child, _mutation.ExtrapolateVariables() );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
			return;
		}
		if( _mutation.Type==EMutationQL::Add ){
			AddBefore();
		}
		else if( _mutation.Type==EMutationQL::Remove )
			Remove();
		else
			ASSERT( false );
	}
	α AddRemoveAwait::Resume( jvalue&& v )ι->void{
		//#47: a statement that matched nothing is not an event.  OnMutation never looked at the result - an integer rowCount is not
		//an object, so `available` was the args alone and the id the client sent went straight to the listeners.  `deleteUser( id:5,
		//name:"nomatch" )` ands the extra arg into the where clause, updates 0 rows, and still had AccessListener mark user 5
		//deleted in memory - "User is deleted" for every later request by 5, until restart, with the row untouched.
		if( !v.is_number() || v.to_number<uint>() )
			Subscriptions::OnMutation( _mutation, v );
		base::Resume( move(v) );
	}
}