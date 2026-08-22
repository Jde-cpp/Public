import { HttpClient } from '@angular/common/http';
import { ProtoService, ETransport, IError } from './proto.service';
import { AuthStore } from './auth.store';

class TestService extends ProtoService<object,object>{
	constructor( authStore:AuthStore ){ super( {create:()=>({}), encode:()=>({finish:()=>new Uint8Array()})}, {} as HttpClient, ETransport.Unsecure, authStore ); }//codec object per the ts-proto refactor - the abstract encode() hook is gone.
	protected override processMessage():void{}
	protected override handleConnectionError():void{}
	register( requestId:number ):Promise<unknown>{ return new Promise( (resolve,reject)=>this._callbacks.set(requestId, {resolve, reject} as any) ); }
}

describe('ProtoService.processError', () => {
	const authStore = { logout: vi.fn() };
	const service = new TestService( authStore as unknown as AuthStore );

	beforeEach( () => authStore.logout.mockClear() );

	it('maps the wire exception into the rejection, statusCode as httpStatus', async () => {
		const p = service.register( 7 );
		expect( service.processError( {what:'denied', code:0xabc, statusCode:403} as any, 7 ) ).toBe( true );
		const e = await p.catch( x=>x ) as {error:IError};
		expect( e.error ).toEqual( {requestId:7, message:'denied', sc:0xabc, httpStatus:403} );
		expect( authStore.logout ).not.toHaveBeenCalled();
	});

	it('a 401 logs the user out even when unhandled, same policy as the http path', () => {
		expect( service.processError( {what:'expired', code:0, statusCode:401} as any, 8 ) ).toBe( false );//no callback registered - the credential is stale regardless.
		expect( authStore.logout ).toHaveBeenCalled();
	});

	//access-review3 #17: a denial is 403 now, not 401 - the server used to answer 401 for "lacks Administer" and this policy
	//logged the user out.  The complement of the case above: a 403 is a permission error, handled or not, never a logout.
	it('a 403 never logs the user out, even when unhandled', () => {
		expect( service.processError( {what:'denied', code:0, statusCode:403} as any, 9 ) ).toBe( false );//no callback registered.
		expect( authStore.logout ).not.toHaveBeenCalled();
	});
});
