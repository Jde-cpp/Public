#include "Variable.h"
#include "VariableAttr.h"
#include "Reference.h"

#define let const auto
namespace Jde::Opc::Server{
	Variable::Variable( const Variable& v )ι:
		Node{ v },
		_refs{ v._refs }{
			UA_VariableAttributes_copy( &v, this );
	}
	Variable::Variable( Variable&& v )ι:
		Node{ move(v) },
		UA_VariableAttributes{ move(v) },
		_refs{ move(v._refs) }{
		UA_VariableAttributes_init( &v );
	}
	α getRefs( jarray&& refs )ι->vector<sp<Reference>>{
		vector<sp<Reference>> result;
		result.reserve(refs.size());
		for( auto&& r : refs )
			result.emplace_back( ms<Reference>(r.as_object()) );
		return result;
	}
	Variable::Variable( jobject&& j, NodePK parentPK, Server::BrowseName browse )ι:
		Node{ j, parentPK, move(browse) },
		UA_VariableAttributes{ VariableAttr{j} },
		_refs{ j.contains("refs") ? getRefs(move(j.at("refs").as_array())) : vector<sp<Reference>>{} }
	{}
	Variable::Variable( UA_NodeId n )ι:
		Node{ {move(n)} },
		UA_VariableAttributes{}
	{}
	Variable::Variable( DB::Row& r, sp<ObjectType> typeDef, UA_Variant&& variant, const UA_DataType& dataType, tuple<UA_UInt32*, uint> dims )ε:
		Node{ move(r), typeDef },
		UA_VariableAttributes{
			r.GetUInt32Opt(13).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(14)) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(15)) },
			r.GetUInt8Opt(16).value_or(0),
			r.GetUInt8Opt(17).value_or(0),
			move(variant),
			dataType.typeId,
			r.GetInt32Opt(23).value_or(-2),
			get<1>(dims),
			get<0>(dims),
			r.GetUInt8Opt(25).value_or(0),
			r.GetUInt8Opt(26).value_or(0),
			r.GetDoubleOpt(27).value_or(-1.0),
			r.GetBitOpt(28).value_or(false)
		}
	{}
	α Variable::operator=( const Variable& v )ι->Variable&{
		if( this != &v ){
			Node::operator=( v );
			UA_VariableAttributes_copy( &v, this );
			_refs = v._refs;
		}
		BREAK_IF(PK==50898);
		return *this;
	}
	α Variable::operator=( Variable&& v )ι->Variable&{
		if( this != &v ){
			Node::operator=( move(v) );
			memcpy( &this->specifiedAttributes, &v.specifiedAttributes, sizeof(UA_VariableAttributes) );
			_refs = move(v._refs);
			UA_VariableAttributes_init( &v );
		}
		BREAK_IF(PK==50898);
		return *this;
	}

	α Variable::InsertParams( DB::Value variantPK )ι->vector<DB::Value>{
		auto params = Node::InsertParams();
		params.emplace_back( variantPK );
		ASSERT( dataType.identifier.numeric );
		params.emplace_back( dataType.identifier.numeric );
		params.emplace_back( valueRank );
		let dims = arrayDimensionsSize ? VariableAttr::ArrayDimensionsString(arrayDimensionsSize, arrayDimensions) : "";
		params.emplace_back( dims, "" );
		params.emplace_back( accessLevel, 0 );
		params.emplace_back( userAccessLevel, 0 );
		params.emplace_back( minimumSamplingInterval, -1.0 );
		params.emplace_back( historizing, false );
		return params;
	}
}