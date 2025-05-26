#include "TableDdl.h"
#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include "Index.h"
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	using std::endl;

	α TableDdl::CreateStatement()Ε->string{
		std::ostringstream createStatement;
		createStatement << "create table " << DBName << "(";
		string suffix = "";
		string pk;
		for( let& c : Columns ){
			if( c->IsSequence )
				pk = Syntax().CreatePrimaryKey( Name, c->Name );
			createStatement << suffix << endl << "\t" << ColumnDdl::CreateStatement( *c );
			suffix = ",";
		}
		if( pk.size() )
			createStatement << suffix << endl << "\t" << pk << endl;
		createStatement << ")";
		return createStatement.str();
	}

	α TableDdl::InsertProcCreateStatement()Ι->string{
		let& syntax = Syntax();
		std::ostringstream osCreate, osInsert, osValues;
		osCreate << "create procedure " << syntax.EscapeDdl(InsertProcName()) << "(";
		osInsert << "\tinsert into " << DBName << "(";
		osValues << "\t\tvalues(";
		let prefix = syntax.ProcParameterPrefix().empty() ? "_" : syntax.ProcParameterPrefix();
		char delimiter = ' ';
		for( let& c : Columns ){
			//let& c = dynamic_cast<const ColumnDdl&>( *column );
			auto value{ Ƒ("{}{}"sv, prefix, c->Name) };
			if( c->Insertable )
				osCreate << delimiter << prefix << c->Name << " " << ColumnDdl::DataTypeString( *c );
			else{
				if( c->IsNullable || !c->Default )
				 	continue;
				if( c->Default->is_string() && c->Default->get_string()=="$now" )
					value = ToSV( syntax.UtcNow() );
			}
			osInsert << delimiter << c->Name;
			osValues << delimiter  << value;
			delimiter = ',';
		}
		osInsert << " )" << endl;
		osValues << " );" << endl;
		osCreate << " )" << endl << syntax.ProcStart() << endl;
		osCreate << osInsert.str() << osValues.str();
		osCreate << "\tselect " << syntax.IdentitySelect() <<";" << endl << syntax.ProcEnd() << endl;// into _id
		return osCreate.str();
	}
}