#include <gtest/gtest.h>
#include <jde/db/DBException.h>
#include <jde/db/generators/Sql.h>
#include <jde/fwk/log/MemoryLog.h>
#include <jde/fwk/log/Entry.h>

#define let const auto

namespace Jde::DB::Tests{
	//#12: DBException::Log must honor the base _logged protocol - a moved-from / already-logged exception must not re-log
	//(before the fix, the moved-from source logged a junk "sqle: " with its emptied Sql, and a BreakLog'd exception double-logged).
	TEST( DBExceptionTests, LogsOnce ){
		if( !Logging::FindLogger<Logging::MemoryLog>() )
			Logging::AddLogger( mu<Logging::MemoryLog>() ); //captures every level; self-contained, no shared-config change.
		let countSqle = []{ return Logging::Find( [](const Logging::Entry& e){ return e.Message().starts_with("sqle:"); } ).size(); };

		Logging::ClearMemory();
		{
			DB::Sql sql; sql.Text = "select dbexception_logonce";
			DBException src{ 7, move(sql), "boom", SRCE_CUR };
			DBException dst{ move(src) }; //src is now moved-from: Sql emptied, _logged=true.
		} //dtors run: dst logs exactly once; src stays silent (was the junk "sqle: " / double-log before the fix).
		EXPECT_EQ( countSqle(), 1u );
	}
}
