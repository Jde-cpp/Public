#pragma once
#include "../Row.h"
#include "SelectAwait.h"
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	ΓDB α ScalerAwaitExecute( IDataSource& _ds, variant<Sql,InsertClause>&& _sql, function<void(optional<Row>)> onRow, function<void(IException&&)> onError, SL sl )ι->QueryAwait::Task;

	Τ struct ScalerAwaitOpt : TAwaitEx<optional<T>,void>{
		using base=TAwaitEx<optional<T>,void>;
		ScalerAwaitOpt( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι: base{ sl }, _ds{ move(ds) }, _sql{ move(s) }{}
		α Execute()ι->void override;
		α OnRow( optional<Row> result )ι->void;
		α OnError( IException&& e )ι->void;
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};

	Τ struct ScalerAwait : TAwait<T>{
		using base = TAwait<T>;
		ScalerAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		ScalerAwait( sp<IDataSource> ds, InsertClause&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ScalerAwaitOpt<T>::Task{
			try{
				auto opt = co_await ScalerAwaitOpt<T>{ move(_ds), move(_sql), base::_sl };
				if( opt )
					base::Resume( move(*opt) );
				else
					base::ResumeExp( Exception{"No value returned", ELogLevel::Error, base::_sl} );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};


	Ŧ ScalerAwaitOpt<T>::Execute()ι->void{
		ScalerAwaitExecute( *_ds, move(_sql),
		[this](optional<Row> r){OnRow(move(r));},
		[this](IException&& e){OnError(move(e));},
		base::_sl );
	}
	Ŧ ScalerAwaitOpt<T>::OnRow( optional<Row> r )ι->void{
		try{
			base::ResumeScaler( r ? r->template Get<T>(0) : optional<T>{} );
		}
		catch( IException& e ){
			base::ResumeExp( move(e) );
		}
	}
	Ŧ ScalerAwaitOpt<T>::OnError( IException&& e )ι->void{
		base::ResumeExp( move(e) );
	}
}