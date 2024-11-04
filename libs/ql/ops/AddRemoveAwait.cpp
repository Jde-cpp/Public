#include "AddRemoveAwait.h"
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
	constexpr ELogTags _tags{ ELogTags::QL };

	AddRemoveAwait::AddRemoveAwait( sp<DB::Table> table, const MutationQL& mutation, UserPK userPK, SL sl )ι:
		TAwait{ sl },
		_mutation{ mutation },
		_table{ table },
		_userPK{ userPK }
	{}

	α GetChildParentParams( sp<DB::Column> parentCol, sp<DB::Column> childCol, const jobject& input )ε->ChildParentParams{
		Trace{ _tags, "GetChildParentParams '{}'", serialize(input) };
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

	α AddRemoveAwait::await_ready()ι->bool{
		try{
			THROW_IF( !_table->Map, "'{}' does not support add.", _table->Name );
			_params = GetChildParentParams( _table->Map->Parent, _table->Map->Child, _mutation.Args );
			return false;
		}
		catch( IException& e ){
			_exception = e.Move();
			return true;
		}
	}
	α AddRemoveAwait::Add()ι->Coroutine::Task{
		let& map = *_table->Map;
		let& parentId = map.Parent->Name; let& childId =map.Child->Name;
		let prefix = Ƒ( "insert into {}({},{})values(?,?", _table->Name, parentId, childId );
		string extraParamsString;
		vector<DB::Value> extraParams;
		try{
			if( auto defParams = Json::FindObject(_mutation.Args, "input"); defParams ){
				for( let& [name,value] : *defParams ){
					if( name=="id" || name==parentId || name==childId )
						continue;
					auto pColumn = _table->GetColumnPtr( DB::Names::FromJson(name) );
					extraParamsString += ",?";
					extraParams.emplace_back( DB::Value{pColumn->Type, value} );
				}
			}
			let sql = prefix + extraParamsString + ")";
			uint result{};
			for( let& p : _params.ChildParams ){
				vector<DB::Value> params{ _params.ParentParam, p };
				params.insert( end(params), begin(extraParams), end(extraParams) );
				auto a = _table->Schema->DS()->ExecuteCo( sql, params, _sl );
				result += *( co_await *a ).UP<uint>();
			}
			ResumeScaler( result );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AddRemoveAwait::Remove()->Coroutine::Task{
		let& map = *_table->Map;
		let sql = Ƒ( "delete from {} where {}=? and {}=?", _table->Name, map.Parent->Name, map.Child->Name );
		uint result{};
		try{
			for( let& p : _params.ChildParams ){
				vector<DB::Value> params{ _params.ParentParam, p };
				auto a = _table->Schema->DS()->ExecuteCo( sql, params, _sl );
				result += *( co_await *a ).UP<uint>();
			}
			ResumeScaler( result );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AddRemoveAwait::Suspend()ι->void{
		if( _mutation.Type==EMutationQL::Add )
			Add();
		else if( _mutation.Type==EMutationQL::Remove )
			Remove();
		else
			ASSERT( false );
	}

	α AddRemoveAwait::await_resume()ε->uint{
		if( _exception )
			_exception->Throw();
		return TAwait<uint>::await_resume();
	}
}