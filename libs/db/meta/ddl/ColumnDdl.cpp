#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Table.h>

#define let const auto
namespace Jde::DB{

	ColumnDdl::ColumnDdl( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, optional<uint8> skIndex, optional<uint> numericPrecision, optional<uint> numericScale )ι:
		Column{name}//,
//		NumericPrecision{numericPrecision},
//		NumericScale{numericScale}
	{
		if( type==EType::Bit )
			Default->set_bool( dflt=="1" );
		IsNullable = isNullable;
		Type = type;
		MaxLength = maxLength;
		IsSequence = isSequence;
		SKIndex = skIndex;
	}

	α ColumnDdl::DataTypeString( const Syntax& syntax )Ι->string{
		return MaxLength ? Ƒ("{}({})", syntax.ToString(Type), *MaxLength) : syntax.ToString( Type );
	}

	α ColumnDdl::CreateStatement( const Syntax& syntax )Ε->string{
		let null = IsNullable ? "null"sv : "not null"sv;
		const string sequence = IsSequence ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string dflt;
		if( Default && !Default->is_null() ){
			if( Default->is_bool() )
				dflt = Ƒ( " default {}", Default->get_bool() ? 1 : 0 );
			else if( string s = Default->is_string() ? Default->get_string() : string{}; s.size() )
				dflt = Ƒ( " default {}", s=="$now" ? ToStr(Table->Syntax().NowDefault()) : Ƒ("'{}'", s) );
			else
				THROW( "{}Default type not implemented.", Default->TypeName() );
		}
		return Ƒ( "{} {} {}{}{}", Name, DataTypeString(syntax), null, sequence, dflt );
	}
}