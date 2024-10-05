#include "TableDdl.h"
#include "Syntax.h"
#include <jde/db/metadata/Column.h>
#include <jde/db/metadata/Table.h>

#define let const auto

namespace Jde::DB{
	using std::endl;

	α TableDdl::Create()Ι->string{
		std::ostringstream createStatement;
		createStatement << "create table " << Name << "(" << endl;
		string suffix = "";
		string pk;
		for( let& c : Columns ){
			if( c->IsIdentity )
				pk = Ƒ( "primary key({})"sv, c->Name );
			createStatement << suffix << endl << "\t" << c->Create( *Syntax );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

	α TableDdl::InsertProcText()Ι->string{
		let& t = *Table;
		std::ostringstream osCreate, osInsert, osValues;
		osCreate << "create procedure " << InsertProcName() << "(";
		osInsert << "\tinsert into " << Name << "(";
		osValues << "\t\tvalues(";
		let prefix = Syntax->ProcParameterPrefix().empty() ? "_"sv : Syntax->ProcParameterPrefix();
		char delimiter = ' ';
		for( let& column : Columns ){
			let& c = *column;
			auto value{ Ƒ("{}{}"sv, prefix, c.Name) };
			if( c.Insertable )
				osCreate << delimiter << prefix << c.Name << " " << c.DataTypeString( *Syntax );
			else{
				if( c.IsNullable || c.Default.empty() )
				 	continue;
				if( c.Default=="$now" )
					value = ToSV( Syntax->UtcNow() );
			}
			osInsert << delimiter << c.Name;
			osValues << delimiter  << value;
			delimiter = ',';
		}
		osInsert << " )" << endl;
		osValues << " );" << endl;
		osCreate << " )" << endl << Syntax->ProcStart() << endl;
		osCreate << osInsert.str() << osValues.str();
		osCreate << "\tselect " << Syntax->IdentitySelect() <<";" << endl << Syntax->ProcEnd() << endl;// into _id
		return osCreate.str();
	}
}