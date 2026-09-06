#include "AddRemoveAwait.h"
#include <jde/fwk/co/AnyAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLHook.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	struct ChildParentParams final{ sp<DB::Column> ParentCol; sp<DB::Column> ChildColumn; DB::Value ParentParam; vector<DB::Value> ChildParams; };

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

	//#49: the column list has to grow with the placeholders.  It was fixed at the two mapped columns while `input:{…}` appended
	//a `?` per extra value, so any use of the feature emitted `insert into t(a,b)values(?,?,?)` - a column-count error, every
	//time.  Nothing in tree sends `input:` on an add, which is why a branch that could never work went unnoticed.
	Ω addSql( const DB::Table& table, const jobject& input, vector<DB::Value>& extraParams )ε->string{
		let& parentId = table.Map->Parent->Name; let& childId = table.Map->Child->Name;
		string extraColumns;
		string extraParamsString;
		if( auto defParams = Json::FindObject(input, "input"); defParams ){
			for( let& [name,value] : *defParams ){
				if( name=="id" || name==parentId || name==childId )
					continue;
				auto pColumn = table.GetColumnPtr( DB::Names::FromJson(name) );
				extraColumns += ","+pColumn->Name;
				extraParamsString += ",?";
				extraParams.emplace_back( DB::Value{pColumn->Type, value} );
			}
		}
		return Ƒ( "insert into {}({},{}{})values(?,?{})", table.SqlName(), parentId, childId, extraColumns, extraParamsString );
	}

	//One coroutine.  Seven used to share the work - AddBefore → Add → AddAfter, Remove → RemoveAfter, AddHook, RemoveHook - each a
	//paste of its neighbour (ql-review3 #52: RemoveAfter awaited Hook::AddAfter), because the hooks and the statements were
	//different task types.  The hooks are AnyAwaits and Any() wraps the statements, so one frame awaits both, and the params are a local again.
	α AddRemoveAwait::Execute()ι->TAwait<jvalue>::Task{
		jvalue y;
		try{
			_table->Authorize( Access::ERights::Update, _userPK, _sl );
			let isAdd = _mutation.Type==EMutationQL::Add;
			ASSERT( isAdd || _mutation.Type==EMutationQL::Remove );
			if( isAdd && _table->AddProc.size() ){//a proc-backed table's add/remove is the hook's alone.
				auto result = co_await Hook::Add( _mutation, _userPK );
				THROW_IF( !result, "Hook::Add returned null." );
				TRACE( "Hook::Add returned '{}'", serialize(*result) );
				y = move( *result );
			}
			else if( !isAdd && _table->RemoveProc.size() ){
				auto result = co_await Hook::Remove( _mutation, _userPK );
				THROW_IF( !result, "Hook::Remove returned null." );
				y = move( *result );
			}
			else{
				THROW_IF( !_table->Map, "'{}' does not support add/remove.", _table->Name );
				let& map = *_table->Map;
				let input = _mutation.ExtrapolateVariables();
				let params = getChildParentParams( map.Parent, map.Child, input );//ahead of the hooks - when both would refuse, this is the message that arrives (access GroupTests).
				auto& ds = *_table->Schema->DS();
				uint rowCount{};
				if( isAdd ){
					co_await Hook::AddBefore( _mutation, _userPK );
					vector<DB::Value> extraParams;
					let sql = addSql( *_table, input, extraParams );
					for( let& p : params.ChildParams ){
						vector<DB::Value> values{ params.ParentParam, p };
						values.insert( end(values), begin(extraParams), end(extraParams) );
						rowCount += co_await Any( ds.Execute({sql, move(values)}, _sl) );
					}
					co_await Hook::AddAfter( _mutation, _userPK );
				}
				else{//no RemoveBefore: nothing has ever hooked one.
					let sql = Ƒ( "delete from {} where {}=? and {}=?", _table->SqlName(), map.Parent->Name, map.Child->Name );
					for( let& p : params.ChildParams )
						rowCount += co_await Any( ds.Execute({sql, {params.ParentParam, p}}, _sl) );
					co_await Hook::RemoveAfter( _mutation, _userPK );
				}
				y = rowCount;
			}
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
			co_return;
		}
		Resume( move(y) );
	}
}