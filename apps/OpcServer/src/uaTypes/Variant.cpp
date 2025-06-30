#include "Variant.h"

namespace Jde::Opc::Server{
	α getDataType( const jvalue& v )ι->const UA_DataType*{
		if( v.is_bool() )
			return &UA_TYPES[UA_TYPES_BOOLEAN];
		if( v.is_string() )
			return &UA_TYPES[UA_TYPES_STRING];
		if( v.is_object() ){
			let type = Json::AsString( v.get_object(), "type" );
			if( type=="double" )
				return &UA_TYPES[UA_TYPES_DOUBLE];
		}
		Warning{ ELogTags::App, "Unsupported data type in Variant: ", serialize(v) };
		return &UA_TYPES[UA_TYPES_STRING];
	}
	Variant::Variant( const jvalue& v, sv dataType )ι:
		UA_Variant{},
		VariantPK{0}{
		if( v.is_bool() ){
			auto boolValue = v.get_bool();
			UA_Variant_setScalarCopy( this, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN] );
		}else if( v.is_string() ){
			auto sValue = ToUV( v.get_string() );
			UA_Variant_setScalarCopy( this, &sValue, &UA_TYPES[UA_TYPES_STRING] );
		}else if( dataType.size() ){
			if( dataType=="double" ){
				auto doubleValue = Json::AsNumber<double>( v );
				UA_Variant_setScalarCopy( this, &doubleValue, &UA_TYPES[UA_TYPES_DOUBLE] );
			}
			else{
				Warning{ ELogTags::App, "Unsupported data type in Variant: ", serialize(v) };
				UA_Variant_setScalar( this, nullptr, &UA_TYPES[UA_TYPES_STRING] );
			}
		}
		else{
			Warning{ ELogTags::App, "Unsupported data type in Variant: ", serialize(v) };
			UA_Variant_setScalar( this, nullptr, &UA_TYPES[UA_TYPES_STRING] );
		}
	}

	Variant::Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι:
		UA_Variant{ &dataType, UA_VARIANT_DATA, get<0>(data), get<1>(data), get<1>(dims), get<0>(dims) },
		VariantPK{pk}
	{}

	Variant::Variant( Variant&& v )ι:
		UA_Variant{ move(v) },
		VariantPK{ move(v.VariantPK) }{
		UA_Variant_init( &v );
	}
	Variant::Variant( const Variant& v )ι:
		VariantPK{ v.VariantPK }{
		UA_Variant_copy( &v, this );
	}

	Variant::Variant( const UA_Variant& v )ι{
		UA_Variant_copy( &v, this );
	}

	α Variant::operator=( const Variant& v )ι->Variant&{
		if( this==&v )
			return *this;
		UA_Variant_copy( &v, this );
		VariantPK = v.VariantPK;
		return *this;
	}
	α Variant::operator=( Variant&& v )ι->Variant&{
		if( this==&v )
			return *this;
		UA_Variant{ move(v) };
		UA_Variant_init( &v );
		VariantPK = move(v.VariantPK);
		return *this;
	}

	α Variant::GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*{
		void* p = UA_new( &type );
		if( let sc = UA_decodeJson(&j, p, &type, nullptr); sc )
			throw UAException{ sc };
		return p;
	}

	α Variant::ArrayDimString()ι->string{
		if( !arrayDimensions || arrayDimensionsSize==0 )
			return {};
		string csv;
		for( uint i=0; i<arrayDimensionsSize; ++i )
			csv += std::to_string( arrayDimensions[i] )+',';
		csv.pop_back();
		return csv;
	}

	α Variant::ToJson()ε->flat_map<uint32, string>{
		flat_map<uint32, string> values;
		if( UA_Variant_isEmpty(this) )
			return values;
		auto add = [&]( int i, void* value ){
			UA_String j;
			UA_ByteString_allocBuffer( &j, 2096 );
			UA_EncodeJsonOptions options{};
			UA_encodeJson( value, type, &j, &options );
			values.emplace( i, ToString(j) );
			UA_String_clear( &j );
		};
		if( UA_Variant_isScalar(this) )
			add( 0, data );
		else{
			for( uint i=0; i<arrayLength; ++i )
				add( i, ((void**)data)[i] );
		}
		return values;
	}
	α Variant::ToUAValues( const UA_DataType& type, flat_map<uint, string>&& values )ι->tuple<uint,void*>{
		void* data = nullptr;
		let size = values.size();
		try{
			if( size==1 )
				data = GetUAValue( type, ToUV(values.begin()->second) );
			else if( size>1 ){
				data = UA_Array_new( values.size(), &type );
				uint i=0;
				for( auto&& [_, j] : values )
					((void**)data)[i++] = GetUAValue( type, ToUV(j) );
			}
			values.clear();
		}
		catch( exception& ){
			if( data )
				UA_Array_delete( data, size, &type );
			return {0, nullptr};
		}
		return { size, data };
	}

	α Variant::ToArrayDims( str csv )ι->tuple<UA_UInt32*, uint>{
		if( csv.empty() )
			return {nullptr, 0};
		auto dims = Str::Split( csv );
		let size = dims.size();
		auto arrayDims = (UA_UInt32*)UA_Array_new( size, &UA_TYPES[UA_TYPES_UINT32] );
		for( uint i=0; i<size; ++i )
			arrayDims[i] = Str::TryTo<UA_UInt32>( string{dims[i]} ).value_or(0);
		return {arrayDims, size};
	}
}