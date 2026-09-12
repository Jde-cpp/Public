import { MutationType } from 'jde-framework';
import { ServerCnnctn, ServerCnnctnProps } from './server-cnnctn';

//defaultBrowseNs was rendered as an editable field but neither compared nor sent - an edit silently never saved.
const props = ( overrides:Partial<ServerCnnctnProps>={} )=>({
	id: 7, slug: "local", name: "Local", url: "opc.tcp://127.0.0.1:4840", certificateUri: "urn:x", defaultBrowseNs: 1, server: undefined as any,
	...overrides
} as ServerCnnctnProps);

describe( 'ServerCnnctn.defaultBrowseNs', ()=>{
	it( 'is sent when changed', ()=>{
		const original = new ServerCnnctn( props() );
		const edited = new ServerCnnctn( props({defaultBrowseNs: 2}) );
		const [mutation] = edited.mutation( original );
		expect( mutation.args ).toEqual( {defaultBrowseNs: 2} );
		expect( mutation.type ).toBe( MutationType.Update );
	} );

	it( 'coerces the form input string to a number and compares through it', ()=>{
		const original = new ServerCnnctn( props() );
		const edited = new ServerCnnctn( props({defaultBrowseNs: "2" as any}) );
		expect( edited.defaultBrowseNs ).toBe( 2 );
		expect( edited.equals(original) ).toBe( false );
		const [mutation] = edited.mutation( original );
		expect( mutation.args ).toEqual( {defaultBrowseNs: 2} );
		expect( mutation.toString() ).toContain( 'defaultBrowseNs:2' );
	} );

	it( 'treats a raw unchanged string on the edited copy as equal', ()=>{//Properties.onChange assigns without reconstructing
		const original = new ServerCnnctn( props() );
		const edited = new ServerCnnctn( props() );
		(edited as any).defaultBrowseNs = "1";
		expect( edited.equals(original) ).toBe( true );
		expect( edited.mutation(original) ).toEqual( [] );
	} );

	it( 'falls back to the default when cleared or non-numeric', ()=>{
		expect( new ServerCnnctn(props({defaultBrowseNs: "" as any})).defaultBrowseNs ).toBe( 1 );
		expect( new ServerCnnctn(props({defaultBrowseNs: "abc" as any})).defaultBrowseNs ).toBe( 1 );
		expect( new ServerCnnctn(props({defaultBrowseNs: undefined})).defaultBrowseNs ).toBe( 1 );
	} );
} );

//url is non-null in the gateway meta; the base canSave only knows name/slug, so Save would have lit up for a connection the insert then rejected.
describe( 'ServerCnnctn.canSave', ()=>{
	it( 'needs a url as well as name and slug', ()=>{
		expect( new ServerCnnctn(props()).canSave ).toBe( true );
		expect( new ServerCnnctn(props({url: ""})).canSave ).toBe( false );
		expect( new ServerCnnctn(props({url: undefined as any})).canSave ).toBe( false );
		expect( new ServerCnnctn(props({name: ""})).canSave ).toBe( false );
	} );
} );
