import { Injectable, Inject, signal, OnDestroy } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { IEnvironment } from 'jde-material';
import { AppService, AuthStore, TableSchema } from 'jde-framework';
import { Resource } from '../model/Resource';



@Injectable( {providedIn: 'root'} )
export class AccessService extends AppService implements OnDestroy{
	constructor( http: HttpClient, @Inject('IEnvironment') environment: IEnvironment, @Inject("AuthStore") authStore:AuthStore ){
		super( http, environment, authStore );
	}
	ngOnDestroy(): void {
		console.log( 'AccessService.ngOnDestroy' );
	}

	#resources:Resource[];
	async loadResources(){
		if( !this.#resources ){
			let resources = ( await this.query( `resources(criteria:null){ id schemaName allowed name deleted target }`))["resources"];
			this.#resources = new Array<Resource>();
			for( const resource of resources )
				this.#resources.push( new Resource(resource) );
		}
		return this.#resources;
	}
	async getResource( target:string ):Promise<Resource>{
		let resources = await this.#resources;
		return resources.find( r=>r.target==target );
	}

	override targetQuery( schema: TableSchema, target: string, showDeleted:boolean ):string{
		let fields = super.fieldColumns( schema, showDeleted );
		switch( schema.collectionName ){
			case "users":
				fields.push( `groupings{id}` );
				break;
			case "groupings":
				fields.push( `members{id isGroup}` );
				break;
			case "roles":
				fields.push( ...[`permissionRights{id allowed denied resource{id}}`, `roles{id}`] );
				break;
			default:
				debugger;
				throw new Error( `Unknown table: ${schema.collectionDisplay}` );
		}
		return `${schema.singular}( target:"${target}" ){ ${fields.join(" ")} }`;
	}
	override subQueries( typeName: string, id: number ):string[]{
		let queries = [];
		switch( typeName ){
		case "User":
		case "Grouping":
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
	override excludedColumns(collectionName: string): string[] {
		let columns = [];
		switch( collectionName ){
			case "users": columns = ["isGroup"]; break;
			case "roles": columns = ["permissions"]; break;
			case "groups": columns = ["isGroup", "members"]; break;
		}
		return columns;
	}
	override toCollectionName( collectionDisplay:string ):string{
		return collectionDisplay=="groups" ? "groupings" : collectionDisplay;
	}


	#resourceSignal = signal<Resource[]>(new Array<Resource>());
  resources = this.#resourceSignal.asReadonly();
};