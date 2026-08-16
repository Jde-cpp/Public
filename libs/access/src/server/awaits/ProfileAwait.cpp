#include <jde/access/server/awaits/ProfileAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalSubscriptions.h>
#include "../serverInternal.h"

#define let const auto
namespace Jde::Access::Server{
	α ProfileAwait::Suspend()ι->void{
		Execute();
	}
	α ProfileAwait::Execute()ι->DB::ExecuteAwait::Task{
		try{
			THROW_IF( !_executer || _executer.Value==UserPK::System, "profile mutations require an authenticated user." );
			using enum QL::EMutationQL;
			THROW_IF( _mutation.Type!=Create && _mutation.Type!=Update, "profile mutation '{}' is not supported.", _mutation.CommandName );
			let args = _mutation.ExtrapolateVariables();
			let target = Json::AsString( args, "target" );
			let value = Json::FindString( args, "value" );
			let& table = GetTable( "profiles" );
			let ds = table.Schema->DS();
			uint count{};
			if( !value )
				count = co_await ds->Execute( DB::Sql{ Ƒ("delete from {} where identity_id=? and target=?", table.DBName), vector<DB::Value>{{_executer.Value}, {target}} }, _sl );
			else{
				count = co_await ds->Execute( DB::Sql{ Ƒ("update {} set value=? where identity_id=? and target=?", table.DBName), vector<DB::Value>{{*value}, {_executer.Value}, {target}} }, _sl );
				if( !count )//2 concurrent first saves can both reach the insert - the composite pk fails one and the client's next save heals it.
					count = co_await ds->Execute( DB::Sql{ Ƒ("insert into {}(identity_id,target,value)values(?,?,?)", table.DBName), vector<DB::Value>{{_executer.Value}, {target}, {*value}} }, _sl );
			}
			jobject y;
			y["rowCount"] = count;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}
