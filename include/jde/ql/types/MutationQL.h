#pragma once
#include "TableQL.h"

namespace Jde::QL{
	struct TableQL;
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6, Start=7, Stop=8 };
	struct MutationQL final{
		MutationQL( sv j, EMutationQL type, const jobject& args, optional<TableQL> resultPtr ):JsonName{j}, Type{type}, Args(args), ResultPtr{resultPtr}{}
		α TableName()Ι->string; //json name=user returns users
		α Input(SRCE)Ε->const jobject&;
		template<class T=uint> α Id()Ι->T;
		α FindParam( sv name )Ε->const jvalue*;
		α InputParam( sv name )Ε->const jvalue&;
		α ParentPK()Ε->uint;
		α ChildPK()Ε->uint;

		string JsonName;
		EMutationQL Type;
		jobject Args;
		optional<TableQL> ResultPtr;
	private:
		mutable string _tableName;
	};

	Ŧ MutationQL::Id()Ι->T{
		const auto y = Json::FindNumber<T>( Args, "id" );
		ASSERT( y );
		return y.value_or( 0 );
	}
}