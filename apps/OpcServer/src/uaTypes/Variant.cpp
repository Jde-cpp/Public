#include "Variant.h"

namespace Jde::Opc::Server{
	Variant::Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι:
		UA_Variant{ &dataType, UA_VARIANT_DATA, get<0>(data), get<1>(data), get<1>(dims), get<0>(dims) },
		VariantPK{pk}
	{}
	Variant::Variant( UA_Variant&& v )ι{
		UA_Variant_copy( &v, this );
		UA_Variant_clear( this );
		VariantPK = {};
	}

	α Variant::GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*{
		void* p = UA_new( &type );
		if( let sc = UA_decodeJson(&j, p, &type, nullptr); sc )
			throw UAException{ sc, nullptr, 0, ELogLevel::Debug };
		return p;
	}

	α Variant::ToUAValues( const UA_DataType& type, flat_map<uint32, string>&& values )ι->tuple<uint,void*>{
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