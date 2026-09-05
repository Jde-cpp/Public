import { Injectable, InjectionToken, signal, OnDestroy } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { IEnvironment } from 'jde-spa';
import { AppService, AuthStore, IGraphQL, StringUtils, TableSchema } from 'jde-framework';
import { Resource } from '../model/resource';



@Injectable( {providedIn: 'root'} )
export class AccessService extends AppService implements OnDestroy{
	ngOnDestroy(): void {
		console.log( 'AccessService.ngOnDestroy' );
	}

	//Cache the PROMISE, not the array.  The array was only assigned after the await, so two callers racing the first load
	//(every permission tab builds its own PermissionTable) both saw an unset #resources and both queried.
	async loadResources():Promise<Resource[]>{
		return this.#resources ??= this.#queryResources().catch( (e)=>{ this.#resources = undefined; throw e; } );//never cache a failure: a rejected promise would be handed to every later caller
	}
	async #queryResources():Promise<Resource[]>{
		const resources = (await this.queryArray<Partial<Resource>>( `resources(criteria:null){ id schemaName allowed name deleted target }` )).map( (r)=>new Resource(r) );
		this.#resourceSignal.set( resources );//the public `resources` signal was never written, so it read empty forever
		return resources;
	}
	async getResource( target:string ):Promise<Resource|undefined>{
		const resources = await this.loadResources();//was `await this.#resources` - the field, not the loader, so this threw on `.find` unless something else had already loaded
		return resources.find( r=>r.target==target );
	}

	//excludedColumns is the collection's tableSettings list, forwarded by DetailResolver.  Hardcoding [] here dropped it:
	//Role's introspected 'permissions' is the extends-base link (roles.role_id -> permissions.permission_id) and the
	//permissions table has no name column, so the generated `permissions{id name}` failed the whole query with a 500.
	override targetQuery( schema: TableSchema, target: string, showDeleted:boolean, excludedColumns:string[]=[] ):string{
		let fields = super.fieldColumns( schema, showDeleted, excludedColumns );
		switch( schema.collectionName ){
			case "users":
				fields.push( `groups{id}` );
				break;
			case "groups":
				//the server parses/returns groupMembers (group_members view); 'members' is only the introspection field name.
				//It is in groupTableSettings.excludedColumns, so fieldColumns may not have emitted it at all - drop whatever
				//is there and append, rather than assigning over a findIndex of -1.
				fields = fields.filter( f=>!f.startsWith("members") );
				fields.push( `groupMembers{id isGroup}` );
				fields.push( "id" ); //not in the introspected type (the map table has two surrogate keys, so no pk field) but the subQueries need it.
				break;
			case "roles":
				fields.push( ...[`permissionRights{id allowed denied resource{id}}`, `roles{id}`] );
				break;
			default:
				throw new Error( `Unknown table: ${schema.collectionDisplay}` );
		}
		return `${schema.singular}( target:${StringUtils.qlString(target)} ){ ${fields.join(" ")} }`;
	}
	override subQueries( typeName: string, id: number ):string[]{
		let queries = new Array<string>();
		switch( typeName ){
		case "User":
		case "Group":
			queries = [
				`acl( identityId:${id} ){ permissionRights{id allowed denied resource{id deleted}} }`,
				`acl( identityId:${id} ){ role{id deleted} }`
			];
		break;
		case "Role":
			queries = [
				`acl( permissionId:${id} ){ identities{id isGroup} }`,
			];
		break;
		}
		return queries;
	}

	override toCollectionName( collectionDisplay:string ):string{
		return collectionDisplay;
	}


	#resources:Promise<Resource[]>|undefined;
	#resourceSignal = signal<Resource[]>(new Array<Resource>());
  resources = this.#resourceSignal.asReadonly();
};
//angular-review3 C13: a typed token in place of the string one - a typo now fails the build instead of resolving to nothing at runtime, and inject() can take it.
export const ACCESS_SERVICE = new InjectionToken<AccessService>( 'AccessService' );
