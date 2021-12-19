#pragma once
#include "Exports.h"
#include "../../../../Framework/source/db/DataSource.h"

namespace Jde::DB::Sqlite
{
	extern "C" ΓITE Jde::DB::IDataSource* GetDataSource();

	struct SqliteDS : IDataSource
	{
		α SchemaProc()noexcept->sp<ISchemaProc>;
		α Execute( string sql, SRCE )noexcept(false)->uint;
		α Execute( string sql, vec<object> parameters, SRCE )noexcept(false)->uint;
		α Execute( string sql, const vector<object>* pParameters, RowΛ* f, bool isStoredProc=false, SRCE )noexcept(false)->uint;
		α ExecuteNoLog( string sql, const vector<object>* pParameters, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )noexcept(false)->uint;
		α ExecuteProc( string sql, vec<object> parameters, SRCE )noexcept(false)->uint;
		α ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )noexcept(false)->uint;
		α ExecuteProcCo( string sql, vector<object> p, SRCE )noexcept->up<IAwaitable>;
		α ExecuteProcNoLog( string sql, vec<object> parameters, SRCE )noexcept(false)->uint;
		α Select( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint;
		α SelectNoLog( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint;
	private:
		α SelectCo( ISelect* pAwait, string sql, vector<object>&& params, SRCE )noexcept->up<IAwaitable>;
	};
}