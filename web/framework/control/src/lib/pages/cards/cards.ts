import { CommonModule } from "@angular/common";
import { Component, Inject, Injectable, OnInit, signal } from "@angular/core";
import { ActivatedRoute, Router, RouterLink, Routes, UrlSegment } from "@angular/router";
import { MatIconModule } from "@angular/material/icon";
import { ComponentCategoryList, RouteItem, IRouteService, RouteService } from "jde-spa";


@Injectable( {providedIn: 'root'} )
export class HomeRouteService extends RouteService{
	constructor( protected override route: ActivatedRoute, private router: Router ){
		super( route );
	}
	override children():Promise<Routes>{
		let y:Routes = [];
		for( let config of this.router.config.filter(x=> x.title && x.path!.length && x.path!="login" && !x.path!.includes('/')) ){
			let pageSettings = config.data ? config.data["pageSettings"] : null;
			y.push( {title: config.title, path: config.path, data:{ id: config.path, summary: pageSettings?.summary }} );
		}
		return Promise.resolve( y );
	}
}

@Component( {
	templateUrl: './cards.html',
	styleUrls: ['./cards.scss'],
	imports: [CommonModule, MatIconModule, RouterLink]
})
export class Cards implements OnInit {
	constructor( private route: ActivatedRoute, private router: Router, @Inject("IRouteService") private routerService: IRouteService )
	{}
	ngOnInit(){
		this.route.url.subscribe( async (urlSegments)=>{
			let items = await this.routerService.docItems( urlSegments );
			this.items.set( items.filter((x)=>x.path.length && x.path!="login") );
		});
	}
	items = signal<RouteItem[]>( null as any );
}