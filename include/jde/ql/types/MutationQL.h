#pragma once
#include <jde/db/Key.h>
#include "TableQL.h"
#include "../usings.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct TableQL;
	struct MutationQL final : Input{
		MutationQL( string commandName, jobject&& args, sp<jobject> variables, optional<TableQL>&& resultRequest, bool returnRaw, const vector<sp<DB::AppSchema>>& schemas, bool system )ε;
		MutationQL( string commandName, jobject&& args, sp<jobject> variables, optional<TableQL>&& resultRequest, bool returnRaw, const sp<DB::AppSchema>& schema )ε;
		Ω IsMutation( sv name )ι->bool;
		Ω ParseCommand( sv name, SRCE )ε->tuple<string,EMutationQL>;
		α TableName()Ι->string; //json name=user returns users
		α JTableName()Ι->string override{ return JsonTableName; }
		α ToString()Ι->string;

		string CommandName;
		sp<DB::Table> DBTable;
		string JsonTableName;
		optional<TableQL> ResultRequest;
		bool ReturnRaw;
		EMutationQL Type;
	};
}