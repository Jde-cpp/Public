//import { toBinary } from "./types";
//import { EObjects } from "./Node"

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
		let j = <NodeIdJson>json;
		this.ns = j.ns ?? 0;
		if( j["id"] )
			this.id = j["id"];
		else if( j.i!==undefined )
			this.id = +j.i;
		else if( j.s!==undefined )
			this.id = j.s;
		else if( j.g!==undefined )
			this.id = new Guid( j.g );
		else if( j.b!==undefined )
			this.id = toBinary( j.b );
		else
			debugger;
	}
	public equals( rhs: NodeId ):boolean{ return this.ns==rhs.ns && this.id===rhs.id; }
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
/*	toQueryParams():string{
		let json = this.toJson();
		let params = [];
		if( json.ns )
			params.push( `ns=${json.ns}` );
		if( json.i )
			params.push( `i=${json.i}` );
		else if( json.s )
			params.push( `s=${json.s}` );
		else if( json.g )
			params.push( `g=${json.g}` );
		else if( json.b )
			params.push( `b=${json.b}` );
		return params.join( "&" );
	}*/
	ns:number;
	get isNumericId(){ return typeof this.id === "number"; }
	get numericId(){ return <number>this.id; }
	id:NodeIdentifier;

	get key():NodeKey{
		if( !this._key ){
			let j = this.toJson();
			this._key = Symbol.for( JSON.stringify(j) );
		}
		return this._key;
	}
	protected _key:NodeKey; //for Map
}