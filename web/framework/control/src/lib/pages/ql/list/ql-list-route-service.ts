import { Injectable } from "@angular/core";
import { ActivatedRoute, Route, Router, Routes, UrlSegment } from "@angular/router";
import { IRouteService, RouteService } from "jde-spa";

@Injectable( {providedIn: 'root'} )
export class QLListRouteService extends RouteService implements IRouteService{
	constructor( private router:Router, route: ActivatedRoute ){
		super( route )
	}
	override children( urlSegments:UrlSegment[] ):Promise<Routes>{
		let y:Routes = [];
		let thisConfig = this.router.config.find( x=>x.path==urlSegments[urlSegments.length-1].path );
		let childrenConfig = this.router.config.find( x=>x.path==thisConfig.path && x.children?.length );
		for( let child of childrenConfig.children?.filter(x=> !x.path.endsWith(":target")) ){
			if( child.path!=":collectionDisplay" )
				y.push( child );
			else{
				for( let collection of child.data["collections"] ){
					var route:Route;
					if( typeof collection=='string' ){
						route = {
							title: collection.charAt( 0 ).toUpperCase()+collection.slice(1),
							data: {id: child.path.replace(":collectionDisplay", collection), collectionName:collection},
							path: collection };
					}else{
						const data = collection.data;
						const path = collection.path ?? data.collectionName;
						const upper = path.charAt( 0 ).toUpperCase()+path.slice( 1 );
						route = {
							title: collection.title ?? upper,
							data: data,
							path: child.path.replace(":collectionDisplay", path),
						};
					}
					y.push( route );
				}
			}
		}
		return Promise.resolve( y );
	}
}
