import { Injectable, Inject, signal, OnDestroy } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { IEnvironment } from 'jde-spa';
import { AppService, AuthStore, IGraphQL, TableSchema } from 'jde-framework';
import { Resource } from '../model/Resource';



@Injectable( {providedIn: 'root'} )
export class AccessService extends AppService implements OnDestroy{
	constructor( http: HttpClient, @Inject('IEnvironment') environment: IEnvironment, @Inject("AuthStore") authStore:AuthStore ){
		super( http, environment, authStore );
	}
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

	override targetQuery( schema: TableSchema, target: string, showDeleted:boolean ):string{
		let fields = super.fieldColumns( schema, showDeleted, [] );
		switch( schema.collectionName ){
			case "users":
				fields.push( `groups{id}` );
				break;
			case "groups":
				fields[fields.findIndex(f=>f.startsWith("members"))] = `groupMembers{id isGroup}`; //the server parses/returns groupMembers (group_members view); 'members' is only the introspection field name.
				fields.push( "id" ); //not in the introspected type (the map table has two surrogate keys, so no pk field) but the subQueries need it.
				break;
			case "roles":
				fields.push( ...[`permissionRights{id allowed denied resource{id}}`, `roles{id}`] );
				break;
			default:
				throw new Error( `Unknown table: ${schema.collectionDisplay}` );
		}
		return `${schema.singular}( target:"${target}" ){ ${fields.join(" ")} }`;
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