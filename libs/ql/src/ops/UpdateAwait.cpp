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
			if( !c->Updateable || !c->IsFlags() )
				continue;
			let jvalue = _mutation.Args.if_contains( QLColumn{c}.MemberName() );
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
			let& args = _mutation.Args;
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

		let& input = _mutation.Args;
		for( let& c : table.Columns ){
			if( !c->Updateable )
				continue;

			const QLColumn qlColumn{ c };
			let jvalue = input.if_contains( qlColumn.MemberName() );
			if( !jvalue )
				continue;
			if( !c->IsFlags() )
				update.Add( c, DB::Value{c->Type, *jvalue} );
			else{
				uint value = 0;
				if( let flags = jvalue->if_array(); flags && flags->size() ){
					let& values = EnumValues( qlColumn.Table() );
					for( let& flagName : *flags ){
						let name = Json::AsString( flagName );
						let flag = FindKey( values, name ); THROW_IF( !flag, "Could not find '{}' for {}", name, qlColumn.MemberName() );
						value |= *flag;
					}
				}
				else if( jvalue->is_number() )
					value = Json::AsNumber<uint>( *jvalue );
				update.Add( c, {value} );
			}
		}
		THROW_IF( update.Where.Empty(), "There is no where clause." );
		if( update.Values.size() )
			_updates.push_back( move(update) );
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
			if( _mutation.Type==EMutationQL::Update ){
				vector<sp<DB::View>> enumTables;
				FlagTables( *_table, enumTables );
				for( let& enumTable : enumTables ){
					if( !_enums.contains(enumTable->Name) )
						_enums.emplace( enumTable->Name, co_await enumTable->Schema->DS()->SelectEnum<uint,string>(*enumTable, _sl) );
				}
				CreateUpdate( *_table );
				THROW_IF( _updates.empty(), "There is nothing to update." );
			}
			else
				CreateDeleteRestore( *_table );
			UpdateBefore();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α UpdateAwait::UpdateBefore()ι->MutationAwaits::Task{
		try{
			co_await Hook::UpdateBefore( _mutation, _userPK );
			Execute();
		}
		catch( exception& e ){
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
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::UpdateAfter( uint rowCount )ι->MutationAwaits::Task{
		try{
			co_await Hook::UpdateAfter( _mutation, _userPK );
			Resume( jvalue{rowCount} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::Resume( jvalue&& v )ι->void{
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