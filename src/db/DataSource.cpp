﻿#include <jde/db/DataSource.h>
#include "metadata/ddl/Syntax.h"

#define let const auto
namespace Jde::DB{
	α IDataSource::Select( string sql, RowΛ f, SL sl )ε->void{
		Select( move(sql), f, nullptr, sl );
	}
	α IDataSource::Select( string sql, RowΛ f, const vector<object>& values, SL sl )ε->void{
		Select( move(sql), f, &values, sl );
	}
	α IDataSource::TrySelect( string sql, RowΛ f, SL sl )ι->bool{
		return Try( [&]{Select( move(sql), f, sl);} );
	}

	α IDataSource::TryExecute( string sql, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = Execute( move(sql), sl );
		}
		catch( const IException&  ){}

		return result;
	}
	α IDataSource::TryExecute( string sql, const vector<object>& parameters, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = Execute( move(sql), parameters, sl );
		}
		catch( const IException&  ){}

		return result;
	}

	α IDataSource::TryExecuteProc( string sql, const vector<object>& parameters, SL sl )ι->optional<uint>{
		optional<uint> result;
		try{
			result = ExecuteProc( move(sql), parameters, sl );
		}
		catch( const IException& ){}

		return result;
	}

	α IDataSource::CatalogName( SL sl )ε->string{
		if( !_catalog ){
			let sql = _syntax->CatalogSelect();
			_catalog = sql.size()
				? Scaler<string>( string{sql}, {}, sl )
				: string{};
		}
		return *_catalog;
	}

	α IDataSource::SchemaName( SL sl )ε->string{
		if( _schema.empty() ){
			let schema = Scaler<string>( string{_syntax->SchemaSelect()}, {}, sl ); THROW_IF( !schema, "Schema name is empty." );
			_schema = *schema;
		}
		return _schema;
	}
}