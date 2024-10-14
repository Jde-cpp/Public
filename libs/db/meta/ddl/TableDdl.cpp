#include "TableDdl.h"
#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include "Index.h"
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	using std::endl;

	α TableDdl::CreateStatement()Ι->string{
		std::ostringstream createStatement;
		createStatement << "create table " << Name << "(" << endl;
		string suffix = "";
		string pk;
		for( let& column : Columns ){
			let& c = dynamic_cast<const ColumnDdl&>( *column );
			if( c.IsSequence )
				pk = Ƒ( "primary key({})"sv, c.Name );
			createStatement << suffix << endl << "\t" << c.CreateStatement( Syntax() );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

	α TableDdl::InsertProcCreateStatement()Ι->string{
		std::ostringstream osCreate, osInsert, osValues;
		osCreate << "create procedure " << InsertProcName() << "(";
		osInsert << "\tinsert into " << Name << "(";
		osValues << "\t\tvalues(";
		let prefix = Syntax().ProcParameterPrefix().empty() ? "_"sv : Syntax().ProcParameterPrefix();
		char delimiter = ' ';
		for( let& column : Columns ){
			let& c = dynamic_cast<const ColumnDdl&>( *column );
			auto value{ Ƒ("{}{}"sv, prefix, c.Name) };
			if( c.Insertable )
				osCreate << delimiter << prefix << c.Name << " " << c.DataTypeString( Syntax() );
			else{
				if( c.IsNullable || !c.Default )
				 	continue;
				if( c.Default->is_string() && c.Default->get_string()=="$now" )
					value = ToSV( Syntax().UtcNow() );
			}
			osInsert << delimiter << c.Name;
			osValues << delimiter  << value;
			delimiter = ',';
		}
		osInsert << " )" << endl;
		osValues << " );" << endl;
		osCreate << " )" << endl << Syntax().ProcStart() << endl;
		osCreate << osInsert.str() << osValues.str();
		osCreate << "\tselect " << Syntax().IdentitySelect() <<";" << endl << Syntax().ProcEnd() << endl;// into _id
		return osCreate.str();
	}
}