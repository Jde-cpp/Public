#include <jde/db/awaits/ScalerAwait.h>
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde::DB{
	α ScalerAwait<uint32>::Execute()ι->ScalerAwaitOpt<uint32>::Task{
		try{
			auto opt = co_await ScalerAwaitOpt<uint32>{ move(_ds), move(_sql), base::_sl };
			if( opt )
				base::Resume( move(*opt) );
			else
				base::ResumeExp( Exception{"No value returned", ELogLevel::Error, base::_sl} );
		}
		catch( Exception& e ){
			base::ResumeExp( move(e) );
		}
	}
}
namespace Jde{
	α DB::ScalerAwaitExecute( IDataSource& ds, variant<Sql,InsertClause>&& sql, function<void(optional<Row>)> onRow, function<void(Exception&&)> onError, SL sl )ι->QueryAwait::Task{
		let isInsert = std::holds_alternative<InsertClause>( sql );
		auto query = isInsert ? get<InsertClause>( move(sql) ).Move() : get<Sql>( move(sql) );
		try{
			auto result = co_await ds.Query( move(query), isInsert, sl );
			onRow( result.Rows.size() ? move(result.Rows[0]) : optional<Row>{} );
		}
		catch( Exception& e ){
			onError( move(e) );
		}
	}
}