
export class OpcError implements Error{
	constructor( public sc:number, public name:string, public stack:string, public cause:string ){
		if( !OpcError.messages.has(sc) )
			OpcError.messages.set( sc, null );
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
	static setMessages( x:OpcError[] ){ x.forEach( e=>
		OpcError.messages.set(e.sc, e.message) );
	}

	private static messages:Map<number,string> = new Map<number,string>();
}