#include "jde/ql/types/MutationQL.h"
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/GraphQLHook.h>
#include "ops/Insert.h"
#include "ops/Purge.h"
#include "types/QLColumn.h"

#define let const auto
namespace Jde::QL{
	using namespace Coroutine;
	using namespace DB::Names;

	constexpr ELogTags _tags{ ELogTags::QL };
	α FindTable( str tableName )ε->sp<DB::View>;

	α ChildParentParams( sv childId, sv parentId, const jobject& input )->vector<DB::Value>{
		vector<DB::Value> parameters;
		if( let p = Json::FindNumber<uint>(input,childId); p )
			parameters.emplace_back( *p );
		else if( let p = Json::FindString(input, "target"); p )
			parameters.emplace_back( *p );
		else
			THROW( "Could not find '{}' or target in '{}'", childId, serialize(input) );

		if( let p = Json::FindNumber<uint>(input, parentId); p )
			parameters.emplace_back( *p );
		else if( let p = Json::FindString(input, "name"); p )
			parameters.emplace_back( *p );
		else
			THROW( "Could not find '{}' or name in '{}'", parentId, serialize(input) );

		return parameters;
	};

	α Add( const DB::Table& table, const jobject& input )->uint{
		string childColName = table.ChildColumn()->Name;
		string parentColName = table.ParentColumn()->Name;
		std::ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << childColName << "," << parentColName;
		let childId = ToJson( childColName );
		let parentId = ToJson( parentColName );
		auto parameters = ChildParentParams( childId, parentId, input );
		std::ostringstream values{ "?,?", std::ios::ate };
		for( let& [name,value] : input ){
			if( name==childId || name==parentId )
				continue;
			let columnName = ToJson( name );
			auto pColumn = table.GetColumnPtr( columnName );
			sql << "," << columnName;
			values << ",?";

			parameters.emplace_back( pColumn->Type, value );
		}
		sql << ")values( " << values.str() << ")";
		return table.Schema->DS()->Execute( sql.str(), parameters );
	}
	α Remove( const DB::Table& table, const jobject& input )->uint{
		let& childId = table.ChildColumn()->Name;
		let& parentId = table.ParentColumn()->Name;
		auto params = ChildParentParams( ToJson(childId), ToJson(parentId), input );
		std::ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << childId;
		/*if( params[0].Type()==EValue::String ){
			let pTable = table.ChildTable(); CHECK( pTable );
			let pTarget = pTable->GetColumnPtr( "target" );
			sql << SubWhere( *pTable, *pTarget, params, 0 );
		}
		else*/
			sql << "=?";
		sql << " and " << parentId;
/*		if( (EValue)params[1].index()==EEValue:String ){
			let pTable = table.ParentTable(); CHECK( pTable );
			let pName = pTable->FindColumn( "name" ); CHECK( pName );
			sql << SubWhere( *pTable, *pName, params, 1 );
			sql << "=( select id from " << pTable->Name << " where name";
			if( pName->QLAppend.size() )
			{

				let split = Str::Split( get<string>(params[1]), "\\" ); CHECK( split.size() );
				let appendColumnName = DB::Schema::FromJson( pName->QLAppend );
				let pColumn = pTable->FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
				sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
				if( split.size()>1 )
				{
					params.push_back( split[1] );
					params[1] = split[0];
				}
			}
			else
				sql << "=? )";
		}
		else*/
			sql << "=?";

		return table.Schema->DS()->Execute( sql.str(), params );
	}

