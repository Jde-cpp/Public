import { Guid } from "jde-framework";

export type NodeKey = Symbol;

export type NodeIdentifier = number | string | Guid | Uint8Array;

export type Namespace = number;
export type NodeIdJson = {ns?:Namespace,s?:string,i?:number,g?:string,b?:string};
function toBinary( x:string ):Uint8Array{  return Uint8Array.from( atob(x), c => c.charCodeAt(0) ); }

export interface INodeId{
	ns:Namespace;
	id:NodeIdentifier;
}

export class NodeId implements INodeId{
	constructor( json:any /*NodeIdJson|INodeId*/ ){
		if( json["node"] )
			json = json["node"];
		if( typeof json === "number" )
			this.id = json;
		else if( typeof json === "string" )
			this.id = json;
		else if( json instanceof Guid )
			this.id = json;
		else if( json instanceof Uint8Array )
			this.id = json;
		if( this.id !== undefined ){//id may legitimately be 0 or "" — test presence, not truthiness
			this.ns ??= 0;
			return;
		}
		let j = <NodeIdJson>json;
		this.ns = j.ns ?? 0;
		if( (j as any)["id"] !== undefined )
			this.id = (j as any)["id"];
		else if( j.i!==undefined )
			this.id = +j.i;
		else if( j.s!==undefined )
			this.id = j.s;
		else if( j.g!==undefined )
			this.id = new Guid( j.g );
		else if( j.b!==undefined )
			this.id = toBinary( j.b );
		else
			console.error( `NodeId - unrecognized json: ${JSON.stringify(json)}` );//was `debugger;` — froze the app whenever a debugger (DevTools/automation) was attached
	}
	public equals( rhs: NodeId ):boolean{ return this.ns==rhs.ns && this.id===rhs.id; }
	static objectsFolder = new NodeId( { ns: 0, i: 85 } );
	toJson():NodeIdJson{
		let json:NodeIdJson = {};
		json.ns = this.ns;
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
	toJSON():NodeIdJson{ return this.toJson(); }//serialize NodeId[] (e.g. saved subscriptions) in the NodeIdJson form the constructor can revive
	toString():string{ return JSON.stringify( this.toJson() ); }//stable + unique per node (was "[object Object]", collapsing every node onto one profile key)
	uaString():string{//OPC-UA standard NodeId string, e.g. "ns=4;i=6020" (ns=0 omitted, per open62541) — matches the server's resource `criteria`
		const p = this.ns ? `ns=${this.ns};` : "";
		if( typeof this.id === "number" )        return `${p}i=${this.id}`;
		else if( typeof this.id === "string" )   return `${p}s=${this.id}`;
		else if( this.id instanceof Guid )       return `${p}g=${this.id.toString()}`;
		else if( this.id instanceof Uint8Array ) return `${p}b=${btoa( this.id.reduce((acc, current) => acc + String.fromCharCode(current), "") )}`;
		return p;
	}
	qlArgs(escape:boolean=false):string{ // ns:4,i:5003
		let y: string = `ns:${this.ns},`;
		if( typeof this.id === "number" )
			y += `i:${this.id}`;
		else if( typeof this.id === "string" )
			y += `s:"${this.id}"`;
		else if( this.id instanceof Guid )
			y += `g:"${this.id.toString()}"`;
		else if( this.id instanceof Uint8Array )
			y += `b:"${btoa( this.id.reduce((acc, current) => acc + String.fromCharCode(current), "") )}"`;
		return escape ? y.replace(/['"]/g, '\\$&') : y;
	}
	static qlArgsArray( nodes:NodeId[] ):string{
		return nodes.map( n=>`{${n.qlArgs()}}` ).join( "," );
	}

	ns!:number;
	get isNumericId(){ return typeof this.id === "number"; }
	get numericId(){ return <number>this.id; }
	id!:NodeIdentifier;
	get isObjectsFolder(){ return this.equals(NodeId.objectsFolder); }

	get key():NodeKey{
		if( !this._key ){
			let j = this.toJson();
			this._key = Symbol.for( JSON.stringify(j) );
		}
		return this._key;
	}
	protected _key:NodeKey|undefined; //for Map
}