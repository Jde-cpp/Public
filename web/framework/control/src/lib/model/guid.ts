
export class Guid{
	constructor( x?:string|Uint8Array<ArrayBufferLike> ){
		if( !x )
			return;
		if( typeof x === "string" ){
			let trimmed = x.replace( /-/g, '' );
			this.value = Uint8Array.from( trimmed.match(/.{1,2}/g)!.map((byte) => parseInt(byte, 16)) );
		}
		else if( x instanceof Uint8Array ){
			this.value = x;
		}
	}
	//JSON.stringify turns the Uint8Array into an OBJECT keyed by index ({"0":18,"1":52,…}), not an array, so a round-tripped
	//Guid comes back as a plain {value:{…}} with no prototype - `guid.equals is not a function` at the first comparison.
	//undefined for a shape this cannot make sense of:  a Guid with a short or missing `value` would compare EQUAL to
	//everything (see equals), so dropping it is the only safe answer.
	static fromJson( json:any ):Guid|undefined{
		if( json instanceof Guid )
			return json;
		if( typeof json==="string" )
			return new Guid( json );
		const value = json?.value;
		if( value instanceof Uint8Array )
			return new Guid( value );
		if( Array.isArray(value) )
			return new Guid( Uint8Array.from(value) );
		if( value && typeof value==="object" ){
			const indexes = Object.keys( value ).map( Number );
			if( indexes.length && indexes.every(i=>Number.isInteger(i)) )
				return new Guid( Uint8Array.from(indexes.sort((a,b)=>a-b).map(i=>value[i])) );
		}
		return undefined;
	}
	toString():string{
		let y = this.value.reduce((str, byte) => str + byte.toString(16).padStart(2, '0'), '');
		return `${y.substring(0, 8)}-${y.substring(8, 12)}-${y.substring(12, 16)}-${y.substring(16, 20)}-${y.substring(20)}`;
	}
	//the length test is not decoration:  `every` is vacuously true for an empty array, so a Guid with no bytes used to equal
	//every other one - and a short one equalled any Guid it was a prefix of.
	equals( other:Guid ){ return !!this.value && !!other?.value && this.value.length==other.value.length && this.value.every( (byte, index) => byte === other.value[index] ); }
	value!:Uint8Array;
}
