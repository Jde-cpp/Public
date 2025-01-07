#include <jde/ql/types/MutationQL.h>
#include <jde/db/names.h>
#include <jde/db/meta/DBSchema.h>
#include "Parser.h"

#define let const auto

namespace Jde::QL{

	Ω parseCommandName( sv commandName )ε->tuple<string,EMutationQL>{
		uint iType=0;
		for( ;iType<MutationQLStrings.size() && !commandName.starts_with(MutationQLStrings[iType]); ++iType );
		THROW_IF( iType==MutationQLStrings.size(), "Could not find mutation {}", commandName );

		auto tableJsonName = string{ commandName.substr(MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );

		return { move(tableJsonName), (EMutationQL)iType };
	}

	MutationQL::MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw )ε:
		CommandName{move(commandName)}, Args(move(args)), ResultRequest{move(resultRequest)}, ReturnRaw{returnRaw}{
			std::tie(JsonTableName,Type) = parseCommandName( CommandName );
		}


	α MutationQL::TableName()Ι->string{
		if( _tableName.empty() )
			_tableName = DB::Names::ToPlural<string>( DB::Names::FromJson<string,string>(JsonTableName) );
		return _tableName;
	}

	α MutationQL::FindParam( sv name )Ε->const jvalue*{
		return Json::FindValue( Args, name );
	}
	//α MutationQL::InputParam( sv key )Ε->const jvalue&{
	//	return Json::AsValue( Input(), key );
	//}

	//α MutationQL::Input(SL sl)Ε->const jobject&{
	//	return Json::AsObject( Args, "input", sl );
	//}

	α MutationQL::IsMutation( sv name )ι->bool{
		bool isMutation{ name=="mutation" };
		for( uint i=name=="query" ? MutationQLStrings.size() : 0; !isMutation && i<MutationQLStrings.size(); ++i )
			isMutation = name.starts_with( MutationQLStrings[i] );
		return isMutation;
	}
}