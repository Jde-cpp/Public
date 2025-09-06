import { P } from "@angular/cdk/keycodes";
import { Component, Inject, OnInit, signal } from "@angular/core";
import { ActivatedRoute, Router } from "@angular/router";
import { ComponentCategoryList, DocItem, IRouteService } from "jde-spa";

@Component( {
	templateUrl: './home.html',
	styleUrls: ['./home.scss'],
	imports: [ComponentCategoryList]
})
export class Home implements OnInit {
	constructor( private route: ActivatedRoute, private router: Router, @Inject("IRouteService") private routerService: IRouteService ){
	}
	async ngOnInit(){
		let items = [];
		for( let config of this.router.config.filter(x=> x.data && x.path.length && x.path!="login") )
			items.push( {title: config.title, path: config.path, id: config.path, summary: config.data["pageSettings"]?.summary} );
		this.items.set( items );
	}

	items = signal<DocItem[]>( null );
}