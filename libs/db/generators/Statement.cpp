#include <jde/db/generators/Statement.h>
//#include <jde/db/meta/Column.h>
//#include <jde/db/meta/View.h>

namespace Jde::DB{
	α Statement::Move()ι->Sql{
		Sql sql;
		sql.Text += Select.ToString()+'\n';
		sql.Text += From.ToString()+'\n';
		sql.Text += Where.Move();
		sql.Params = move( Where.Params() );
		return sql;
	}
}