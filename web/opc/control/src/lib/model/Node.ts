import { NodeId, NodeIdJson } from "./NodeId";
import { ETypes, Browse, ILocalizedText, Ns, toLocalizedText, EAccess, EWriteAccess } from "./types";
import { toValue, Value } from "./Value";
import { Enum } from "./Enum";

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
	constructor( json:any, parent?:UaNode ){
		super( json );
		this.browse = json.browse;
		this.description = toLocalizedText( json.description );
		this.#name = toLocalizedText( json.name ); //TODO switch to just name?
		this.parent = parent;
		this.refType = json.referenceType ? new NodeId( json.referenceType ) : null;
		this.typeDef = json.typeDefinition ? new ObjectType( json.typeDefinition ) : null;
	}
	browseFQ( defaultNS:Ns ):string{ return this.browse.ns===defaultNS ? this.browse.name.toString() : `${this.browse.ns}~${this.browse.name}`; }

	get nodeId(){ return new NodeId( this ); }
	get name(){ return this.#name?.text; } #name:ILocalizedText;

	browse?:Browse;
	description:ILocalizedText;
	get displayed(){ return false; }
	get isSystem(){ return this.ns==0 && this.isNumericId && this.numericId<32750; }
	get isObject(){ return this.nodeClass == ENodeClass.Object; }
	get isVariable(){ return false; }
	abstract get nodeClass():ENodeClass;
	parent?:UaNode;
	refType?:NodeId;
	specified:number;
	typeDef?:ObjectType;
	userWriteMask:EWriteAccess;
	writeMask:EWriteAccess;
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
	constructor( json:{browseName?:Browse, dataType?:NodeIdJson, displayName:ILocalizedText, node?:NodeIdJson, nodeClass?:number, referenceType?:NodeIdJson, typeDefinition?:NodeIdJson, value?:any, valueRank?:number}, parent?:UaNode	){
		super( json, parent );
		if( json.dataType?.ns ){
			this.dataType = ETypes.None;
			this.customDataType = new NodeId( json.dataType );
		}
		else
			this.dataType = <ETypes>json.dataType?.i;
		this.value = toValue( json.value );
		this.valueRank = json.valueRank ?? -1;
		this.accessLevel = json["accessLevel"];
		this.userAccessLevel = json["userAccessLevel"];
	}
	override get nodeClass():ENodeClass{ return ENodeClass.Variable; }

	accessLevel?:EAccess;
	userAccessLevel?:EAccess;
	dataType?:ETypes;
	customDataType?:NodeId|Enum;
	override get displayed(){ return true; }
	get isArray():boolean{ return this.valueRank!=-1 && Array.isArray(this.value); }
	override get isVariable(){ return true; }
	get isInteger():boolean{ return [ETypes.SByte, ETypes.Int16, ETypes.Int32, ETypes.Int64].includes(this.dataType); }
	get isFloating():boolean{ return [ETypes.Float, ETypes.Double].includes(this.dataType); }
	get isUnsigned():boolean{ return [ETypes.Byte, ETypes.UInt16, ETypes.UInt32, ETypes.UInt64].includes(this.dataType); }
	value?:Value;
	valueRank?:number; // -1 scalar, 1 one-dimensional array, etc.
}