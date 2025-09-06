
export class Guid{
	constructor( x?:string|Uint8Array<ArrayBufferLike> ){
		if( !x )
			return;
		if( typeof x === "string" ){
			let trimmed = x.replace( /-/g, '' );
			this.value = Uint8Array.from( trimmed.match(/.{1,2}/g).map((byte) => parseInt(byte, 16)) );
		}
		else if( x instanceof Uint8Array ){
			this.value = x;
		}
	}
	toString():string{
		let y = this.value.reduce((str, byte) => str + byte.toString(16).padStart(2, '0'), '');
		return `${y.substring(0, 8)}-${y.substring(8, 12)}-${y.substring(12, 16)}-${y.substring(16, 20)}-${y.substring(20)}`;
	}
	value:Uint8Array;
}
