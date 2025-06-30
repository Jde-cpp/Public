#include <jde/db/awaits/ScalerAwait.h>
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde{
	α DB::ScalerAwaitExecute( IDataSource& ds, variant<Sql,InsertClause>&& _sql, function<void(optional<Row>)> onRow, function<void(IException&&)> onError, SL sl )ι->QueryAwait::Task{
		let outParams = _sql.index()==1;
		if( outParams )
			get<InsertClause>(_sql).Add( (uint)0 );
		auto sql = outParams ? get<InsertClause>( move(_sql) ).Move() : get<Sql>( move(_sql) );
		try{
			auto result = co_await ds.Query( move(sql), outParams, sl );
			onRow( result.Rows.size() ? move(result.Rows[0]) : optional<Row>{} );
		}
		catch( IException& e ){
			onError( move(e) );
		}
	}
}