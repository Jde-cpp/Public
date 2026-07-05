import { Injectable } from '@angular/core';
import { ActivatedRoute, Router, Routes, UrlSegment } from "@angular/router";
import { RouteItem } from '../pages/component-sidenav/component-sidenav';


export interface IRouteService{
	children( x:UrlSegment[] ):Promise<Routes>;
	docItems( x:UrlSegment[] ):Promise<RouteItem[]>;
}

@Injectable()
export class RouteService implements IRouteService{
	constructor( protected route: ActivatedRoute )
	{}
	children( urlSegments:UrlSegment[] ):Promise<Routes>{
		let config = this.route.parent?.routeConfig! ?? this.route.routeConfig;
		return Promise.resolve( config.children! );
	}
	async docItems( urlSegments:UrlSegment[] ):Promise<RouteItem[]>{
		const children = await this.children(urlSegments);
		let items = [];
		for( let child of children ){
			var docItem = <RouteItem>{...child.data, path:child.path, title: <string>child.title };
			// if( !docItem.path )
			// 	docItem.path = child.path;
			if( child.path!.length )
				items.push( docItem );
		}
		return items;
	}
}