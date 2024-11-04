#include "InsertAwait.h"
#include <jde/ql/GraphQLHook.h>
#include <jde/db/Database.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/names.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include "../GraphQuery.h"
#include "../types/QLColumn.h"

#define let const auto

namespace Jde::QL{
	using DB::Value;
	constexpr ELogTags _tags{ ELogTags::QL };
	α getEnumValue( const DB::Column& c, const QLColumn& qlCol, const jvalue& v )->Value;
	α GetEnumValues( const DB::View& table, SRCE )ε->sp<flat_map<uint,string>>;

	InsertAwait::InsertAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK_, SL sl )ι:
		AsyncReadyAwait{
			[&](){ return CreateQuery( table ); },
			[&,userPK=userPK_](HCoroutine h){ Execute( move(h), userPK, table.Schema->DS() ); },
			sl,
			"InsertAwait"
		},
		_mutation{ mutation }
	{}

	α InsertAwait::CreateQuery( const DB::Table& table )ι->optional<AwaitResult>{
		try{
			CreateQuery( table, _mutation.Input() );
			return _statements.empty() ? optional<AwaitResult>{ mu<uint>(0) } : nullopt;
		}
		catch( IException& e ){
			return AwaitResult{ e.Move() };
		}
	}

	α InsertAwait::CreateQuery( const DB::Table& table, const jobject& input, bool nested )ε->void{
		for( let& [key,value] : input ){ //look for nested tables.
			if( auto nestedTable = value.is_object() ? table.Schema->FindTable(DB::Names::ToPlural(key)) : nullptr; nestedTable ){
				let& o = value.get_object();
				if( o.size()==1 && o.contains("id") && nestedTable->SurrogateKeys.size() )
					_nestedIds.emplace( nestedTable->SurrogateKeys[0]->Name, DB::Value{Json::AsNumber<uint>(o, "id")} );
				else
					CreateQuery( *nestedTable, o, true );
			}
		}
		if( table.Extends && !nested )
			AddStatement( *table.Extends, input, false, table.GetSK0()->Criteria );
		AddStatement( table, input, nested );
	}

	α InsertAwait::AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria )ε->void{
		uint cNonDefaultArgs{};
		string missingColumnsError{};
		DB::InsertStatement statement;
		vector<sp<DB::Column>> missingColumns;
		for( let& c : table.Columns ){
			if( !c->Insertable )
				continue;
			const QLColumn qlCol{ c };
			Value value;
			let memberName = qlCol.MemberName();
			if( let jvalue = input.if_contains(memberName); jvalue ){// calling a stored proc, so need all columns.
				++cNonDefaultArgs;
				value = c->IsEnum() && (jvalue->is_string() || jvalue->is_array())
					? getEnumValue( *c, qlCol, *jvalue )
					: Value{ c->Type, *jvalue };
			}
			else if( !c->Default && c->Insertable ){ //insertable=not populated by stored proc, may not be an extension record.
				THROW_IF( !c->PKTable, "No default for {} in {}", c->Name, table.Name );
				++cNonDefaultArgs;
				missingColumns.emplace_back( c );//also needs to be inserted, insert null for now.
			}
			else if( c->Name==criteria )
				value = Value{ true };
			else
				value = *c->Default;
			statement.Add( c, value );
		}
		if( (cNonDefaultArgs || missingColumns.size()) && cNonDefaultArgs!=missingColumns.size() ){//don't want to insert just identity_id in users table.
			_statements.emplace_back( cNonDefaultArgs>0 ? move(statement) : DB::InsertStatement{} );
			_missingColumns.emplace_back( move(missingColumns) );
		}
	}

	α InsertAwait::Execute( HCoroutine h, UserPK userPK, sp<DB::IDataSource> ds )ι->Task{
		try{
				( co_await Hook::InsertBefore(_mutation, userPK) ).UP<uint>();
		}
		catch( IException& e ){
			Resume( move(e), move(h) );
			co_return;
		}
		up<IException> exception;
		uint mainId{};
		try{
			for( uint i=0; i<_statements.size(); ++i ){
				auto& statement = _statements[i];
				for( auto&& missingCol : _missingColumns[i] ){
					if( auto missingValue = _nestedIds.find(missingCol->Name); missingValue!=_nestedIds.end() )
						statement.SetValue( missingCol, move(missingValue->second) );
				}

				uint id;
				auto sql = statement.Move();
				if( statement.IsStoredProc )
					(co_await *ds->ExecuteProcCo(sql.Text, move(sql.Params), [&](const DB::IRow& row){id = (int32)row.GetInt(0);}) ).CheckError();
				else
					co_await *ds->ExecuteCo( sql.Text, move(sql.Params) );


				auto table = statement.Values.size() ? statement.Values.begin()->first->Table : nullptr;
				if( auto sequence = statement.Values.size() && table->SurrogateKeys.size() ? table->SurrogateKeys[0] : nullptr; sequence )
					_nestedIds.emplace( sequence->Name, id );
				if( !mainId )
					mainId = id;
			}
			Resume( mu<uint>(mainId), h );
		}
		catch( IException& e ){
			exception = e.Move();
		}
		if( exception ){
			( co_await Hook::InsertFailure(_mutation, userPK) ).UP<uint>();
			Resume( move(*exception), h );
		}
	}
	α getEnumValue( const DB::Column& c, const QLColumn& qlCol, const jvalue& v )->Value{
		Value y;
		let values = GetEnumValues( qlCol.Table() );
		if( v.is_string() ){
			let enum_ = FindKey( *values, string{v.get_string()} ); THROW_IF( !enum_, "Could not find '{}' for {}", string{v.get_string()}, qlCol.MemberName() );
			y = *enum_;
		}
		else{
			let& jflags = v.get_array();
			uint flags{};
			for( let& jflag : jflags ){
				if( !jflag.is_string() )
					continue;
				let flag = FindKey( *values, string{jflag.get_string()} ); THROW_IF( !flag, "Could not find '{}' for {}", string{jflag.get_string()}, qlCol.MemberName() );
				flags |= *flag;
			}
			y = Value{ c.Type, flags };
		}
		return y;
	}
}