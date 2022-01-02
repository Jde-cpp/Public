#include "SqliteDS.h"
#include "SqliteSchemaProc.h"

Jde::DB::IDataSource* GetDataSource()
{
	return new Jde::DB::Sqlite::SqliteDS();
}

namespace Jde::DB::Sqlite
{
	struct Connection
	{
		Connection( fs::path path )
		{
			::sqlite3_open( path.string().c_str(), &_pConnection );
		}
		~Connection()
		{
			if( _pConnection )
				::sqlite3_close_v2( _pConnection );
		}
	private:
		::sqlite3* _pConnection;
	};

	α SqliteDS::SchemaProc()noexcept->sp<ISchemaProc>//TODO not shared ptr, probably static.
	{
		return ms<Sqlite::SchemaProc>( shared_from_this() );
	}

			sqlite3_exec(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);

	α SqliteDS::Execute( string sql, SL sl )noexcept(false)->uint
	{
		try
		{
			Connection c{ _connectionString };
		}
		catch( IException& )
		{}
	}
}