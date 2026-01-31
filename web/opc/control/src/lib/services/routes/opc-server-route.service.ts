import { inject, Inject, Injectable } from '@angular/core';
import { ActivatedRoute, Routes, UrlSegment } from "@angular/router";
import { DocItem, IRouteService } from "jde-spa";
import { AppService, RouteStore, subscribe } from "jde-framework";
import { GatewayService } from '../gateway.service';

@Injectable( {providedIn: 'root'} )
export class OpcServerRouteService implements IRouteService{
	constructor( @Inject('AppService') private _app:AppService ){}
	async children():Promise<Routes>{
		throw new Error("Not implemented");
	}

	async docItems( urlSegments:UrlSegment[] ):Promise<DocItem[]>{
		let y = [];
		let instances = await this._app.opcServerInstances();
		for( const s of instances )
			y.push( new DocItem({path: s.host, title: s.host}) );

		this.routeStore.setChildren( urlSegments, y );
		return y;
	}

	routeStore = inject( RouteStore );
}