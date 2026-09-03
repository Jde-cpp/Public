//review3 L11: neither environment file carried an `httpTransport` key, so AppService was constructed with `undefined` as
//its transport.  EnvironmentService.get() looks keys up by NAME, so a key spelled only in the development file reads back
//undefined in production - the key set has to match, and only the app can see both files.
import { ETransport } from 'jde-framework';
import { environment as production } from '../environments/environment';
import { environment as development } from '../environments/environment.development';

describe( 'environment key sets', ()=>{
	it( 'declare httpTransport in production and development alike', ()=>{
		expect( production.httpTransport ).toBe( ETransport.Unsecure );
		expect( development.httpTransport ).toBe( ETransport.Unsecure );
	} );

	it( 'agree on their key set, bar the production flag', ()=>{
		const keys = ( e:object )=>Object.keys( e ).filter( k=>k!='production' ).sort();
		expect( keys(development) ).toEqual( expect.arrayContaining(keys(production)) );//development may add dev-only overrides; it must never MISS one
	} );
} );
