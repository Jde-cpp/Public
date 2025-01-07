#pragma once
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
		α FindParam( sv name )Ε->const jvalue*;
		//α InputParam( sv name )Ε->const jvalue&;
		α ParentPK()Ε->uint;
		α ChildPK()Ε->uint;

		string CommandName;
		string JsonTableName;
		EMutationQL Type;
		jobject Args;
		optional<TableQL> ResultRequest;
		bool ReturnRaw;
	private:
		mutable string _tableName;
	};

	Ŧ MutationQL::Id()Ι->T{
		const auto y = Json::FindNumber<T>( Args, "id" );
		ASSERT( y );
		return y.value_or( 0 );
	}
}