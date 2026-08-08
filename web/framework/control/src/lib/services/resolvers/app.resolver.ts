import {inject, Inject, Injectable} from '@angular/core';
import {ActivatedRouteSnapshot, Resolve, RouterStateSnapshot} from '@angular/router';
import { RouteItem } from 'jde-spa';
import { AppService } from '../app/app.service';
import { RouteStore } from '../route.store';
import { StringUtils } from '../../utils/StringUtils';
import { TableSettings } from '../ql-list.resolver';


export type Connection = { id: number, instanceId: number, programName: string, instanceName: string, hostName: string, created: Date, status: { memory: number, values: any[] }, urlSegments:string[] };

export class AppInstanceRoute extends RouteItem{
	constructor( programName:string, instanceName:string, tableSettings:TableSettings ){
		super();
		this.path = `${programName}/${instanceName}`;
		this.title = `${programName}/${instanceName}`;
		this.tableSettings = tableSettings;
	}
	tableSettings:TableSettings;
}

@Injectable()
export class AppResolver implements Resolve<Connection[]> {
	constructor( @Inject("AppService") private appService: AppService )
	{}
	async resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<Connection[]>{
		let connections = await this.appService.queryArray<Connection>( "connections{id instanceId programName instanceName hostName created status{memory values}}", null, (m)=>console.log(m) );
		let urlMap:any = {};
		connections.forEach( c=>{
			c.created = new Date( c.created );
			c.programName = c.programName.startsWith("Jde.") ? c.programName.substring(4) : c.programName;
			if( c.programName=="OpcGateway" )
				c.programName = "Gateway";
			let childPath = StringUtils.toJson(StringUtils.plural(c.programName));
			c.urlSegments = [ childPath, c.instanceName];
			if( !urlMap[childPath] )
				urlMap[childPath] = [];
			urlMap[childPath].push( new RouteItem({ path: c.urlSegments.join('/'), title: `${c.instanceName}` }) );
		});
		Object.keys(urlMap).forEach( key=>{
			this.routeStore.setChildren( 'apps/'+key, urlMap[key] );
		} );
		return connections;
	}

	routeStore = inject( RouteStore );
}