	α Update( const DB::Table& table, const MutationQL& m )->tuple<uint,DB::Value>{
		let pExtendedFromTable = table.GetExtendedFromTable();
		auto [count,rowKey] = pExtendedFromTable ? Update(*pExtendedFromTable, m) : make_tuple( 0ul, DB::Value{0} );

		DB::UpdateStatement update;
		if( pExtendedFromTable )
			update.Where.Add( table.GetPK(), rowKey );
		else{
			if( let id = table.FindPK() ? Json::FindNumber<uint>(m.Args, table.GetPK()->Name) : optional<uint>{}; id )
				update.Where.Add( table.FindPK(), DB::Value{*id} );
			else if( let name = table.FindColumn("name") ? Json::FindSV(m.Args, "name") : optional<sv>{}; name )
				update.Where.Add( table.FindColumn("name"), DB::Value{string{*name}} );
			else if( let target = table.FindColumn("target") ? Json::FindSV(m.Args, "target") : optional<sv>{}; target )
				update.Where.Add( table.FindColumn("target"), DB::Value{string{*target}} );
			else
				THROW( "Could not get criterial from {}", serialize(m.Args) );
			rowKey = update.Where.Params()[0];
		}

		let& input = m.Input();
		for( let& c : table.Columns ){
			if( !c->Updateable ){
				if( c->Name=="updated" )
					update.Add(c, {ToStr(c->Table->Syntax().UtcNow())} );
				continue;
			}
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
					[] (auto& values, auto& t)ι->Coroutine::Task {
						AwaitResult result = co_await t.Schema->DS()-> template SelectMap<string,uint>( Ƒ("select name, {} from {}", t.GetPK()->Name, t.DBName) );
						values = *( result.UP<flat_map<string,uint>>() );
					}( values, qlColumn.Table() );
					while( !values )
						std::this_thread::yield();

					for( let& flagName : *flags ){
						if( let pFlag = values->find(Json::AsString(flagName)); pFlag != values->end() )
							value |= pFlag->second;
					}
				}
				update.Add( c, {value} );
			}
		}
		THROW_IF( update.Where.Empty(), "There is no where clause." );
		THROW_IF( update.Values.size()==0 && count==0, "There is nothing to update." );
		let sql = update.Move();
		uint result = table.Schema->DS()->Execute( sql.Text, sql.Params );
		return make_tuple( count+result, rowKey );
	}
	α SetDeleted( const DB::Table& table, uint id, iv time )ε->uint{
		DB::UpdateStatement update;
		update.Add( table.GetColumnPtr("deleted"), {ToStr(time)} );
		update.Where.Add( table.FindPK(), DB::Value{id} );
		let sql = update.Move();
		return table.Schema->DS()->Execute( move(sql.Text), move(sql.Params) );
	}
	α Delete( const DB::Table& table, const MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), table.Syntax().UtcNow() );
	}
	α Restore( const DB::Table& table, const MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), "null" );
	}


	α Start( sp<MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->MutationAwaits::Task {
			auto await_ = Hook::Start( m, userPK );
			result = co_await await_;
		}();
		while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}

	α Stop( sp<MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->MutationAwaits::Task {
			result = co_await Hook::Stop( m, userPK );
		}();
	while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}

//#pragma warning( disable : 4701 )
	α Mutation( const MutationQL& m, UserPK userPK )ε->uint{
		let& table = DB::AsTable( FindTable(m.TableName()) );
		// if( let authorizer = table.Schema->Authorizer; authorizer )
		// 	authorizer->Test( m.Type, userPK );
		optional<uint> result;
		switch( m.Type ){
		//using namespace DB;
		using enum EMutationQL;
		case Create:{
			optional<uint> extendedFromId;
			if( let pExtendedFrom = table->GetExtendedFromTable(); pExtendedFrom ){
				[](auto& extendedFromId, let& pExtendedFrom, let& m, auto userPK)ι->Task {
					AwaitResult result = co_await InsertAwait( *pExtendedFrom, m, userPK, 0 );
					extendedFromId = *( result.UP<uint>() );
				}(extendedFromId, pExtendedFrom, m, userPK);
				while (!extendedFromId)
					std::this_thread::yield();
			}
			auto _tag = _tags | ELogTags::Pedantic;
			Trace{ _tag, "calling InsertAwait" };
			[] (auto& table, auto& m, auto userPK, auto extendedFromId, auto& result)ι->Task {
				//result = *( co_await InsertAwait(table, m, userPK, extendedFromId.value_or(0)) ).UP<uint>();
				AwaitResult awaitResult = co_await InsertAwait( *table, m, userPK, extendedFromId.value_or(0) );
				result = *( awaitResult.UP<uint>() );
			}(table, m, userPK, extendedFromId, result);
			while (!result)
				std::this_thread::yield();
			Trace{ _tag, "~calling InsertAwait" };
		break;}
		case Update:
			result = get<0>( QL::Update(*table, m) );
			break;
		case Delete:
			result = QL::Delete( *table, m);
			break;
		case Restore:
			result = QL::Restore( *table, m );
			break;
		case Purge:
			[] (auto& table, auto& m, auto userPK, auto& result )ι->Task {
				//result = *( co_await PurgeAwait{table, m, userPK} ).UP<uint>();
				AwaitResult awaitResult = co_await PurgeAwait{ *table, m, userPK };
				result = *( awaitResult.UP<uint>() );
			}( table, m, userPK, result );
			while (!result)
				std::this_thread::yield();
			break;
		case Add:
			result = QL::Add( *table, m.Input() );
			break;
		case Remove:
			result = QL::Remove( *table, m.Input() );
			break;
		case Start:
			QL::Start( ms<MutationQL>(m), userPK );
			result = 1;
			break;
		case Stop:
			QL::Stop( ms<MutationQL>(m), userPK );
			result = 1;
			break;
		}
		return *result;
	}
}