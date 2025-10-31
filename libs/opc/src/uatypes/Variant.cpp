#include <jde/opc/uatypes/Variant.h>
#include <jde/opc/UAException.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/UAString.h>

#define let const auto
namespace Jde::Opc{
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
		WARNT( ELogTags::App, "Unsupported data type in Variant: ", serialize(v) );
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
				WARNT( ELogTags::App, "Unsupported data type in Variant: ", serialize(v) );
				UA_Variant_setScalar( this, nullptr, &UA_TYPES[UA_TYPES_STRING] );
			}
		}
		else{
			WARNT( ELogTags::App, "Unsupported data type in Variant: ", serialize(v) );
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
	Variant::Variant( UA_Variant&& v )ι:
		UA_Variant{ v }{
		UA_Variant_init( &v );
	}
	Variant::Variant( StatusCode sc )ι:
		UA_Variant{ &UA_TYPES[UA_TYPES_STATUSCODE], UA_VARIANT_DATA_NODELETE, 0, (void*)(uintptr_t)sc }
		//UA_Variant_setScalar( this, &sc, &UA_TYPES[UA_TYPES_STATUSCODE] );
	{}

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

	Ω uaJsonString( void* v, const UA_DataType& type )ε->string{
		UAString j{ 2096 };
		UA_EncodeJsonOptions options{};
		if( let sc=UA_encodeJson(v, &type, &j, &options); sc )
			throw UAException{ sc };
		return j.ToString();
	}
	α Variant::ToUAJson()ε->vector<string>{
		vector<string> y;
		if( IsNull() )
			return y;
		if( IsScalar() ){
			y.emplace_back( uaJsonString(data, *this->type) );
		}else{
			for( uint i=0; i<arrayLength; ++i )
				y.emplace_back( uaJsonString(((void**)data)[i], *type) );
		}
		return y;
	}
	α Variant::ToJson( bool trimNames )ε->jvalue{
		jvalue y;
		if( IsNull() )
			return y;
		auto toJson = [trimNames]( void* v, const UA_DataType& type )ε->jvalue{
			if( &type==&UA_TYPES[UA_TYPES_LOCALIZEDTEXT] && trimNames )
				return jstring{ ToString( ((UA_LocalizedText*)v)->text ) };
			else{
				auto uaJson = uaJsonString( v, type );
				try{
					return parse( uaJson );
				}
				catch( exception& e ){
					ERRT( ELogTags::Parsing, "Error parsing {} - {}", uaJson, e.what() );
					return {uaJson};
				}
			}
		};
		if( IsScalar() ){
			if( type==&UA_TYPES[UA_TYPES_STATUSCODE] )
				y = toJson( &data, *type );
			else if( type==&UA_TYPES[UA_TYPES_NODEID] )
			  y = Opc::ToJson( *(UA_NodeId*)data );
			else if( type==&UA_TYPES[UA_TYPES_QUALIFIEDNAME] )
			  y = BrowseName::ToJson( *(UA_QualifiedName*)data );
			else
				y = toJson( data, *type );
		}
		else{
			jarray arr;
			for( uint i=0; i<arrayLength; ++i )
				arr.emplace_back( toJson(((void**)data)[i], *type) );
			y = move(arr);
		}
		return y;
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
		return { size==1 ? 0 : size, data };//size==1 would be an array
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