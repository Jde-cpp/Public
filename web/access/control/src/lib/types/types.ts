import Long from "long";
import {Error} from "./Error";

export class Guid{
	constructor( x?:string ){
		if( !x )
			return;
		let trimmed = x.replace( /-/g, '' );
		this.value = Uint8Array.from( trimmed.match(/.{1,2}/g).map((byte) => parseInt(byte, 16)) );
	}
	toString():string{
		let y = this.value.reduce((str, byte) => str + byte.toString(16).padStart(2, '0'), '');
		return `${y.substring(0, 8)}-${y.substring(8, 12)}-${y.substring(12, 16)}-${y.substring(16, 20)}-${y.substring(20)}`;
	}
	value:Uint8Array;
}
export function toBinary( x:string ):Uint8Array{  return Uint8Array.from( atob(x), c => c.charCodeAt(0) ); }
export type NodeId = number | string | Guid | Uint8Array;
export type Namespace = string | number;
export class Timestamp{seconds:number; nanos:number;}
export class Duration{seconds:number; nanos:number;}
export type Value = boolean | Duration | Error | ExpandedNode | Guid | Long | Node | number | string | Timestamp | Uint8Array | Value[];
export type OpcId = string;

export function toValue( json:any ):Value{
	let value = json;
	if( value?.hasOwnProperty('sc') )
		value = new Error( json.sc );
	else if( value?.hasOwnProperty('unsigned') )
		value = new Long( json.low, json.high, json.unsigned );
	return value;
}

export function toString( value: Value ){
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
	else if( value instanceof Timestamp )
		return `${value.seconds}.${value.nanos}`;
	else if( value instanceof ExpandedNode )
		return value.toJson();
	else if( value instanceof Node )
		return value.id.toString();
	else if( value instanceof Error )
		return value.toString();
	else if( Array.isArray(value) )
		return value.map( x=>this.toString(x) ).join( "," );
	else
		return `unknown type ${typeof value}`;
}

export enum ENodes{
	ObjectsFolder = 85 /* Object */
}

export interface INode{
	ns:Namespace;
	id:NodeId;
}
export type NodeJson = {ns?:number, nsu?:string,s?:string,i?:number,g?:string,b?:string};
export class Node implements INode{
	constructor( json:NodeJson ){
		this.ns =  json.ns ? +json.ns : undefined;
		if( json.i!==undefined )
			this.id = +json.i;
		else if( json.s!==undefined )
			this.id = json.s;
		else if( json.g!==undefined )
			this.id = new Guid( json.g );
		else if( json.b!==undefined )
			this.id = toBinary( json.b );
		else
			this.id = Node.defaultNode;
	}
	public equals( obj: Node ):boolean{ return (this.ns ?? 0)==(obj.ns ?? 0) && this.id==obj.id; }
	toJson():NodeJson{
		let json:NodeJson = {};
		if( typeof this.ns === "string" )
			json.nsu = this.ns;
		else if( typeof this.ns === "number" && this.ns )
			json.ns = this.ns;

		let idValue = this.id;
		if( typeof this.id === "number" )
			json.i = this.id;
		else if( typeof this.id === "string" )
			json.s = this.id;
		else if( this.id instanceof Guid )
			json.g = this.id.toString();
		else if( this.id instanceof Uint8Array )
			json.b = btoa( this.id.reduce((acc, current) => acc + String.fromCharCode(current), "") );

		return json;
	}
	ns:number;
	id:NodeId;
	static defaultNode:NodeId = ENodes.ObjectsFolder;//TODO set from environment and document.
}

export interface IExpandedNode extends INode{
	serverIndex:number;
	nsu:string;
}
export type ExpandedNodeJson = {nsu?:string,serverIndex?:number} & NodeJson;
export type NodeKey = Symbol;
export class ExpandedNode extends Node implements IExpandedNode{
	constructor( json: ExpandedNodeJson ){
		super(json);
		this.serverIndex = json.serverIndex;
		this.nsu = json.nsu;
		if( !this.ns && !this.nsu && ExpandedNode.defaultNS )
			this.ns = ExpandedNode.defaultNS;
	}
	public override equals(obj: ExpandedNode) : boolean { return super.equals(obj) && (this.serverIndex ?? 0)==(obj.serverIndex ?? 0) && (this.nsu ?? 0)==(obj.nsu ?? 0); }

	get key():NodeKey{
		if( !this.#key ){
			let j = this.toJson();
			if( this.serverIndex )
				j["serverIndex"] = this.serverIndex;
			this.#key = Symbol.for( JSON.stringify(j) );
		}
		return this.#key;
  }
	serverIndex:number;
	nsu:string;
	#key:NodeKey;
	static defaultNS:number = 0;//TODO set from environment and document.
}

export interface ILocalizedText{
	locale: string;
	text: string;

}
export interface IBrowseName{
	ns:Namespace;
	name:String;
}

export enum ENodeClass{
  Unspecified = 0,
  Object = 1,
  Variable = 2,
  Method = 4,
  ObjectType = 8,
  VariableType = 16,
  ReferenceType = 32,
  DataType = 64,
  nodeClassView = 128,
}
export enum ETypes{
	None = 0,
	Boolean = 1,
	SByte = 2,
	Byte = 3,
	Int16 = 4,
	UInt16 = 5,
	Int32 = 6,
	UInt32 = 7,
	Int64 = 8,
	UInt64 = 9,
	Float = 10,
	Double = 11,
	String = 12,
	BaseDataType = 24,
	FolderType = 61
}
export interface IReference{
	browseName?:IBrowseName;
	displayName:ILocalizedText;
	isForward?:boolean;P
	node?:IExpandedNode;
	nodeClass?:ENodeClass;
	referenceType?:INode;
	typeDefinition?:IExpandedNode;
}
export class Reference{
	constructor( json:{browseName:IBrowseName, dataType:NodeJson, displayName:ILocalizedText, isForward:boolean, node?:ExpandedNodeJson, nodeClass?:number, referenceType?:NodeJson, typeDefinition?:ExpandedNodeJson, value:any } ){
		this.browseName = json.browseName;
		this.displayName = json.displayName;
		this.isForward = json.isForward;
		this.node = new ExpandedNode( json.node );
		this.nodeClass = <ENodeClass>json.nodeClass;
		this.referenceType = new Node( json.referenceType );
		this.dataType = this.node.id==ETypes.FolderType ? ETypes.FolderType : json.dataType.i;
		this.typeDefinition = new ExpandedNode( json.typeDefinition );
		this.value = toValue( json.value );
		// if( this.referenceType.id==ENodes.ObjectsFolder )
		// 	this.referenceType.id = environment.defaultNode;
	}
	nodeParams():NodeJson{ return this.node.toJson(); }
	browseName?:IBrowseName;
	dataType?:ETypes;
	displayName:ILocalizedText;
	get isArray():boolean{ return Array.isArray(this.value); }
	isForward?:boolean;
	get isFolderType(){ return this.dataType==ETypes.FolderType; }
	get isInteger():boolean{ return [ETypes.SByte, ETypes.Int16, ETypes.Int32, ETypes.Int64].includes(this.dataType); }
	get isFloating():boolean{ return [ETypes.Float, ETypes.Double].includes(this.dataType); }
	get isUnsigned():boolean{ return [ETypes.Byte, ETypes.UInt16, ETypes.UInt32, ETypes.UInt64].includes(this.dataType); }
	node?:ExpandedNode;
	value?:Value;
	nodeClass?:ENodeClass;
	referenceType?:INode;
	typeDefinition?:IExpandedNode;
}