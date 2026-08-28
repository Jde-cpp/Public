#include "UpdateAwait.h"
#include <jde/ql/QLHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/UpdateClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/access/IAcl.h>
#include <jde/ql/LocalSubscriptions.h>
#include "../types/QLColumn.h"

#define let const auto

namespace Jde::QL{
	using DB::Value;
	α GetEnumValues( const DB::View& table, SRCE )ε->flat_map<uint,string>; //ops/SelectAwait.cpp - SelectEnumSync, i.e. cached but blocking.
	α ToFlags( const flat_map<uint,string>& values, const jarray& flags, sv memberName, SRCE )ε->uint;//ops/SelectAwait.cpp - shared with InsertAwait (#48).

	UpdateAwait::UpdateAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SL sl )ι:
		base{ sl },
		_mutation{ move(mutation) },
		_table{ table },
		_userPK{ userPK }
	{}

	//Authorization only - it is an in-memory acl check, so an unauthorized mutation is still refused without suspending.  The
	//clauses are built in Build() instead: resolving a flags column's names needs the enum table, and await_ready can not
	//co_await, which is what forced the old BlockAwait onto whatever thread happened to be running the mutation.
	α UpdateAwait::await_ready()ι->bool{
		try{
			THROW_IF( !_table, "Table not found for mutation '{}'.", _mutation.ToString() );
			_table->Authorize( _mutation.Type==EMutationQL::Update ? Access::ERights::Update : Access::ERights::Delete, _userPK, _sl );
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		return _exception!=nullptr;
	}

	//The flags columns this mutation actually sets, over the same extension chain and the same arg predicate CreateUpdate uses.
	α UpdateAwait::FlagTables( const DB::Table& table, vector<sp<DB::View>>& y )ι->void{
		if( let extends = table.IsView() ? nullptr : AsTable(table).Extends; extends )
			FlagTables( *extends, y );
		for( let& c : table.Columns ){
			if( !c->Updateable || c->IsPK() || c->SKIndex || !c->IsFlags() )//#46: the same predicate CreateUpdate uses, so the two walks stay in step.
				continue;
			let jvalue = _input.if_contains( QLColumn{c}.MemberName() );
			if( let flags = jvalue ? jvalue->if_array() : nullptr; flags && flags->size() )
				y.push_back( QLColumn{c}.Column->PKTable );
		}
	}

	//Prefetched by Build().  The fallback is the old path (cached, but it blocks); it only runs if the two walks ever
	//disagree, so a future divergence costs a stall instead of a failed mutation.
	α UpdateAwait::EnumValues( const DB::View& enumTable )ε->const flat_map<uint,string>&{
		auto p = _enums.find( enumTable.Name );
		if( p==_enums.end() ){
			WARNT( ELogTags::QL, "[{}]enum values were not prefetched - falling back to a blocking lookup.", enumTable.Name );
			p = _enums.emplace( enumTable.Name, GetEnumValues(enumTable) ).first;
		}
		return p->second;
	}

	α UpdateAwait::CreateUpdate( const DB::Table& table )ε->DB::Value{
		let pExtendedFromTable = table.IsView() ? nullptr : AsTable(table).Extends;
		DB::Value rowKey = pExtendedFromTable  ? CreateUpdate(*pExtendedFromTable) : DB::Value{};

		DB::UpdateClause update;
		if( pExtendedFromTable )
			update.Where.Add( table.SurrogateKeys[0], rowKey );
		else{
			let& args = _input;
			if( let id = table.FindPK() ? Json::FindNumber<uint>(args, "id") : optional<uint>{}; id )
				update.Where.Add( table.FindPK(), DB::Value{*id} );
			else if( let name = table.FindColumn("name") ? Json::FindSV(args, "name") : optional<sv>{}; name )
				update.Where.Add( table.FindColumn("name"), DB::Value{string{*name}} );
			else if( let target = table.FindColumn("target") ? Json::FindSV(args, "target") : optional<sv>{}; target )
				update.Where.Add( table.FindColumn("target"), DB::Value{string{*target}} );
			else
				THROW( "Could not get criteria from {}", serialize(args) );
			rowKey = update.Where.Params()[0];
		}

		let& input = _input;
		for( let& c : table.Columns ){
			//#46: a key column is not a settable one.  `updateable` defaults to true and none of common-meta's sequenced pk
			//shapes says otherwise, so `updateResource( id:6, resourceId:5006 )` emitted `set resource_id = 5006` and renumbered
			//the row out from under Authorize's in-memory map;  only sqlite's FKs (created without ON UPDATE CASCADE) stopped
			//the identities case, and then only where a child row happened to exist.
			if( !c->Updateable || c->IsPK() || c->SKIndex )
				continue;

			const QLColumn qlColumn{ c };
			let jvalue = input.if_contains( qlColumn.MemberName() );
			if( !jvalue )
				continue;
			if( !c->IsFlags() )
				update.Add( c, DB::Value{c->Type, *jvalue} );
			else{
				uint value = 0;
				if( let flags = jvalue->if_array(); flags && flags->size() )
					value = ToFlags( EnumValues(qlColumn.Table()), *flags, qlColumn.MemberName() );
				else if( jvalue->is_number() )
					value = Json::AsNumber<uint>( *jvalue );
				update.Add( c, {value} );
			}
		}
		THROW_IF( update.Where.Empty(), "There is no where clause." );
		if( update.Values.size() ){
			//#45: an extension row is keyed by whatever the parent's where clause was keyed by, and on the name/target branches that
			//is the literal, not the row - `update access_users set … where access_users.identity_id='bob'` matched nothing and the
			//parent's row count was reported as success.  Resolving the parent pk first would mean a blocking select on the mutation
			//path, so the shape is refused.  Only when this statement has something to set:  `updateGroup( name:…, description:… )`
			//touches parent columns only, so no extension statement is emitted and the natural key is still the right way to say it.
			THROW_IF( pExtendedFromTable && !Json::FindNumber<uint>(_input, "id"), "'{}' extends '{}', so setting one of its own columns needs an id - name and target key the parent, not its extension.", table.Name, pExtendedFromTable->Name );
			_updates.push_back( move(update) );
		}
		return rowKey;
	}
	α UpdateAwait::CreateDeleteRestore( const DB::Table& table )ε->void{
		DB::UpdateClause update;
		auto deleted = table.GetColumnPtr( "deleted" );
		let value = _mutation.Type==EMutationQL::Delete ? DB::Value{"$now"} : DB::Value{};
		update.Add( deleted, value );
		auto key = _mutation.GetKey();
		update.Where.Add( key.IsPK() ? deleted->Table->GetPK() : deleted->Table->GetColumnPtr("target"), DB::Value::FromKey(key) );//deleted=main table, table=possibly extension table.
		for( let& arg : _mutation.ExtrapolateVariables() ){
			if( arg.key()=="id" || arg.key()=="target" )
				continue;
			if( let column = table.FindColumn( arg.key() ); column )
				update.Where.Add( column, DB::Value{column->Type, arg.value()} );
		}
		_updates.push_back( move(update) );
	}

	//The clauses are built here rather than in await_ready: a flags column resolves its names through the enum table, and this
	//is where waiting on the db is free.  Each awaitable dictates its caller's return type, so this step is CacheAwait's and
	//hands off to UpdateBefore - the same chaining InsertAwait uses.
	α UpdateAwait::Build()ι->TTask<flat_map<uint,string>>{
		try{
			_input = _mutation.ExtrapolateVariables();
			if( _mutation.Type==EMutationQL::Update ){
				vector<sp<DB::View>> enumTables;
				FlagTables( *_table, enumTables );
				for( let& enumTable : enumTables ){
					if( !_enums.contains(enumTable->Name) )
						_enums.emplace( enumTable->Name, co_await enumTable->Schema->DS()->SelectEnum<uint,string>(*enumTable, Cache::DefaultDuration(), _sl) );
				}
				CreateUpdate( *_table );
				THROW_IF( _updates.empty(), "There is nothing to update." );
			}
			else
				CreateDeleteRestore( *_table );
			UpdateBefore();
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α UpdateAwait::UpdateBefore()ι->MutationAwaits::Task{
		try{
			co_await Hook::UpdateBefore( _mutation, _userPK );
			Execute();
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::Execute()ι->DB::ExecuteAwait::Task{
		try{
			uint rowCount{};
			for( auto& update : _updates )
				rowCount += co_await _table->Schema->DS()->Execute( update.Move() );
			UpdateAfter( rowCount );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::UpdateAfter( uint rowCount )ι->MutationAwaits::Task{
		try{
			co_await Hook::UpdateAfter( _mutation, _userPK );
			Resume( jvalue{rowCount} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::Resume( jvalue&& v )ι->void{
		//#47: a statement that matched nothing is not an event.  OnMutation never looked at the result - an integer rowCount is not
		//an object, so `available` was the args alone and the id the client sent went straight to the listeners.  `deleteUser( id:5,
		//name:"nomatch" )` ands the extra arg into the where clause, updates 0 rows, and still had AccessListener mark user 5
		//deleted in memory - "User is deleted" for every later request by 5, until restart, with the row untouched.
		if( !v.is_number() || v.to_number<uint>() )
			Subscriptions::OnMutation( _mutation, v );
		base::Resume( move(v) );
	}
	α UpdateAwait::await_resume()ε->jvalue{
		if( _exception )
			_exception->Throw();
		return Promise()
			? TAwait<jvalue>::await_resume()
			: jvalue{};
	}
}