#include "ColumnDdl.h"
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Table.h>

#define let const auto
namespace Jde::DB{

	ColumnDdl::ColumnDdl( sv name, uint /*ordinalPosition*/, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, optional<uint8> skIndex, optional<uint> /*numericPrecision*/, optional<uint> /*numericScale*/ )ι:
		Column{name}//,
	{
		if( type==EType::Bit )
			Default = Value{ dflt=="1" };
		IsNullable = isNullable;
		Type = type;
		MaxLength = maxLength;
		IsSequence = isSequence;
		SKIndex = skIndex;
	}

	α ColumnDdl::DataTypeString( const Column& config )ι->string{
		let& syntax = config.Table->Syntax();
		let useMaxLength = config.MaxLength && syntax.HasLength( config.Type );
		return useMaxLength ? Ƒ( "{}({})", syntax.ToString(config.Type), *config.MaxLength ) : syntax.ToString( config.Type );
	}

	α ColumnDdl::CreateStatement( const Column& config )ε->string{
		let& syntax = config.Table->Syntax();
		let null = config.IsNullable ? "null"sv : "not null"sv;
		const string sequence = config.IsSequence ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string defaultClause;
		let& dflt = config.Default;
		if( dflt && !dflt->is_null() ){
			if( dflt->is_bool() )
				defaultClause = Ƒ( " default {}", dflt->get_bool() ? 1 : 0 );
			else if( string s = dflt->is_string() ? dflt->get_string() : string{}; s.size() )
				defaultClause = Ƒ( " default {}", s=="$now" ? ToStr(syntax.NowDefault()) : Ƒ("'{}'", s) );
			else if( config.Type!=EType::VarBinary )
				THROW( "({})Default type not implemented.", dflt->TypeName() );
		}
		return Ƒ( "{} {} {}{}{}", config.Name, DataTypeString(config), null, sequence, defaultClause );
	}
}