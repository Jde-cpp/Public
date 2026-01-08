import {inject, Inject, Injectable} from '@angular/core';
import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, RouterStateSnapshot} from '@angular/router';
import { DocItem } from 'jde-spa';
import { IErrorService } from '../error/IErrorService';
import { AppService } from '../app/app.service';
import { RouteStore } from '../route.store';
import { StringUtils } from '../../utils/StringUtils';


export type Connection = { id: number, programName: string, instanceName: string, hostName: string, created: Date, status: { memory: number, values: any[] }, urlSegments:string[] };

export class AppInstanceRoute extends DocItem{
	constructor( programName:string, instanceName:string ){
		super();
		this.path = `${programName}/${instanceName}`;
		this.title = `${programName}/${instanceName}`;
	}
}

@Injectable()
export class AppResolver implements Resolve<Connection[]> {
	constructor( @Inject("AppService") private appService: AppService, @Inject('IProfile') @Inject('IErrorService') private cnsl: IErrorService )
	{}
	async resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<Connection[]>{
		let connections = await this.appService.queryArray<Connection>( "connections{id programName instanceName hostName created status{memory values}}", (m)=>console.log(m) );
		let urlMap = {};
		connections.forEach( c=>{
			c.created = new Date( c.created+'Z' );
			c.programName = c.programName.startsWith("Jde.") ? c.programName.substring(4) : c.programName;
			if( c.programName=="OpcGateway" )
				c.programName = "Gateway";
			let childPath = StringUtils.toJson(StringUtils.plural(c.programName));
			c.urlSegments = [ childPath, c.instanceName];
			if( !urlMap[childPath] )
				urlMap[childPath] = [];
			urlMap[childPath].push( new DocItem({ path: c.urlSegments.join('/'), title: `${c.instanceName}` }) );
		});
//		let parent = new DocItem( { path: route.url.map( s=>s.path ).join('/') } );
		Object.keys(urlMap).forEach( key=>{
			this.routeStore.setChildren( 'apps/'+key, urlMap[key] );
		} );
		return connections;
	}

	routeStore = inject( RouteStore );
}
