import { segmentDisplay } from './route-utils';

//The breadcrumb fallback for a segment nothing names.  "gateways" sat lowercase between "Applications" and the instance.
describe( 'segmentDisplay', ()=>{
	it( 'title-cases a lowercase segment', ()=>{
		expect( segmentDisplay('gateways') ).toBe( 'Gateways' );
	} );
	it( 'splits a camelCase segment into words', ()=>{
		expect( segmentDisplay('appServers') ).toBe( 'App Servers' );
	} );
	it( 'leaves anything that is not a plain identifier alone', ()=>{
		expect( segmentDisplay('OpcHub.debug') ).toBe( 'OpcHub.debug' );
		expect( segmentDisplay('my-target') ).toBe( 'my-target' );
		expect( segmentDisplay('42') ).toBe( '42' );
		expect( segmentDisplay('Already') ).toBe( 'Already' );
	} );
	it( 'decodes before deciding', ()=>{
		expect( segmentDisplay('a%20b') ).toBe( 'a b' );
	} );
} );
