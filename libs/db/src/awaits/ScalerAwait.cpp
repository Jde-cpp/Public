#include <jde/db/awaits/ScalerAwait.h>
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde{
	α DB::ScalerAwaitExecute( IDataSource& ds, variant<Sql,InsertClause>&& sql, function<void(optional<Row>)> onRow, function<void(IException&&)> onError, SL sl )ι->QueryAwait::Task{
		let isInsert = std::holds_alternative<InsertClause>( sql );
		auto query = isInsert ? get<InsertClause>( move(sql) ).Move() : get<Sql>( move(sql) );
		try{
			auto result = co_await ds.Query( move(query), isInsert, sl );
			onRow( result.Rows.size() ? move(result.Rows[0]) : optional<Row>{} );
		}
		catch( IException& e ){
			onError( move(e) );
		}
	}
}