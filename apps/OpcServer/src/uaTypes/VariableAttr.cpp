#include "VariableAttr.h"
#include <jde/opc/uatypes/Variant.h>
#include "DataType.h"

#define let const auto
namespace Jde::Opc::Server {
	α getAccessLevelMask( const jarray* accessLevels )ι->UA_Byte{
		UA_Byte mask = 0;
		if( !accessLevels )
			return UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_STATUSWRITE | UA_ACCESSLEVELMASK_TIMESTAMPWRITE;
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
	Ω toVariables( jobject& o, SL sl )->UA_VariableAttributes{
		auto variant = o.contains("value") ? Variant{ o.at("value"), Json::FindSV(o, "dataType").value_or("") } : UA_VariableAttributes_default.value;
		auto dataType = o.contains("dataType") ? DT( o.at("dataType"), sl ).typeId : variant.type ? variant.type->typeId : UA_TYPES[UA_TYPES_STRING].typeId;
		return UA_VariableAttributes{
			0,
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::AsString(o, "name")) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::FindDefaultSV(o, "description")) },
			getAccessLevelMask( Json::FindArray(o,"writeMask") ),
			getAccessLevelMask( Json::FindArray(o,"userWriteMask") ),
			move(variant),
			dataType,
			o.contains("valueRank") ? getValueRank(o.at("valueRank").as_string()) : UA_VALUERANK_ANY,       // valueRank
			0,       // arrayDimensionsSize
			nullptr, // arrayDimensions
			getAccessLevelMask( Json::FindArray(o,"accessLevel") ),
			getAccessLevelMask( Json::FindArray(o,"userAccessLevel") ),
			Json::FindNumber<double>(o, "minimumSamplingInterval").value_or( -1.0 ),
			Json::FindBool( o, "historizing" ).value_or( false )
		};
	}
	VariableAttr::VariableAttr( jobject& o, SL sl )ε:
		UA_VariableAttributes{ toVariables(o, sl) }
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