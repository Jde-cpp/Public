#include "TableDdl.h"
#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include "Index.h"
#include "SchemaDdl.h"
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

	α TableDdl::InsertProcCreateStatement( const Table& config )Ι->string{
		let& syntax = Syntax();
		std::ostringstream osCreate, osInsert, osValues;
		string procName = InsertProcName();
		if( let index = procName.find_first_of('.'); index<procName.size()-1 )
			procName = procName.substr( index+1 );
		osCreate << "create procedure " << this->Schema->DBSchema->Name << "." << syntax.EscapeDdl( procName ) << "(";
		osInsert << "\tinsert into " << DBName << "(";
		osValues << "\t\tvalues(";
		let prefix = syntax.ProcParameterPrefix().empty() ? "_" : syntax.ProcParameterPrefix();
		char delimiter = ' ';
		for( let& c : config.Columns ){
			auto value{ Ƒ("{}{}"sv, prefix, c->Name) };
			if( c->Insertable )
				osCreate << delimiter << prefix << c->Name << " " << ColumnDdl::DataTypeString( *c );
			else{
				if( c->IsNullable || !c->Default )
				 	continue;
				if( c->Default->is_string() && c->Default->get_string()=="$now" )
					value = syntax.UtcNow();
			}
			osInsert << delimiter << c->Name;
			osValues << delimiter  << value;
			delimiter = ',';
		}
		osInsert << " )" << endl;
		osValues << " );" << endl;
		let seqCol = /*syntax.DriverReturnsLastInsertId() ? nullptr :*/ SequenceColumn();
		if( seqCol ){
			osCreate << delimiter;
			if( syntax.PrefixOut() )
				osCreate << " out";
			osCreate << " " << prefix << SequenceColumn()->Name << " " << ColumnDdl::DataTypeString( *seqCol );
			if( !syntax.PrefixOut() )
				osCreate << " output";
		}

		osCreate << " )" << endl << syntax.ProcStart() << endl;
		osCreate << osInsert.str() << osValues.str();
		if( seqCol )
			osCreate << "\tset " << prefix << seqCol->Name << " = " << syntax.IdentitySelect() << ";" << endl;
		osCreate << syntax.ProcEnd();
		return osCreate.str();
	}
}