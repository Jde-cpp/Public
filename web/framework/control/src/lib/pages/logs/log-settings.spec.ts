import { Guid } from '../../model/guid';
import { LogSettings } from './log-settings';

//ProfileStore.load hands back plain JSON.parse output, so LogDetail has to build a LogSettings from it - and a persisted
//hiddenMessages entry is {value:{"0":18,…}}, not a Guid (angular-review3 #14).
describe( 'LogSettings revival', ()=>{
	const id = new Guid( "12345678-1234-5678-1234-567812345678" );

	it( 'revives persisted hiddenMessages into real Guids', ()=>{
		const stored = JSON.parse( JSON.stringify({hiddenMessages: [id], level: 2}) );
		const settings = new LogSettings( stored );
		expect( settings.hiddenMessages ).toHaveLength( 1 );
		expect( settings.hiddenMessages[0] ).toBeInstanceOf( Guid );
		expect( settings.hiddenMessages[0].equals(id) ).toBe( true );//log-data-source's isHidden calls this on every row
		expect( settings.level ).toBe( 2 );
	} );

	it( 'drops an entry it cannot revive rather than filtering everything out', ()=>{
		const settings = new LogSettings( <any>{hiddenMessages: [{}, {value: {}}]} );
		expect( settings.hiddenMessages ).toEqual( [] );
	} );

	it( 'leaves a settings object with no hiddenMessages alone', ()=>{
		expect( new LogSettings(<any>{level: 1}).hiddenMessages ).toEqual( [] );
	} );
} );
