import { Guid } from './guid';

const a = new Guid( "12345678-1234-5678-1234-567812345678" );
const b = new Guid( "87654321-4321-8765-4321-876543218765" );

describe( 'Guid.equals', ()=>{
	it( 'matches the same bytes and nothing else', ()=>{
		expect( a.equals(new Guid(a.toString())) ).toBe( true );
		expect( a.equals(b) ).toBe( false );
	} );

	//`every` is vacuously true for an empty array, so a byte-less Guid used to equal EVERY other one, and a short one
	//equalled any Guid it was a prefix of.
	it( 'a byte-less or short Guid matches nothing', ()=>{
		expect( new Guid().equals(a) ).toBe( false );
		expect( new Guid(a.value.slice(0, 4)).equals(a) ).toBe( false );
		expect( a.equals(new Guid()) ).toBe( false );
	} );
} );

//angular-review3 #14: JSON.stringify writes the Uint8Array as an OBJECT keyed by index, so a persisted Guid comes back as a
//plain {value:{…}} - `guid.equals is not a function` on the first comparison, on every row, until the profile row is cleared.
describe( 'Guid.fromJson', ()=>{
	it( 'revives what JSON.stringify actually wrote', ()=>{
		const revived = Guid.fromJson( JSON.parse(JSON.stringify(a)) )!;
		expect( revived ).toBeInstanceOf( Guid );
		expect( revived.equals(a) ).toBe( true );
		expect( revived.toString() ).toBe( a.toString() );
	} );

	it( 'takes an array, a string and a Guid too', ()=>{
		expect( Guid.fromJson({value: [...a.value]})!.equals(a) ).toBe( true );
		expect( Guid.fromJson(a.toString())!.equals(a) ).toBe( true );
		expect( Guid.fromJson(a) ).toBe( a );
	} );

	//not `new Guid()`: a byte-less Guid would be dropped by equals now, but returning one would still put junk in the filter.
	it( 'answers undefined for a shape it cannot read', ()=>{
		expect( Guid.fromJson(undefined) ).toBeUndefined();
		expect( Guid.fromJson({}) ).toBeUndefined();
		expect( Guid.fromJson({value: {}}) ).toBeUndefined();
	} );
} );
