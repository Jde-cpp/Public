export interface IError{
	sc:number;
	message:string;
}

export class Error implements IError
{
	constructor( public sc:number )
	{
		if( !Error.messages.has(sc) )
			Error.messages.set( sc, null );
	}
	get message(){ return Error.messages.get(this.sc) ?? `sc=${this.sc}`; }
	toString(){ return `[${this.sc.toString(16)}] - ${Error.messages.get(this.sc)}`; }
	static emptyMessages(){
		let empty = [];
		for( const [sc, message] of Error.messages ) {
			if( message===null ){
				Error.messages.set( sc, '' );
				empty.push( sc );
			}
		};
		return empty;
	}
	static setMessages( x:IError[] ){ x.forEach( e=>Error.messages.set(e.sc, e.message) ); }

	private static messages:Map<number,string> = new Map<number,string>();
}