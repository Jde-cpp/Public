import { TestBed } from '@angular/core/testing';
import { AccessService } from './access-service';
import { AccessSearchProvider } from './access-search-provider';

const rows:Record<string,{id:number,name:string,target:string}[]> = {
	users: [ {id:1, name:"John Duffy", target:"Google-johnmduffy@gmail.com"}, {id:2, name:"alice", target:"alice"} ],
	groups: [ {id:3, name:"ops", target:"ops"} ],
	roles: [ {id:4, name:"admin", target:"admin"} ]
};
const access = {
	queryArray: ( ql:string )=>Promise.resolve( rows[ql.split('{')[0].trim()] ?? [] ),
	loadResources: ()=>Promise.resolve( [ {id:5, name:"nodes", target:"nodeIds", schema:"opc.nodes"} ] )
};

describe( 'AccessSearchProvider', ()=>{
	let provider:AccessSearchProvider;
	beforeEach( ()=>{
		TestBed.configureTestingModule({ providers: [ {provide: AccessService, useValue: access} ] });
		provider = TestBed.inject( AccessSearchProvider );
		provider.invalidate();
	} );

	//angular-review3 #7: the route was pre-encoded and then handed to router.navigate, which encodes each command again -
	//'@' went out as '%2540', paramMap yielded '…%40…', and no Google-provisioned user resolved.  Raw segments, as
	//NodeSearchProvider has always done.
	it( 'hands the router raw segments, not a pre-encoded string', async ()=>{
		const [hit] = await provider.search( 'john', 'user', 10 );//SearchService lowercases the query; searchRank only lowercases the title
		expect( hit.route ).toEqual( ['/access', 'users', 'Google-johnmduffy@gmail.com'] );
	} );

	it( 'routes groups and roles the same way', async ()=>{
		expect( (await provider.search('ops', 'group', 10))[0].route ).toEqual( ['/access', 'groups', 'ops'] );
		expect( (await provider.search('admin', 'role', 10))[0].route ).toEqual( ['/access', 'roles', 'admin'] );
	} );

	//resources have no detail page - the hit opens the list, and that url carries no user data.
	it( 'sends a resource hit to the list as a plain url', async ()=>{
		expect( (await provider.search('nodes', 'resource', 10))[0].route ).toBe( '/access/resources' );
	} );
} );
