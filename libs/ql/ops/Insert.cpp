#include "Insert.h"
//#include "../GraphQL.h"
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
	InsertAwait::InsertAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK_, uint extendedFromId, SL sl )ι:
		AsyncReadyAwait{
			[&, id=extendedFromId](){ return CreateQuery( table, id ); },
			[&,userPK=userPK_](HCoroutine h){ Execute( move(h), userPK, table.Schema->DS() ); },
			sl,
			"InsertAwait"
		},
		_mutation{ mutation }
	{}

	α InsertAwait::CreateQuery( const DB::Table& table, uint extendedFromId )ι->optional<AwaitResult>{
		_sql << DB::Names::ToSingular(table.Name) << "_insert(";
		_parameters.reserve( table.Columns.size() );
		let pk = table.FindPK();
		let input = _mutation.Input();
		uint cNonDefaultArgs{};
		for( let& c : table.Columns ){
			if( !c->Insertable )
				continue;
			const QLColumn qlCol{ c };

			if( _parameters.size() )
				_sql << ", ";
			_sql << "?";
			DB::Value value;
			if( let jvalue = input.if_contains( qlCol.MemberName() ); jvalue ){// calling a stored proc, so need all columns.
				++cNonDefaultArgs;
				if( c->IsEnum() && jvalue->is_string() ){
					let pValues = SFuture<flat_map<uint,string>>( table.Schema->DS()->SelectEnum<uint>(qlCol.Table().DBName) ).get();
					optional<uint> pEnum = FindKey( *pValues, string{jvalue->get_string()} ); THROW_IF( !pEnum, "Could not find '{}' for {}", string{jvalue->get_string()}, qlCol.MemberName() );
					value = *pEnum;
				}
				else
					value = DB::Value{ c->Type, *jvalue };
			}
			else{
				if( extendedFromId && pk && pk->Name==c->Name )
					value = extendedFromId;
				else if( !c->Default && c->Insertable ) //insertable=not populated by stored proc
					THROW( "No value specified for {}.{}.", table.Name, qlCol.MemberName() );
				else
					value = *c->Default;
			}

			_parameters.push_back( value );
		}
		_sql << ")";
		return extendedFromId && !cNonDefaultArgs ? optional<AwaitResult>{ mu<uint>(extendedFromId) } : nullopt; // if no extended columns set, nothing todo.
	}
	α InsertAwait::Execute( HCoroutine h, UserPK userPK, sp<DB::IDataSource> ds )ι->Task{
		let _tag{ ELogTags::QL | ELogTags::Pedantic };
		Trace( _tag, "({})InsertAwait::Execute", h.address() );
		try{
				( co_await Hook::InsertBefore(_mutation, userPK) ).UP<uint>();
		}
		catch( IException& e ){
			Resume( move(e), move(h) );
			co_return;
		}
		up<IException> exception;
		try{
			auto pId = mu<uint>();
			(co_await *ds->ExecuteProcCo(_sql.str(), move(_parameters), [&](const DB::IRow& row){*pId = (int32)row.GetInt(0);}) ).CheckError();
			Resume( move(pId), move(h) );
		}
		catch( IException& e ){
			exception = e.Move();
		}
		if( exception ){
			h.promise().SetResult( move(*exception) );
			h.resume();
		}
	}
}