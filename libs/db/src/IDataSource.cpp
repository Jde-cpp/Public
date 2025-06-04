#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

#define let const auto
namespace Jde::DB{
	α IDataSource::TrySelect( Sql&& sql, RowΛ f, SL sl )ι->bool{
		return Try( [&]{Select(move(sql), f, sl);} );
	}

	α IDataSource::TryExecute( Sql&& sql, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = Execute( move(sql), false, sl );
		}
		catch( const IException& )
		{}
		return result;
	}

	α IDataSource::CatalogName( SL sl )ε->string{
		if( !_catalog ){
			let sql = Syntax().CatalogSelect();
			_catalog = sql.size()
				? Scaler<string>( {string{sql}}, sl )
				: string{};
		}
		return *_catalog;
	}

	α IDataSource::SchemaName( SL sl )ε->string{
		if( _schema.empty() ){
			let schema = Scaler<string>( {string{Syntax().SchemaSelect()}}, sl ); THROW_IF( !schema, "Schema name is empty." );
			_schema = *schema;
		}
		return _schema;
	}
}