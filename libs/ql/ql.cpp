#include <jde/ql/ql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/MutationQL.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
  α Mutation( const MutationQL& m, UserPK userPK )ε->uint;
  α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
}
namespace Jde{
	α QL::Configure( vector<sp<DB::AppSchema>>&& schemas )ι->void{
		_schemas = move( schemas );
	}

	α QL::Query( sv query, UserPK userPK, SL sl )ε->jobject{
		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Query: {}", query };
		let qlType = Parse( query );
		vector<TableQL> tableQueries;
		jobject j;
		if( qlType.index()==1 ){
			let& mutation = get<MutationQL>( qlType );
			uint result = Mutation( mutation, userPK );
			sv resultMemberName = mutation.Type==EMutationQL::Create ? "id" : "rowCount";
			let wantResults = mutation.ResultPtr && mutation.ResultPtr->Columns.size()>0;
			if( wantResults && mutation.ResultPtr->Columns.front().JsonName==resultMemberName )
				j["data"].emplace_object()[mutation.JsonName].emplace_object()[resultMemberName] = result;
			else if( mutation.ResultPtr )
				tableQueries.push_back( *mutation.ResultPtr );
		}
		else
			tableQueries = get<vector<TableQL>>( qlType );
		jobject y = tableQueries.size() ? QueryTables( tableQueries, userPK ) : j;
		//Dbg( y.dump() );
		return y;
	}

	α QL::CoQuery( string q_, UserPK u_, SL sl )ι->TPoolAwait<jobject>{
		return Coroutine::TPoolAwait<jobject>( [q=move(q_), u=u_](){
			return mu<jobject>(Query(q,u));
		}, {}, sl );
	}
}

namespace Jde::QL{
	α FindTable( str tableName )ε->sp<DB::View>{
		for( let& schema : _schemas ){
			if( let pTable = schema->GetTablePtr(tableName); pTable )
				return DB::AsView( pTable );
		}
		THROW( "Could not find table '{}'", tableName );
	}
  α Schemas()ι->const vector<sp<DB::AppSchema>>&{
    return _schemas;
  }
}