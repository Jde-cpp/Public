import { StatusCode } from "./types";

export class OpcError implements Error{
	constructor( public sc:StatusCode, public name:string, public stack:string, public cause:string|undefined ){
		if( !OpcError.messages.has(sc) )
			OpcError.messages.set( sc, undefined );
	}
	get message(){ return OpcError.messages.get(this.sc) ?? `sc=${this.sc}`; }
	toString(){ return `[${this.sc.toString(16)}] - ${OpcError.messages.get(this.sc)}`; }
	static emptyMessages(){
		let empty = [];
		for( const [sc, message] of OpcError.messages ) {
			if( message===null )
				empty.push( sc );
		};
		return empty;
	}
	static statusCodeText( sc:StatusCode ):string|undefined{
		const y = OpcError.messages.get(sc);
		if( !y )
			OpcError.messages.set( sc, null as unknown as string );
		return y;
	}
	static setMessages( x:OpcError[] ){ x.forEach( e=>
		OpcError.messages.set(e.sc, e.message) );
	}

	private static messages:Map<StatusCode,string|undefined> = new Map<StatusCode,string|undefined>();
}