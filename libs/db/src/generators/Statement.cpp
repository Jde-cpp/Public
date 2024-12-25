#include <jde/db/generators/Statement.h>
#include <jde/db/meta/View.h>

namespace Jde::DB{

	α Statement::Move()ε->Sql{
		Sql sql;
		sql.Text += Select.ToString()+'\n';
		sql.Text += From.ToString()+'\n';
		sql.Text += Where.Move();
		if( _limit )
			sql.Text = From.GetFirstTable()->Syntax().Limit( sql.Text, _limit );
		sql.Params = move( Where.Params() );
		return sql;
	}
}