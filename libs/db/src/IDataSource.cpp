#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <stdexcept>

#define let const auto
namespace Jde::DB{
	//C1: the sync wrappers every driver used to copy.  Each is a shape over the driver's Execute(Sql&&, SL, Params).
	α IDataSource::ExecuteSync( Sql&& sql, SL sl )ε->uint{
		return Execute( move(sql), sl, {} );
	}
	α IDataSource::ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value{
		Value y;
		RowΛ f = [&]( Row&& r )->void{
			THROW_IFSL( r.Size()==0, "Query did not return any {}.", outValue==EValue::Null ? "rows" : "out params" );
			y = move( r[0] );
		};
		Execute( move(sql), sl, {.Function=&f, .OutValue=outValue} );
		return y;
	}
	α IDataSource::Select( Sql&& sql, SL sl )ε->vector<Row>{
		vector<Row> rows;
		RowΛ f = [&rows]( Row&& r ){ rows.push_back( move(r) ); };
		Execute( move(sql), sl, {.Function=&f} );
		return rows;
	}
	α IDataSource::Select( Sql&& sql, RowΛ f, SL sl )ε->uint{
		return Execute( move(sql), sl, {.Function=&f} );
	}

	α IDataSource::AtCatalog( sv catalog, SL sl )ε->sp<IDataSource>{
		THROW_IFSL( Syntax().HasCatalogs(), "AtCatalog('{}') is not implemented by this driver.", catalog );
		LOGSL( ELogLevel::Critical, sl, ELogTags::Sql, "This dialect has no catalogs - AtCatalog('{}') answers the same data source.", catalog );
		return shared_from_this();
	}
	α IDataSource::AtSchema( sv schema, SL sl )ε->sp<IDataSource>{
		THROW_IFSL( Syntax().CanSetDefaultSchema(), "AtSchema('{}') is not implemented by this driver.", schema );
		LOGSL( ELogLevel::Critical, sl, ELogTags::Sql, "This dialect cannot switch its default schema - AtSchema('{}') answers the same data source.", schema );
		return shared_from_this();
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
			let& syntax = Syntax();
			if( let sql = syntax.SchemaSelect(); sql.size() ){
				let schema = ScalerSyncOpt<string>( {string{sql}}, sl ); THROW_IF( !schema, "Schema name is empty." );
				_schema = *schema;
			}
			else //schemaless dialect (sqlite) - answer with SysSchema so callers needn't branch on HasSchemas.
				_schema = syntax.SysSchema();
		}
		return _schema;
	}
}