#include <jde/ql/Introspection.h>
#include <jde/ql/GraphQL.h>

#define let const auto

namespace Jde::QL{
	Type::Type( const jobject& j )ε:
		Name{ j["name"].is_null() ? "" : j["name"].get<string>() },
		Kind{ Str::ToEnum<QLFieldKind>(QLFieldKindStrings, j["kind"].get<string>()).value_or(QLFieldKind::Scalar) },
		IsNullable{ Kind!=QLFieldKind::NonNull }{
		if( !IsNullable ){
			json type = j["ofType"];
			Name = type["name"].get<string>();
			Kind = Str::ToEnum<QLFieldKind>( QLFieldKindStrings, type["kind"].template get<string>() ).value_or( QLFieldKind::Scalar );
			if( type.contains("ofType") )
				OfTypePtr = mu<Type>( type["ofType"] );
		}
		else if( j.contains("ofType") )
			OfTypePtr = mu<Type>( j["ofType"] );
	}
	α Type::ToJson( const TableQL& query, bool ignoreNull )Ε->jobject{
		json jType;
		bool isNullType = ignoreNull || IsNullable;
		if( auto pColumn = query.FindColumn("name"); pColumn ){
			if( isNullType && Name.size() )
				jType["name"] = Name;
			else
				jType["name"] = nullptr;
		}
		if( auto pColumn = query.FindColumn("kind"); pColumn )
			jType["kind"] = Str::FromEnum<QLFieldKind>( QLFieldKindStrings, isNullType ? Kind : QLFieldKind::NonNull );
		auto pOfType = !IsNullable && !ignoreNull ? this : OfTypePtr.get();
		if( auto pTable = pOfType ? query.FindTable("ofType") : nullptr; pTable )
			jType["ofType"] = pOfType->ToJson( *pTable, pOfType==this );
		return jType;
	}

	Field::Field( const jobject& j )ε:
		Name{ j["name"] },
		FieldType{ j["type"] }
	{}

	α Field::ToJson( const TableQL& query )Ε->jobject{
		json jField;
		if( auto pColumn = query.FindColumn("name"); pColumn )
			jField["name"] = Name;
		if( auto pTable = query.FindTable("type"); pTable )
			jField["type"] = FieldType.ToJson( *pTable, false );
		return jField;
	}

	Object::Object( str name, const jobject& j )ε:
		Name{ name }{
		for( const json& field : j["fields"] ){
			Fields.emplace_back( field );
		}
	}

	α Object::ToJson( const TableQL& query )Ε->jobject{
		ASSERT( query.JsonName=="fields" );
		jobject jTable;
		jTable["name"] = Name;
		auto fields = json::array();
		for( let& field : Fields )
			fields.push_back( field.ToJson(query) );
		jTable["fields"] = fields;
		return jTable;
	}

	Introspection::Introspection( const jobject&& j )ε{
		for( let& [name, Value]  : j.items() )
			Objects.emplace_back( name, Value );
	}

	α Introspection::Find( sv name )Ι->const Object*{
		auto y = find_if( Objects, [name](let& Value){ return Value.Name==name; } );
		return y==Objects.end() ? nullptr : &*y;
	}
}