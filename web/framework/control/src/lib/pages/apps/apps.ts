import {Component, OnDestroy, OnInit, Inject, inject} from '@angular/core';
import { DatePipe } from '@angular/common';
import {ActivatedRoute} from '@angular/router';
import { RouterLink } from '@angular/router';
//import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import { MatCardModule } from '@angular/material/card';
import { Connection } from '../../services/resolvers/app.resolver';
import { StringUtils } from '../../utils/StringUtils';
import { RouteStore } from '../../services/route.store';
import { DocItem } from '../../../../../jde-spa/src/lib/pages/component-sidenav/component-sidenav';

@Component({
    selector: 'apps',
    templateUrl: './apps.html',
    styleUrls: ['./apps.scss'],
    imports: [DatePipe, MatCardModule, RouterLink]
})
export class Apps implements OnInit{
	constructor( private route: ActivatedRoute ){
	}

	async ngOnInit(){
		this.route.data.subscribe( (data)=>{
			this.connections = data["connections"];
			this.routeStore.setChildren( '/apps', this.connections.map( c=>new DocItem({ path: c.urlSegments.join('/'), title: `${c.programName}/${c.instanceName}` }) ) );
    });
	}
	formatMemory(memory:number):string{
		if(memory < 1024) return memory + ' B';
		else if(memory < 1024*1024) return (memory/1024).toFixed(0) + ' KB';
		else if(memory < 1024*1024*1024) return (memory/(1024*1024)).toFixed(0) + ' MB';
		else return (memory/(1024*1024*1024)).toFixed(2) + ' GB';
	}

	connections:Connection[]=[];
	routeStore = inject( RouteStore );
}
