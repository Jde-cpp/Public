import Long from "long";
import { Duration, Guid, ProtoUtils, Timestamp  } from "jde-framework";
import { NodeId } from "./NodeId";
import { ExNodeId } from "./ExNodeId";
import {OpcError} from "../model/OpcError";

export type Value = boolean | Duration | OpcError | ExNodeId | Guid | Long | NodeId | number | string | Timestamp | Uint8Array | Value[];

export function valueJson( value: Value ){
	if( value instanceof ExNodeId )
		return value.toJson();
	else if( value instanceof NodeId )
		return value.id;
	else if( value instanceof Uint8Array )
		return {b: btoa(value.reduce((acc, current) => acc + String.fromCharCode(current), "")) };
	else if( value instanceof OpcError )
		return {sc: value.sc };
	else if( Array.isArray(value) )
		return value.map( x=>valueJson(x) );
	else
		return value;
}

export function valueString( value: Value ){
	if( typeof value === "string" )
		return value;
	else if( typeof value === "number" )
		return value.toString();
	else if( typeof value === "boolean" )
		return value.toString();
	else if( value instanceof Long )
		return value.toString();
	else if( value instanceof Guid )
		return value.toString();
	else if( value instanceof Uint8Array )
		return btoa( value.reduce((acc, current) => acc + String.fromCharCode(current), "") );
	else if( Object.hasOwn(value, "seconds") && Object.hasOwn(value, "nanos") )
		return ProtoUtils.toDate( <Timestamp>value ).toISOString();
	else if( value instanceof ExNodeId )
		return value.toJson();
	else if( value instanceof NodeId )
		return value.id.toString();
	else if( value instanceof OpcError )
		return value.toString();
	else if( Array.isArray(value) )
		return value.map( x=>this.toString(x) ).join( "," );
	else
		return `unknown type ${typeof value}`;
}

export function toValue( json:any ):Value{
	let value = json;
	if( value?.hasOwnProperty('sc') )
		value = new Error( json.sc );
	else if( value?.hasOwnProperty('unsigned') )
		value = new Long( json.low, json.high, json.unsigned );
	return value;
}
