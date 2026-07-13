#include "TableDdl.h"
#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include "Index.h"
#include "SchemaDdl.h"
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	using std::endl;

	TableDdl::~TableDdl()=default;

	α TableDdl::CreateStatement()Ε->string{
		let& syntax = Syntax();
		std::ostringstream createStatement;
		createStatement << "create table " << DBName << "(";
		string suffix = "";
		for( let& c : Columns ){
			createStatement << suffix << endl << "\t" << ColumnDdl::CreateStatement( *c );
			suffix = ",";
		}
		//SurrogateKeys is the (possibly composite) primary key, ordered by SKIndex; a sequence column has SKIndex 0 so it is included here.
		if( SurrogateKeys.size() ){
			string columns, columnDelimiter;
			for( let& c : SurrogateKeys ){
				columns += columnDelimiter + c->Name;
				columnDelimiter = ", ";
			}
			createStatement << suffix << endl << "\t" << syntax.CreatePrimaryKey( Name, columns ) << endl;
		}
		//When the syntax can't 'alter table add constraint' (sqlite), SyncFKs is skipped, so declare fks inline here -
		//it's the only place they get enforced. Column::NeedsFK is the selection rule shared with SchemaDdl::SyncFKs.
		if( !syntax.CanAddForeignKeys() ){
			for( let& c : Columns ){
				if( !c->NeedsFK() )
					continue;
				let& pk = *c->PKTable;
				createStatement << suffix << endl << "\tforeign key(" << c->Name << ") references " << pk.DBName << "(" << pk.GetPK()->Name << ")";
				suffix = ",";
			}
		}
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