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

	async loadResources(){
		if( !this.#resources ){
			let resources = ( await this.queryArray<Partial<Resource>>( `resources(criteria:null){ id schemaName allowed name deleted target }`));
			this.#resources = new Array<Resource>();
			for( const resource of resources )
				this.#resources.push( new Resource(resource) );
		}
		return this.#resources;
	}
	async getResource( target:string ):Promise<Resource|undefined>{
		let resources:Resource[] = await this.#resources;
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


	#resources!:Resource[];
	#resourceSignal = signal<Resource[]>(new Array<Resource>());
  resources = this.#resourceSignal.asReadonly();
};