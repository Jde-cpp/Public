#pragma once

namespace Jde::QL{
	struct TableQL;
	enum class QLFieldKind : uint8{ Scalar=0, Object=1, Interface=2, Union=3, Enum=4, InputObject=5, List=6, NonNull=7 };
	constexpr array<sv,8> QLFieldKindStrings = { "SCALAR", "OBJECT", "INTERFACE", "UNION", "ENUM", "INPUT_OBJECT", "LIST", "NON_NULL" };
	struct Type final{
		Type( const json& j )ε;
		α ToJson( const TableQL& query, bool ignoreNull )Ε->json;
		string Name;
		QLFieldKind Kind;
		bool IsNullable{true};
		up<Type> OfTypePtr;
	};
	struct Field final{
		Field( const json& j )ε;
		α ToJson( const TableQL& query )Ε->json;
		string Name;
		Type FieldType;
	};

	struct Object final{
		Object( str name, const json& j )ε;
		α ToJson( const TableQL& query )Ε->json;
		string Name;
		vector<Field> Fields;
	};

	struct Introspection final{
		Introspection( const json&& j )ε;
		α Find( sv name )Ι->const Object*;

		vector<Object> Objects;
	};
}