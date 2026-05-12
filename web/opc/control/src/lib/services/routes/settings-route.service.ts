import { inject, Inject, Injectable } from '@angular/core';
import { ActivatedRoute, Router, Routes, UrlSegment } from "@angular/router";
import { RouteItem, RouteService } from "jde-spa";

@Injectable( {providedIn: 'root'} )
export class SettingsRouteService extends RouteService{
	constructor( protected override route: ActivatedRoute, private router: Router){
		super( route )
	}

	override children( urlSegments:UrlSegment[] ):Promise<Routes>{
		let y: Routes = [];
		for( let config of this.router.config.filter(x => x.path.startsWith('settings/') && !x.path.includes(':')) ){
			let pageSettings = config.data ? config.data["pageSettings"] : null;
			y.push( {title: config.title, path: config.path.substring(9), data:{ id: config.path, summary: pageSettings?.summary }} );
	}
		return Promise.resolve( y );
	}
	// override async children():Promise<Routes>{
	// 	throw new Error("Not implemented");
	// }

	// override docItems( urlSegments:UrlSegment[] ):Promise<RouteItem[]>{
	// 	let y = [];
	// 	for( const s of this._iot.instances )
	// 		y.push( {id: s.host, path: '/gatewayClients/'+s.host, title: s.host} );

	// 	return Promise.resolve(y);
	// }
}