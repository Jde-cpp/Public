import { HttpClient } from '@angular/common/http';
import { ProtoService, ETransport, IError } from './proto-service';
import { AuthStore } from './auth-store';

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

//angular-review3 #6:  the handshake message is the subclass's to build, because the protos disagree - Opc.FromClient
//declares `string session_id`, App.FromClient `uint32 session_id`.  The base used to hard-code the authorization string
//into the field, which threw @bufbuild's assertUInt32 on the App socket;  the catch released the backlog anyway, so the
//socket ran unauthenticated for the rest of its life.
class HandshakeService extends ProtoService<any,any>{
	constructor( authStore:AuthStore, private message?:( a:string )=>any ){
		super( {create:()=>({messages:[]}), encode:()=>({finish:()=>new Uint8Array()})}, {} as HttpClient, ETransport.Unsecure, authStore );
	}
	override connect():void{ this.connects++; }//no websocket in a unit test
	protected override processMessage():void{}
	protected override handleConnectionError():void{}
	protected override authorizationMessage( authorization:string ):any|undefined{ return this.message ? this.message( authorization ) : super.authorizationMessage( authorization ); }
	handshake( socketId:number ):Promise<void>{ return this.sendAuthorization( socketId ); }
	get sent():any[]{ return this.backlog.flatMap( (t:any)=>t.messages ); }
	get id():number{ return this.socketId; }
	connects = 0;
}
const storeFor = ( authorization:string|null )=>({ user: ()=>({authorization}), logout: vi.fn() }) as unknown as AuthStore;

describe('ProtoService.sendAuthorization', () => {
	//not awaited:  sendPromise settles on the server's reply, and the socketId is released from that continuation.
	it('sends what authorizationMessage builds - the base keeps the string form Opc.FromClient declares', () => {
		const service = new HandshakeService( storeFor("1a2b") );
		service.handshake( 42 );
		expect( service.sent ).toEqual( [{requestId: 1, sessionId: "1a2b"}] );
		expect( service.id ).toBeFalsy();//the handshake goes out BEFORE the socketId releases the rest of the backlog
	});

	it('lets an override change the field - AppService hands over a uint32', () => {
		const service = new HandshakeService( storeFor("1a2b"), a=>({sessionId: parseInt(a,16)}) );
		service.handshake( 42 );
		expect( service.sent ).toEqual( [{requestId: 1, sessionId: 0x1a2b}] );
	});

	it('sends nothing when the credential cannot authenticate the socket, and still releases the backlog', async () => {
		const service = new HandshakeService( storeFor("Bearer ey.j.s"), ()=>undefined );
		await service.handshake( 7 );
		expect( service.sent ).toEqual( [] );
		expect( service.id ).toBe( 7 );
	});

	it('sends nothing when there is no authorization at all', async () => {
		const service = new HandshakeService( storeFor(null) );
		await service.handshake( 9 );
		expect( service.sent ).toEqual( [] );
		expect( service.id ).toBe( 9 );
	});
});

//review3 L12: sendTransmission dumped every outgoing transmission with an unconditional console.log - so the handshake
//printed the session id, the credential the socket authenticates with, past the log.sockRequests gate the rest of the
//class honours.
describe( 'ProtoService.sendTransmission logging', ()=>{
	class LogService extends ProtoService<any,object>{
		constructor(){ super( {create:()=>({}), encode:()=>({finish:()=>new Uint8Array()})}, {} as HttpClient, ETransport.Unsecure, {user:()=>undefined, logout:()=>{}} as unknown as AuthStore ); }
		protected override processMessage():void{}
		protected override handleConnectionError():void{}
		setLog( sockRequests:boolean ){ this.log.sockRequests = sockRequests; }
	}
	const handshake = { messages: [{requestId: 1, sessionId: "deadbeefcafe"}, {requestId: 2, jwt: "ey.token.sig"}] };
	let logged:string[];
	let spy:any;

	beforeEach( ()=>{
		logged = [];
		spy = vi.spyOn( console, 'log' ).mockImplementation( (m:any)=>{ logged.push(String(m)); } );
	} );
	afterEach( ()=>spy.mockRestore() );

	it( 'never puts the session id or the jwt on the console', ()=>{
		new LogService().sendTransmission( handshake );
		expect( logged.join('\n') ).not.toContain( "deadbeefcafe" );
		expect( logged.join('\n') ).not.toContain( "ey.token.sig" );
		expect( logged.join('\n') ).toContain( "<redacted>" );
	} );

	it( 'honours log.sockRequests', ()=>{
		const service = new LogService();
		service.setLog( false );
		service.sendTransmission( handshake );
		expect( logged ).toEqual( [] );
	} );
} );
