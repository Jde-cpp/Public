#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

#define let const auto
namespace Jde::DB{
	α IDataSource::TrySelect( Sql&& sql, RowΛ f, SL sl )ι->bool{
		try{
			Select( move(sql), f, sl );
			return true;
		}
		catch( const Exception& ){
			return false;
		}
	}

	α IDataSource::TryExecuteSync( Sql&& sql, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = ExecuteSync( move(sql), sl );
		}
		catch( const Exception& )
		{}
		return result;
	}

	α IDataSource::CatalogName( SL sl )ε->string{
		if( !_catalog ){
			let sql = Syntax().CatalogSelect();
			_catalog = sql.size()
				? ScalerSync<string>( {string{sql}}, sl )
				: string{};
		}
		return *_catalog;
	}

	α IDataSource::SchemaName( SL sl )ε->string{
		if( _schema.empty() ){
			let schema = ScalerSyncOpt<string>( {string{Syntax().SchemaSelect()}}, sl ); THROW_IF( !schema, "Schema name is empty." );
			_schema = *schema;
		}
		return _schema;
	}
}