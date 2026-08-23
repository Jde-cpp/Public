import { StatusCode } from "./types";

export class OpcError implements Error{
	constructor( public sc:StatusCode, public name:string, public stack:string, public cause:string|undefined ){
		if( !OpcError.messages.has(sc) )
			OpcError.messages.set( sc, null );//null = "pending fetch" sentinel that emptyMessages() collects; undefined would be skipped and never fetched from /ErrorCodes
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
			OpcError.messages.set( sc, null );
		return y ?? undefined;
	}
	static setMessages( x:OpcError[] ){ x.forEach( e=>
		OpcError.messages.set(e.sc, e.message) );
	}

	private static messages:Map<StatusCode,string|null> = new Map<StatusCode,string|null>();
}