#include "Insert.h"
#include <jde/ql/GraphQL.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/db/Database.h>
#include "../GraphQuery.h"

#define var const auto

namespace Jde::QL{
	InsertAwait::InsertAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK_, uint extendedFromId, sp<DB::IDataSource> ds, SL sl )ι:
		AsyncReadyAwait{
			[&, id=extendedFromId](){ return CreateQuery( table, id ); },
			[&,userPK=userPK_](HCoroutine h){ Execute( move(h), userPK ); },
			sl,
			"InsertAwait"
		},
		_ds{ ds },
		_mutation{ mutation }
	{}

	α InsertAwait::CreateQuery( const DB::Table& table, uint extendedFromId )ι->optional<AwaitResult>{
		_sql << DB::Schema::ToSingular(table.Name) << "_insert(";
		_parameters.reserve( table.Columns.size() );
		var input = _mutation.Input();
		uint cNonDefaultArgs{};
		for( var& c : table.Columns ){
			if( !c.Insertable )
				continue;
			var [memberName, tableName] = ToJsonName( c, _ds->Schema() );

			if( _parameters.size() )
				_sql << ", ";
			_sql << "?";
			DB::object value;
			if( var pValue = input.find( memberName ); pValue!=input.end() ){// calling a stored proc, so need all variables.
				++cNonDefaultArgs;
				if( c.IsEnum && pValue->is_string() ){
					var pValues = SFuture<flat_map<uint,string>>( _ds->SelectEnum<uint>(tableName) ).get();
					optional<uint> pEnum = FindKey( *pValues, pValue->get<string>() ); THROW_IF( !pEnum, "Could not find '{}' for {}", pValue->get<string>(), memberName );
					value = *pEnum;
				}
				else
					value = ToObject( c.Type, *pValue, memberName );
			}
			else{
				if( extendedFromId && table.SurrogateKey().Name==c.Name )
					value = extendedFromId;
				else if( !c.IsNullable && c.Default.empty() && c.Insertable )
					THROW( "No value specified for {}.{}.", table.Name, memberName );
				else
					value = c.DefaultObject();
			}

			_parameters.push_back( value );
		}
		_sql << ")";
		return extendedFromId && !cNonDefaultArgs ? optional<AwaitResult>{ mu<uint>(extendedFromId) } : nullopt; // if no extended columns set, nothing todo.
	}
	α InsertAwait::Execute( HCoroutine h, UserPK userPK )ι->Task{
		var _tag{ ELogTags::QL | ELogTags::Pedantic };
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
			(co_await *_ds->ExecuteProcCo(_sql.str(), move(_parameters), [&](const DB::IRow& row){*pId = (int32)row.GetInt(0);}) ).CheckError();
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