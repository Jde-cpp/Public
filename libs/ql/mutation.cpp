#include "jde/ql/types/MutationQL.h"
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/GraphQLHook.h>
#include "ops/AddRemoveAwait.h"
#include "ops/InsertAwait.h"
#include "ops/PurgeAwait.h"
#include "types/QLColumn.h"

#define let const auto
namespace Jde::QL{
	using namespace Coroutine;
	using namespace DB::Names;

	constexpr ELogTags _tags{ ELogTags::QL };
	α GetTable( str tableName )ε->sp<DB::View>;


	α Update( const DB::View& table, const MutationQL& m )->tuple<uint,DB::Value>{
		let pExtendedFromTable = table.IsView() ? nullptr : AsTable(table).Extends;
		auto [count,rowKey] = pExtendedFromTable ? Update(*pExtendedFromTable, m) : make_tuple( 0ul, DB::Value{0} );

		DB::UpdateStatement update;
		if( pExtendedFromTable )
			update.Where.Add( table.SurrogateKeys[0], rowKey );
		else{
			if( let id = table.FindPK() ? Json::FindNumber<uint>(m.Args, "id") : optional<uint>{}; id )
				update.Where.Add( table.FindPK(), DB::Value{*id} );
			else if( let name = table.FindColumn("name") ? Json::FindSV(m.Args, "name") : optional<sv>{}; name )
				update.Where.Add( table.FindColumn("name"), DB::Value{string{*name}} );
			else if( let target = table.FindColumn("target") ? Json::FindSV(m.Args, "target") : optional<sv>{}; target )
				update.Where.Add( table.FindColumn("target"), DB::Value{string{*target}} );
			else
				THROW( "Could not get criteria from {}", serialize(m.Args) );
			rowKey = update.Where.Params()[0];
		}

		let& input = m.Input();
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
		auto sql = update.Move();
		uint result = sql.Text.size() ? table.Schema->DS()->Execute( move(sql.Text), sql.Params ) : 0; //main/extended table may not have update.
		return make_tuple( count+result, rowKey );
	}
	α SetDeleted( const DB::Table& table, uint id, DB::Value value )ε->uint{
		DB::UpdateStatement update;
		auto deleted = table.GetColumnPtr( "deleted" );
		update.Add( deleted, value );
		update.Where.Add( deleted->Table->GetPK(), DB::Value{id} );//deleted=main table, table=possibly extension table.
		let sql = update.Move();
		return table.Schema->DS()->Execute( move(sql.Text), move(sql.Params) );
	}
	α Delete( const DB::Table& table, const MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), DB::Value{"$now"} );
	}
	α Restore( const DB::Table& table, const MutationQL& m )ε->uint{
		return SetDeleted( table, m.Id(), {} );
	}

	α Start( MutationQL m, UserPK userPK )ε->optional<jvalue>{
		optional<jvalue> result;
		bool set{};
		[&]()ι->MutationAwaits::Task {
			auto await_ = Hook::Start( move(m), userPK );
			result = co_await await_;
			set = true;
		}();
		while( !set )
			std::this_thread::yield();//TODO remove this when async.
		return result;
	}

	α Stop( MutationQL m, UserPK userPK )ε->optional<jvalue>{
		optional<jvalue> result;
		bool set{};
		[&]()ι->MutationAwaits::Task {
			result = co_await Hook::Stop( move(m), userPK );
			set = true;
		}();
	while( !set )
			std::this_thread::yield();//TODO remove this when async.
		return result;
	}

//#pragma warning( disable : 4701 )
	α Mutation( const MutationQL& m, UserPK userPK )ε->optional<jvalue>{
		let& table = DB::AsTable( GetTable(m.TableName()) );
		// if( let authorizer = table.Schema->Authorizer; authorizer )
		// 	authorizer->Test( m.Type, userPK );
		optional<jvalue> y;
		switch( m.Type ){
		using enum EMutationQL;
		case Create:{
			[]( auto& table, auto& m, auto userPK, auto& result )ι->TAwait<jvalue>::Task {
				result = co_await InsertAwait( table, m, userPK );
			}( table, m, userPK, y );
			while( !y )
				std::this_thread::yield();
		break;}
		case Update:
			y = get<0>( QL::Update(*table, m) );
			break;
		case Delete:
			y = QL::Delete( *table, m);
			break;
		case Restore:
			y = QL::Restore( *table, m );
			break;
		case Purge:{
			bool set{};
			[] (auto& table, auto& m, auto userPK, auto& result, auto& set )ι->TAwait<jvalue>::Task {
				result = co_await PurgeAwait{ table, m, userPK };
				set = true;
			}( table, m, userPK, y, set );
			while( !set )
				std::this_thread::yield();
			break;}
		case Add:
		case Remove:{
			bool set{};
			[] (auto& table, auto& m, auto userPK, auto& result, bool& set )ι->AddRemoveAwait::Task {
				result = co_await AddRemoveAwait{ table, m, userPK };
				set = true;
			}( table, m, userPK, y, set );
			while( !set )
				std::this_thread::yield();
			break;}
		case Start:
			y = QL::Start( move(m), userPK );
			break;
		case Stop:
			y = QL::Stop( move(m), userPK );
			break;
		}
		return y;
	}
}