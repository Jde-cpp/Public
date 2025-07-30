#include <jde/ql/types/MutationQL.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include "Parser.h"

#define let const auto

namespace Jde::QL{
	α MutationQL::ParseCommand( sv commandName )ε->tuple<string,EMutationQL>{
		uint iType=0;
		for( ;iType<MutationQLStrings.size() && !commandName.starts_with(MutationQLStrings[iType]); ++iType );
		THROW_IF( iType==MutationQLStrings.size(), "Could not find mutation {}", commandName );

		auto tableJsonName = string{ commandName.substr(MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );

		return { move(tableJsonName), (EMutationQL)iType };
	}

	MutationQL::MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw, const vector<sp<DB::AppSchema>>& schemas )ε:
		Args(move(args)), CommandName{move(commandName)}, ResultRequest{move(resultRequest)}, ReturnRaw{returnRaw}{
		std::tie(JsonTableName,Type) = ParseCommand( CommandName );
		DBTable = DB::AppSchema::GetTablePtr( schemas, DB::Names::ToPlural(DB::Names::FromJson(JsonTableName)) );
	}
	MutationQL::MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw, const sp<DB::AppSchema>& schema )ε:
		Args(move(args)), CommandName{move(commandName)}, ResultRequest{move(resultRequest)}, ReturnRaw{returnRaw}{
		std::tie(JsonTableName,Type) = ParseCommand( CommandName );
		DBTable = schema->GetTablePtr( DB::Names::ToPlural(DB::Names::FromJson(JsonTableName)) );
	}

	α MutationQL::TableName()Ι->string{ ASSERT(DBTable); return DBTable->Name; }
	α MutationQL::ToString()Ι->string{
		auto args = serialize(Args);
		if( args.size()>3 && args[0]=='{' )
			args = args.substr(1, args.size()-2);
		return Ƒ( "{}({}){}", CommandName, serialize(Args), ResultRequest ? ResultRequest->ToString() : "" );
	}

	α MutationQL::FindParam( sv name )ι->jvalue*{
		return Json::FindValue( Args, name );
	}
	α MutationQL::GetParam( sv name, SL sl )ε->jvalue&{
		auto p = FindParam( name );
		THROW_IFSL( !p, "Could not find param '{}' in '{}'", name, serialize(Args) );
		return *p;
	}

	α MutationQL::GetKey(SL sl)ε->DB::Key{
		let y = FindKey();
		THROW_IFSL( !y, "Could not find id or target in mutation '{}'", ToString() );
		return *y;
	}
	α MutationQL::FindKey()ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = FindId<uint>(); id )
			y = DB::Key{ *id };
		else if( let target = FindParam("target"); target )
			y = DB::Key{ Json::AsString(move(*target)) };
		return y;
	}

	α MutationQL::IsMutation( sv name )ι->bool{
		bool isMutation{ name=="mutation" };
		for( uint i=name=="query" ? MutationQLStrings.size() : 0; !isMutation && i<MutationQLStrings.size(); ++i )
			isMutation = name.starts_with( MutationQLStrings[i] );
		return isMutation;
	}
}