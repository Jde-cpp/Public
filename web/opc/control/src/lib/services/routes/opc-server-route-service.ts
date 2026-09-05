import { inject, Injectable } from '@angular/core';
import { ActivatedRoute, Routes, UrlSegment } from "@angular/router";
import { RouteItem, IRouteService, RouteStore } from "jde-spa";
import { APP_SERVICE, AppService, subscribe } from "jde-framework";
import { GatewayService } from '../gateway-service';

@Injectable( {providedIn: 'root'} )
export class OpcServerRouteService implements IRouteService{
	private _app:AppService = inject( APP_SERVICE );
	async children():Promise<Routes>{
		throw new Error("Not implemented");
	}

	async docItems( urlSegments:UrlSegment[] ):Promise<RouteItem[]>{
		let y = [];
		let instances = await this._app.opcServerInstances();
		for( const s of instances )
			y.push( new RouteItem({path: s.host, title: s.host}) );

		this.routeStore.setChildren( urlSegments, y );
		return y;
	}

	routeStore = inject( RouteStore );
}