#pragma once
#include "../Row.h"
#include "SelectAwait.h"
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	//The statement a scaler runs.  An InsertClause is a proc call whose trailing placeholder is the OUT param the
	//sequence comes back through (see IDataSource::ExecuteScalerSync) - it is appended here, typed as the scaler's T,
	//before Move builds the call; a plain Sql passes through.  The bool is Query's outParams: on a proc it asks the
	//driver for that OUT row.
	Ŧ scalerQuery( variant<Sql,InsertClause>&& sql )ι->std::pair<Sql,bool>{
		let isInsert = std::holds_alternative<InsertClause>( sql );
		if( isInsert )
			get<InsertClause>( sql ).Add( T{} );
		return { isInsert ? get<InsertClause>( move(sql) ).Move() : get<Sql>( move(sql) ), isInsert };
	}
	//#53: GetOpt, not Get.  Get on a NULL cell ASSERTs and returns T{}, so the async ScalerOpt answered an *engaged*
	//optional{0} where ScalerSyncOpt - which goes through Row::GetOpt - answers nullopt, and Scaler<T> then returned 0
	//instead of reporting "No value returned".  One API, one answer.
	Ŧ scalerValue( Result&& r )ι->optional<T>{
		return r.Rows.size() ? r.Rows[0].template GetOpt<T>( 0 ) : optional<T>{};
	}

	Τ struct ScalerAwaitOpt : TAwaitEx<optional<T>,void>{
		using base=TAwaitEx<optional<T>,void>;
		ScalerAwaitOpt( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι: base{ sl }, _ds{ move(ds) }, _sql{ move(s) }{}
		α Execute()ι->void override{
			auto [sql, isProc] = scalerQuery<T>( move(_sql) );
			RunQuery( *this, *_ds, move(sql), isProc, base::_sl, scalerValue<T> );
		}
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};

	//The one body ScalerAwait<T> and its ScalerAwait<uint32> specialisation share (C4): a Scaler is a ScalerOpt that
	//treats no row as an error, and RunQuery routes the throw to the promise exactly as it would a driver failure.
	Ŧ ScalerExecute( TAwait<T>& self, IDataSource& ds, variant<Sql,InsertClause>&& sql, SL sl )ι->QueryAwait::Task{
		auto [query, isProc] = scalerQuery<T>( move(sql) );
		return RunQuery( self, ds, move(query), isProc, sl, [sl]( Result&& r )->T{
			auto v = scalerValue<T>( move(r) );
			if( !v )
				throw Exception{ "No value returned", ELogLevel::Error, sl };
			return move( *v );
		});
	}

	Τ using ScalerBase = std::conditional_t<std::same_as<T,uint32>, UInt32Await, TAwait<T>>;
	Τ struct ScalerAwait : ScalerBase<T>{
		using base = ScalerBase<T>;
		ScalerAwait( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task{ return ScalerExecute<T>( *this, *_ds, move(_sql), base::_sl ); }
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};
}
