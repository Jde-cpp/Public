#include "InsertAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/names.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/LocalSubscriptions.h>
#include "../types/QLColumn.h"

#define let const auto

namespace Jde::QL{
	using DB::Value;
	constexpr ELogTags _tags{ ELogTags::QL };
	Ω getEnumValue( const DB::Column& c, const QLColumn& qlCol, const jvalue& v )->Value;
	α GetEnumValues( const DB::View& table, SRCE )ε->flat_map<uint,string>;
	α ToFlags( const flat_map<uint,string>& values, const jarray& flags, sv memberName, SRCE )ε->uint;//ops/SelectAwait.cpp - shared with UpdateAwait (#48).

	InsertAwait::InsertAwait( sp<DB::Table> table, MutationQL m, UserPK executer, SL sl )ι:
		InsertAwait( table, move(m), false, executer, sl )
	{}
	InsertAwait::InsertAwait( sp<DB::Table> table, MutationQL&& m, bool identityInsert, UserPK executer, SL sl )ι:
		base{sl},
		_executer{ executer },
		_identityInsert{ identityInsert },
		_mutation{ move(m) },
		_table{ table }
	{}

	α InsertAwait::await_ready()ι->bool{
		try{
			_table->Authorize( Access::ERights::Create, _executer, _sl );
			CreateQuery( *_table, _mutation.ExtrapolateVariables() );
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		return _exception || ( _statements.empty() && !_table->HasCustomInsertProc );
	}

	α InsertAwait::CreateQuery( const DB::Table& table, jobject input, bool nested )ε->void{
		for( let& [key,value] : input ){ //look for nested tables.
			if( auto nestedTable = value.is_object() ? table.Schema->FindTable(DB::Names::FromJson(DB::Names::ToPlural(key))) : nullptr; nestedTable ){
				let& o = value.get_object();
				if( o.size()==1 && o.contains("id") && nestedTable->SurrogateKeys.size() ){
					nestedTable->Authorize( Access::ERights::Read, _executer, _sl );
					//#24: the same rule as the pk branch in AddStatement - `identity:{id:N}` on a table that *extends* identities
					//would pre-seed the back-fill and bind the new row to N, so the id this insert is about to create is dropped.
					let extends = table.IsView() ? nullptr : AsTable(table).Extends;
					if( extends && extends->Name==nestedTable->Name ){
						WARNT( ELogTags::QL, "[{}]ignoring the supplied '{}' id - an extension row takes its pk from the row it extends.", table.Name, string{key} );
					}
					else
						_nestedIds.emplace( nestedTable->SurrogateKeys[0]->Name, DB::Value{Json::AsNumber<uint>(o, "id")} );
				}
				else{
					nestedTable->Authorize( Access::ERights::Create, _executer, _sl );
					CreateQuery( *nestedTable, o, true );
				}
			}
		}
		if( table.Extends && !nested )
			AddStatement( *table.Extends, input, table.GetSK0()->Criteria );
		AddStatement( table, input );
	}

	α InsertAwait::AddStatement( const DB::Table& table, const jobject& input, optional<DB::Criteria> criteria )ε->void{
		uint cNonDefaultArgs{};
		string missingColumnsError{};
		DB::InsertClause statement;
		vector<sp<DB::Column>> missingColumns;
		for( let& c : table.Columns ){
			if( !c->Insertable && (!_identityInsert || !c->IsPK()) )
				continue;
			const QLColumn qlCol{ c };
			Value value;
			let memberName = qlCol.MemberName();
			//#24: an extension's pk is the parent row's id - it comes from the insert a few lines up, never from the client.
			//Honouring `identityId:N` (or `id:N`) bound the new users row to an existing identity and left the identities row
			//this mutation just created an orphan;  ignoring it puts the column in _missingColumns, where Execute fills it in.
			let isExtensionKey = !_identityInsert && c->IsPK() && !table.IsView() && AsTable(table).Extends;
			if( isExtensionKey && (input.if_contains(memberName) || input.if_contains("id")) )
				WARNT( ELogTags::QL, "[{}.{}]ignoring the supplied key - an extension row takes its pk from the row it extends.", table.Name, c->Name );
			if( let jvalue = isExtensionKey ? nullptr : input.if_contains(memberName); jvalue ){// calling a stored proc, so need all columns.
				++cNonDefaultArgs;
				value = c->IsEnum() && (jvalue->is_string() || jvalue->is_array())
					? getEnumValue( *c, qlCol, *jvalue )
					: Value{ c->Type, *jvalue };
			}
			//#24: still ungated beyond the extension check above - the schema seed inserts its enum rows through this branch
			//(`createRights( name:"None", id:0 )`, which LocalQL::Upsert routes here *without* _identityInsert), so gating it on
			//_identityInsert as the fix note suggests fails startup.  The extension case, which is the one that binds a new row
			//to someone else's identity, is handled by isExtensionKey.
			else if( let id = !isExtensionKey && c->IsPK() ? input.if_contains("id") : nullptr; id )
				value = Value{ c->Type, *id };
			else if( !c->Default && c->Insertable ){ //insertable=not populated by stored proc, may [not] be an extension record.
				THROW_IF( !c->PKTable, "No default for '{}' in '{}'. mutation='{}'", c->Name, table.Name, _mutation.ToString() );
				++cNonDefaultArgs;
				missingColumns.emplace_back( c );//also needs to be inserted, insert null for now.
			}
			else if( criteria && c->Name==criteria->Column->Name )
				value = criteria->Value;
			else
				value = *c->Default;
			statement.Add( c, value.Variant );
		}
		//`missingColumns` only ever grows next to a `++cNonDefaultArgs`, so its size can never exceed the count.  The guard is
		//therefore true only when cNonDefaultArgs>missingColumns.size()>=0, i.e. cNonDefaultArgs>0 - which is why the old
		//`(cNonDefaultArgs || missingColumns.size())` disjunct and the empty-InsertClause arm below could never do anything.
		if( cNonDefaultArgs!=missingColumns.size() ){//don't want to insert just identity_id in users table.
			if( /*table.SequenceColumn() &&*/ !_identityInsert ) //role does not have a seq column, but has a return param.
				statement.Add( table.SequenceColumn(), (uint)0ul );
			_statements.emplace_back( move(statement) );
			_missingColumns.emplace_back( move(missingColumns) );
		}
	}

	α InsertAwait::InsertBefore()ι->MutationAwaits::Task{
		try{
			optional<jarray> result = co_await Hook::InsertBefore( _mutation, _executer );
			auto result0 = result ? result->if_contains(0) : nullptr;
			if( result0 && result0->is_object() && Json::FindDefaultBool(result0->get_object(), "complete") ){
				result0->get_object().erase( "complete" );
				Resume( jarray{ move(*result0) } );
				co_return;
			}
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
			co_return;
		}
		Execute();
	}

	α InsertAwait::Execute()ι->DB::QueryAwait::Task{
		jarray y;
		auto& ds = *_table->Schema->DS();
		try{
			for( uint i=0; i<_statements.size(); ++i ){
				auto& statement = _statements[i];
				for( auto&& missingCol : _missingColumns[i] ){
					if( auto missingValue = _nestedIds.find(missingCol->Name); missingValue!=_nestedIds.end() )
						statement.SetValue( missingCol, move(missingValue->second) );
				}

				uint id{};
				if( _identityInsert )
					statement.IsStoredProc = false;
				auto sql = statement.Move();
				if( statement.IsStoredProc ){
					let result = co_await ds.Query( move(sql), true, _sl );
					for( let& row : result.Rows ){
						ASSERT( row.Size() );
						id = row.Size() ? row.GetInt32( 0 ) : 0;
					}
					y.push_back( jobject{ {"id", id}, {"rowCount",result.RowsAffected} } );
				}else{
					if( _identityInsert && ds.Syntax().NeedsIdentityInsert() )
						sql.Text = Ƒ("SET IDENTITY_INSERT {0} ON;{1};SET IDENTITY_INSERT {0} OFF;", _table->SqlName(), sql.Text );
					let rowCount = ( co_await ds.Query(move(sql), false, _sl) ).RowsAffected;
					y.push_back( jobject{ {"rowCount",rowCount} } );
				}

				auto table = statement.Values.size() ? statement.Values.begin()->first->Table : nullptr;
				if( auto sequence = statement.Values.size() && table->SurrogateKeys.size() ? table->SurrogateKeys[0] : nullptr; sequence )
					_nestedIds.emplace( sequence->Name, id );
			}
			TRACE( "InsertAwait::Execute: {}", serialize(y) );
			InsertAfter( move(y) );
		}
		catch( runtime_error& e ){
			InsertFailure( ToExceptionPtr(move(e)) );
		}
	}
	α InsertAwait::InsertAfter( jarray result )ι->MutationAwaits::Task{
		try{
			let id = result.size() ? Json::FindNumber<uint>(result[0], "id").value_or(0) : 0;
			co_await Hook::InsertAfter( id, _mutation, _executer );
			Resume( move(result) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	//#28: by value, because the coroutine outlives the catch block that built it - but as up<Exception>, so the dynamic type
	//survives.  ResumeExp( Exception&& ) hands the referent to SetExp, which calls the virtual Move().
	α InsertAwait::InsertFailure( up<runtime_error> e )ι->MutationAwaits::Task{
		try{
			co_await Hook::InsertFailure( _mutation, _executer );
			ResumeExp( move(*e) );
		}
		catch( runtime_error& e2 ){
			ResumeExp( move(e2) );
		}
	}
	α InsertAwait::Resume( jarray&& v )ι->void{
		//#47: the insert half - no row means no statement inserted one, and publishing `null` told subscribers a row they could
		//not identify had appeared.
		if( v.size() )
			Subscriptions::OnMutation( _mutation, v[0] );
		base::Resume( move(v) );
	}

	α InsertAwait::await_resume()ε->jvalue{
		if( _exception )
			_exception->Throw();
		return Promise()
			? TAwait<jvalue>::await_resume()
			: jvalue{};
	}

	α getEnumValue( const DB::Column& c, const QLColumn& qlCol, const jvalue& v )->Value{
		Value y;
		let values = GetEnumValues( qlCol.Table() );
		if( v.is_string() ){
			let enum_ = FindKey( values, string{v.get_string()} ); THROW_IF( !enum_, "Could not find '{}' for {}", string{v.get_string()}, qlCol.MemberName() );
			y = *enum_;
		}
		else
			y = Value{ c.Type, ToFlags(values, v.get_array(), qlCol.MemberName()) };
		return y;
	}
}