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
	constexpr ELogTags _tags{ ELogTags::QL };
	UpdateAwait::UpdateAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SL sl )ι:
		base{ sl },
		_mutation{ move(mutation) },
		_table{ table },
		_userPK{ userPK }
	{}

	α UpdateAwait::await_ready()ι->bool{
		try{
			if( _mutation.Type==EMutationQL::Update ){
				_table->Authorize( Access::ERights::Update, _userPK, _sl );
				CreateUpdate( *_table );
				THROW_IF( _updates.empty(), "There is nothing to update." );
			}
			else{
				CreateDeleteRestore( *_table );
				_table->Authorize( Access::ERights::Delete, _userPK, _sl );
			}
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		return _exception!=nullptr;
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
					optional<flat_map<string,uint>> values;
					[] (auto& values, auto& t)ι->DB::MapAwait<string,uint>::Task {
						values = co_await t.Schema->DS()-> template SelectMap<string,uint>( {Ƒ("select name, {} from {}", t.GetPK()->Name, t.DBName)} );
					}( values, qlColumn.Table() );
					while( !values )
						std::this_thread::yield();

					for( let& flagName : *flags ){
						if( let pFlag = values->find(Json::AsString(flagName)); pFlag != values->end() )
							value |= pFlag->second;
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
		update.Where.Add( deleted->Table->GetPK(), DB::Value{_mutation.Id()} );//deleted=main table, table=possibly extension table.
		_updates.push_back( move(update) );
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
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α UpdateAwait::UpdateAfter( uint rowCount )ι->MutationAwaits::Task{
		try{
			co_await Hook::UpdateAfter( _mutation, _userPK );
			Resume( jvalue{rowCount} );
		}
		catch( IException& e ){
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