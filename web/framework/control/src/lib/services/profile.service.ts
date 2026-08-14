import { Service, Signal, computed, inject } from '@angular/core';
import { IProfileService } from 'jde-spa';
import { AppService } from './app/app.service';
import { Mutation, MutationType } from '../model/ql/Mutation';
import { StringUtils } from '../utils/StringUtils';

//Persists ProfileStore blobs to the AppServer's access_ui_profiles table; the server scopes rows to the executer.
@Service()
export class ProfileService implements IProfileService{
	#app = inject( AppService );
	userKey:Signal<string|undefined> = computed( ()=>{
		const u = this.#app.user();//same logged-in predicate as ProtoService.isLoggedIn.
		return u && (u.jwt || u.id) ? (u.id ?? u.email) : undefined;
	} );
	async load( key:string ):Promise<string|null>{
		const row = await this.#app.querySingle<{value:string}|null>( `uiProfile( target:${StringUtils.qlString(key)} ){ value }` );
		return row?.value ?? null;
	}
	async save( key:string, value:string|null ):Promise<void>{//value null deletes the row.
		await this.#app.mutate( new Mutation('uiProfile', 0, {target:key, value:value}, MutationType.Update) );
	}
}
