if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { TestBed } from '@angular/core/testing';
import { provideHttpClient } from '@angular/common/http';
import { HttpClient } from '@angular/common/http';
import * as FromClient from 'jde-proto/App.FromClient';
import * as App from 'jde-proto/App';
import { AuthStore } from '../auth-store';
import { ETransport, RequestId } from '../proto-service';
import { AppService } from './app-service';

//review3 L10: the send payloads are plain objects spread into a FromClient.Message, so a key the proto does not declare is
//silently dropped by encode() and a requestId-only message goes on the wire - the server has nothing to answer and the
//promise never settles.  `strings` was such a key; the field is `request_strings`.
class TestAppService extends AppService{
	sent:any[] = [];
	//capture what send() hands the wire.  Going through the real path would open a socket: with none connected sendWithId
	//backlogs and calls connect(), so the encode below is done here instead - it is the step the bug was lost in.
	protected override sendWithId( m:any, requestId:RequestId, _log:string ):void{ this.sent.push( {requestId, ...m} ); }
	wire( index=0 ):FromClient.Message{
		return FromClient.Message.decode( FromClient.Message.encode(FromClient.Message.fromPartial(this.sent[index])).finish() );
	}
}

const create = ( env:Record<string,any> = {} )=>{
	TestBed.resetTestingModule();
	TestBed.configureTestingModule({ providers: [ provideHttpClient() ]});
	const values = { applicationServer: {host:'localhost', port:1967}, ...env };
	return TestBed.runInInjectionContext( ()=>new TestAppService(
		TestBed.inject( HttpClient ),
		{ get: ( key:string )=>(<any>values)[key] } as any,
		{ user: ()=>undefined, logout: ()=>{} } as unknown as AuthStore
	) );
};

describe( 'AppService socket message field names', ()=>{

	const md5 = new Uint8Array( 16 ).fill( 7 );

	it( 'puts the StringMD5s payload on the wire', ()=>{
		const service = create();
		service.requestStrings( App.StringMD5s.fromPartial({messages:[md5], files:[], functions:[], userPKs:[42]}) );
		const message = service.wire();
		expect( message.requestStrings?.messages ).toEqual( [md5] );
		expect( message.requestStrings?.userPKs ).toEqual( [42] );
	} );

	it( 'puts the unsubscribe request type on the wire', ()=>{
		const service = create();
		service.logsUnsubscribe( 9 );
		const message = service.wire();
		expect( message.requestType ).toBe( FromClient.ERequestType.UnsubscribeLogs );
		expect( message.requestId ).toBe( 9 );
	} );
} );

//review3 L11: neither environment file carried an `httpTransport` key, so this was constructed with `undefined`.  Every
//test in ProtoService is `==Secure`/`==Hybrid`, which undefined fails, so it acted as Unsecure - but `==ETransport.Unsecure`
//was false too, that enumerator being 0.
describe( 'AppService transport', ()=>{
	it( 'is a real enumerator when the environment names none', ()=>{
		expect( create().transport ).toBe( ETransport.Unsecure );
	} );

	it( 'honours the environment key', ()=>{
		expect( create({httpTransport: ETransport.Secure}).transport ).toBe( ETransport.Secure );
	} );
} );
