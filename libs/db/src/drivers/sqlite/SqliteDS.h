#pragma once
#include "Exports.h"
#include "../../../../Framework/source/db/DataSource.h"

namespace Jde::DB::Sqlite
{
	extern "C" ΓITE Jde::DB::IDataSource* GetDataSource();

	struct SqliteDS : IDataSource
	{
		α SchemaProc()noexcept->sp<ISchemaProc> override;
		α Execute( string sql, SRCE )noexcept(false)->uint override;
		α Execute( string sql, vec<object> parameters, SRCE )noexcept(false)->uint override;
		α Execute( string sql, const vector<object>* pParameters, RowΛ* f, bool isStoredProc=false, SRCE )noexcept(false)->uint override;
		α ExecuteNoLog( string sql, const vector<object>* pParameters, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )noexcept(false)->uint override;
		α ExecuteProc( string sql, vec<object> parameters, SRCE )noexcept(false)->uint override;
		α ExecuteProc( string sql, vec<object> parameters, RowΛ f, SRCE )noexcept(false)->uint override;
		α ExecuteProcCo( string sql, vector<object> p, SRCE )noexcept->up<IAwait> override;
		α ExecuteProcNoLog( string sql, vec<object> parameters, SRCE )noexcept(false)->uint override;
		α Select( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint override;
		α SelectNoLog( string sql, RowΛ f, const vector<object>* pValues, SRCE )noexcept(false)->uint override;
	private:
		α SelectCo( ISelect* pAwait, string sql, vector<object>&& params, SRCE )noexcept->up<IAwait> override;
	};
}