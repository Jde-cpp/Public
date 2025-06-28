#include "VariableAttr.h"
#include "Variant.h"
#include "DataType.h"

#define let const auto
namespace Jde::Opc::Server {
	α getAccessLevelMask( const jarray* accessLevels )ι->UA_Byte{
		UA_Byte mask = 0;
		if( !accessLevels || accessLevels->empty() )
			return mask;
		for( auto&& m : *accessLevels ){
			let value = m.as_string();
			if( value=="read" )
				mask |= UA_ACCESSLEVELMASK_READ;
			else if( value=="currentRead" )
				mask |= UA_ACCESSLEVELMASK_CURRENTREAD;
			else if( value=="write" )
				mask |= UA_ACCESSLEVELMASK_WRITE;
			else if( value=="currentWrite" )
				mask |= UA_ACCESSLEVELMASK_CURRENTWRITE;
			else if( value=="historyRead" )
				mask |= UA_ACCESSLEVELMASK_HISTORYREAD;
			else if( value=="historyWrite" )
				mask |= UA_ACCESSLEVELMASK_HISTORYWRITE;
			else if( value=="semanticChange" )
				mask |= UA_ACCESSLEVELMASK_SEMANTICCHANGE;
			else if( value=="statusWrite" )
				mask |= UA_ACCESSLEVELMASK_STATUSWRITE;
			else if( value=="timestampWrite" )
				mask |= UA_ACCESSLEVELMASK_TIMESTAMPWRITE;
		}
		return mask;
	}
	α getValueRank( sv value )-> UA_Int32{
		if( value=="scalar" )
			return UA_VALUERANK_SCALAR;
		else if( value=="one" )
			return UA_VALUERANK_ONE_DIMENSION;
		else if( value=="any" )
			return UA_VALUERANK_ANY;
		else if( value=="scalarOrOne" )
			return UA_VALUERANK_SCALAR_OR_ONE_DIMENSION;
		else if( value=="two" )
			return UA_VALUERANK_TWO_DIMENSIONS;
		else if( value=="three" )
			return UA_VALUERANK_THREE_DIMENSIONS;
		else
			return UA_VALUERANK_ANY;
	}
	VariableAttr::VariableAttr( jobject& j, SL sl )ε:
		UA_VariableAttributes{
			0,
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::AsString(j, "name")) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::FindDefaultSV(j, "description")) },
			getAccessLevelMask( Json::FindArray(j,"writeMask") ),
			getAccessLevelMask( Json::FindArray(j,"userWriteMask") ),
			j.contains("value") ? Variant{ j.at("value"), Json::FindSV(j, "dataType").value_or("") } : UA_VariableAttributes_default.value,
			j.contains("dataType") ? DT( j.at("dataType"), sl ).typeId : value.type ? value.type->typeId : UA_TYPES[UA_TYPES_STRING].typeId,
			j.contains("valueRank") ? getValueRank(j.at("valueRank").as_string()) : UA_VALUERANK_ANY,       // valueRank
			0,       // arrayDimensionsSize
			nullptr, // arrayDimensions
			getAccessLevelMask( Json::FindArray(j,"accessLevel") ),
			getAccessLevelMask( Json::FindArray(j,"userAccessLevel") ),
			Json::FindNumber<double>(j, "minimumSamplingInterval").value_or( -1.0 ),
			Json::FindBool( j, "historizing" ).value_or( false )
		}
	{}

	VariableAttr::VariableAttr( DB::Row& r, UA_Variant&& variant, const UA_DataType& dataType, tuple<UA_UInt32*, uint> dims )ε:
		//Node{ move(r) },
		UA_VariableAttributes{
			r.GetOpt<UA_UInt32>(18).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(19)) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(20)) },
			r.GetOpt<UA_UInt32>(21).value_or(0),
			r.GetOpt<UA_UInt32>(22).value_or(0),
			variant,
			dataType.typeId,
			r.GetOpt<UA_Int32>(25).value_or(-2),
			get<1>(dims),
			get<0>(dims),
			r.GetUInt8Opt(27).value_or(0),
			r.GetUInt8Opt(28).value_or(0),
			r.GetDoubleOpt(29).value_or(-1.0),
			r.GetBit(30)
		}
	{}

	α VariableAttr::ArrayDimensionsString( size_t arrayDimensionsSize, UA_UInt32* arrayDimensions )ι->string{
		if( !arrayDimensions || arrayDimensionsSize==0 )
			return {};
		string result;
		for( size_t i = 0; i < arrayDimensionsSize; ++i )
			result += std::to_string(arrayDimensions[i])+',';
		if( !result.empty() )
			result.pop_back(); // Remove the trailing comma
		return result;
	}
}