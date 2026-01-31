#include <jde/ql/QLHook.h>
#include <jde/db/generators/Statement.h>

namespace Jde::Access::Server{
	struct UserAwait final : TAwait<jvalue>{
		UserAwait( const QL::TableQL& query, UserPK executer, SRCE )ι:
		TAwait<jvalue>{ sl },
			Query{ query },
			Executer{ executer }
		{}
		α Suspend()ι->void override{ QueryGroups(); }
		QL::TableQL Query;
		UserPK Executer;
	private:
		α QueryGroups()ι->TAwait<jarray>::Task;
		α QueryTables( jarray groups )ι->TAwait<jvalue>::Task;
		α GroupStatement()->DB::Statement;
		flat_map<uint8,string> _groupColumns;
	};
}