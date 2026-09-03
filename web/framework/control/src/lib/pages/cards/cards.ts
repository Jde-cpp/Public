import { inject } from '@angular/core';
import { Component, Inject, Injectable, OnInit, signal } from "@angular/core";
import { ActivatedRoute, Router, RouterLink, Routes, UrlSegment } from "@angular/router";
import { MatIconModule } from "@angular/material/icon";
import { RouteItem, IRouteService, RouteService } from "jde-spa";


@Injectable( {providedIn: 'root'} )
export class HomeRouteService extends RouteService{
	private router:Router = inject( Router );
	override children():Promise<Routes>{
		let y:Routes = [];
		for( let config of this.router.config.filter(x=> x.title && x.path!.length && x.path!="login" && !x.path!.includes('/')) ){
			const data = config.data ?? {};
			const pageSettings = data["pageSettings"];//the app routes set summary/icon directly; pageSettings is the older shape
			//cards.scss colors the tile per section; the route path is the section name (gateways/access/apps)
			y.push( {title: config.title, path: config.path, data:{ id: config.path, icon: data["icon"] ?? pageSettings?.icon, summary: data["summary"] ?? pageSettings?.summary, cardClass: data["cardClass"] ?? `card-${config.path}` }} );
		}
		return Promise.resolve( y );
	}
}

//the route summary names the page ("Available Gateways"); the title is the fallback, unless it is an unsubstituted parameter like ":gateway"
export function pageHeading( route:ActivatedRoute ):string{
	const summary = <string|undefined>route.snapshot.data["summary"];
	const title = <string|undefined>route.snapshot.title;
	return summary ?? (title?.startsWith(":") ? "" : title ?? "");
}

@Component( {
	templateUrl: './cards.html',
	styleUrls: ['./cards.scss'],
	imports: [MatIconModule, RouterLink]
})
export class Cards implements OnInit {
	private route:ActivatedRoute = inject( ActivatedRoute );
	private router:Router = inject( Router );
	constructor( @Inject("IRouteService") private routerService: IRouteService )
	{}
	ngOnInit(){
		this.route.url.subscribe( async (urlSegments)=>{
			this.heading.set( pageHeading(this.route) );
			let items = await this.routerService.docItems( urlSegments );
			this.items.set( items.filter((x)=>x.path.length && x.path!="login") );
		});
	}
	heading = signal<string>( "" );
	items = signal<RouteItem[]>( [] );
}
