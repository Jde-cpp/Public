import { NodeId, NodeIdJson } from "./NodeId";
import { ETypes, IBrowseName, ILocalizedText } from "./types";
import { toValue, Value } from "./Value";

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

export abstract class UaNode extends NodeId{
	constructor( json:any ){
		super( json );
		this.#browse = json.browseName;
		this.#name = json.displayName;
//		this.nodeClass = <ENodeClass>json.nodeClass;
		this.refType = json.referenceType ? new NodeId( json.referenceType ) : null;
		this.typeDef = json.typeDefinition ? new ObjectType( json.typeDefinition ) : null;
//		this.value = toValue( json.value );
	}
	//equals(rhs:NodeId):boolean{ return  }
	get displayed(){ return false; }
	get isSystem(){ return this.ns==0 && this.isNumericId && this.numericId<32750; }
	get isObject(){ return this.nodeClass == ENodeClass.Object; }
	get isVariable(){ return false; }
	abstract get nodeClass():ENodeClass;
	get nodeId(){ return new NodeId( this ); }
	parent?:UaNode;
	refType?:NodeId;
	typeDef?:ObjectType;
	get browse():string{ return this.#browse?.name?.toString(); } #browse?:IBrowseName;

	get name(){ return this.#name?.text; } #name:ILocalizedText;
	description:ILocalizedText;
	specified:number;
	userWriteMask:number;
	writeMask:number;
}

export class ObjectType extends UaNode{
	override get nodeClass():ENodeClass{ return ENodeClass.ObjectType; }
}

export enum EObjects{
	ObjectsFolder = 85, /* Object */
	Server = 2253
}
export class OpcObject extends UaNode{
	static get rootNode(){ return new OpcObject({ ns: 0, i: EObjects.ObjectsFolder, name: {locale:"en-US", text:"root"}}); }
	override get displayed(){ return !this.isSystem || this.equals(OpcObject.rootNode); }
	override get nodeClass():ENodeClass{ return ENodeClass.Object; }
}

export class Variable extends UaNode{
	constructor( json:{browseName?:IBrowseName, dataType?:NodeIdJson, displayName:ILocalizedText, node?:NodeIdJson, nodeClass?:number, referenceType?:NodeIdJson, typeDefinition?:NodeIdJson, value?:any } ){
		super(json);
		this.dataType = <ETypes>json.dataType?.i;
		this.value = toValue( json.value );
	}
	override get nodeClass():ENodeClass{ return ENodeClass.Variable; }

	dataType?:ETypes;
	override get displayed(){ return true; }
	get isArray():boolean{ return Array.isArray(this.value); }
	override get isVariable(){ return true; }
//	get isFolderType(){ return this.dataType==ETypes.FolderType; }
	get isInteger():boolean{ return [ETypes.SByte, ETypes.Int16, ETypes.Int32, ETypes.Int64].includes(this.dataType); }
	get isFloating():boolean{ return [ETypes.Float, ETypes.Double].includes(this.dataType); }
	get isUnsigned():boolean{ return [ETypes.Byte, ETypes.UInt16, ETypes.UInt32, ETypes.UInt64].includes(this.dataType); }
//	nodeId?:NodeId;
	value?:Value;
//
//	referenceType?:INodeId;
//	typeDefinition?:INodeId;
}
