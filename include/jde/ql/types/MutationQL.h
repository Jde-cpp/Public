#pragma once
#include <jde/db/Key.h>
#include "TableQL.h"
#include "../usings.h"

namespace Jde::QL{
	struct TableQL;
	struct MutationQL final{
		MutationQL( string commandName, jobject&& args, optional<TableQL>&& resultRequest, bool returnRaw )ε;
		Ω IsMutation( sv name )ι->bool;
		α TableName()Ι->string; //json name=user returns users
		//α Input(SRCE)Ε->const jobject&;
		template<class T=uint> α Id()Ι->T;
		template<class T=uint> α FindId()Ι->optional<T>;
		α GetKey(SRCE)ε->DB::Key;
		α FindKey(SRCE)ι->optional<DB::Key>;
		α FindParam( sv name )Ι->const jvalue*;
		α GetParam( sv name, SRCE )Ε->const jvalue&;
		α ParentPK()Ε->uint;
		α ChildPK()Ε->uint;
		α ToString()Ι->string;

		string CommandName;
		string JsonTableName;
		EMutationQL Type;
		jobject Args;
		optional<TableQL> ResultRequest;
		bool ReturnRaw;
	private:
		mutable string _tableName;
	};

	Ŧ MutationQL::FindId()Ι->optional<T>{
		return Json::FindNumber<T>( Args, "id" );
	}

	Ŧ MutationQL::Id()Ι->T{
		const auto y = FindId();
		ASSERT( y );
		return y.value_or( 0 );
	}
}