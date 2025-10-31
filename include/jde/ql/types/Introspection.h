#pragma once
//#include <jde/fwk/io/jobject.h>

namespace Jde::QL{
	struct TableQL;
	enum class EFieldKind : uint8{ Scalar=0, Object=1, Interface=2, Union=3, Enum=4, InputObject=5, List=6, NonNull=7 };
	struct Type final{
		Type( const jobject& j )ε;
		α ToJson( const TableQL& query, bool ignoreNull )Ε->jobject;
		string Name;
		EFieldKind Kind;
		bool IsNullable{true};
		up<Type> OfTypePtr;
	};
	struct Field final{
		Field( const jobject& j )ε;
		α ToJson( const TableQL& query )Ε->jobject;
		string Name;
		Type FieldType;
	};

	struct EnumValue final{
		EnumValue( const jobject& j )ε;
		α ToJson( const TableQL& query )Ε->jobject;

		optional<_int> Id;
		string Name;
		string Description;
		bool IsDeprecated{};
		string DeprecationReason;
	};

	struct Object final{
		Object( sv name, const jobject& j )ε;
		α ToJson( const TableQL& query )Ε->jobject;

		string Name;
		vector<Field> Fields;
		vector<EnumValue> EnumValues;
	};

	struct Introspection final{
		Introspection()=default;
		Introspection( const jobject&& j )ε;
		α Find( sv name )Ι->const Object*;
		α operator+=( Introspection&& x )ι->void;

		vector<Object> Objects;
	};
}