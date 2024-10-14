#pragma once
//#include <jde/framework/io/jobject.h>

namespace Jde::QL{
	struct TableQL;
	enum class QLFieldKind : uint8{ Scalar=0, Object=1, Interface=2, Union=3, Enum=4, InputObject=5, List=6, NonNull=7 };
	constexpr array<sv,8> QLFieldKindStrings = { "SCALAR", "OBJECT", "INTERFACE", "UNION", "ENUM", "INPUT_OBJECT", "LIST", "NON_NULL" };
	struct Type final{
		Type( const jobject& j )ε;
		α ToJson( const TableQL& query, bool ignoreNull )Ε->jobject;
		string Name;
		QLFieldKind Kind;
		bool IsNullable{true};
		up<Type> OfTypePtr;
	};
	struct Field final{
		Field( const jobject& j )ε;
		α ToJson( const TableQL& query )Ε->jobject;
		string Name;
		Type FieldType;
	};

	struct Object final{
		Object( str name, const jobject& j )ε;
		α ToJson( const TableQL& query )Ε->jobject;
		string Name;
		vector<Field> Fields;
	};

	struct Introspection final{
		Introspection( const jobject&& j )ε;
		α Find( sv name )Ι->const Object*;

		vector<Object> Objects;
	};
}