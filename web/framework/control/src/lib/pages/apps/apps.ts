import {Component, OnDestroy, OnInit, Inject, inject} from '@angular/core';
import { DatePipe } from '@angular/common';
import {ActivatedRoute} from '@angular/router';
import { RouterLink } from '@angular/router';
import { MatCardModule } from '@angular/material/card';
import { MatIconModule } from '@angular/material/icon';
import { RouteItem, RouteStore } from 'jde-spa';
import { Connection } from '../../services/resolvers/app-resolver';
import { pageHeading } from '../cards/cards';

@Component({
    selector: 'apps',
    templateUrl: './apps.html',
    styleUrls: ['./apps.scss'],
    imports: [DatePipe, MatCardModule, MatIconModule, RouterLink]
})
export class Apps implements OnInit{
	constructor( private route: ActivatedRoute ){
	}

	async ngOnInit(){
		this.heading = pageHeading( this.route );
		this.route.data.subscribe( (data)=>{
			this.connections = data["connections"];
			this.routeStore.setChildren( '/apps', this.connections.map( c=>new RouteItem({ path: c.urlSegments.join('/'), title: `${c.programName}/${c.instanceName}` }) ) );
    });
	}
	//the gateway tiles on /gateways use router too, so the same service reads the same in both places
	icon( programName:string ):string{
		switch( programName ){
			case "Gateway": return "router";
			case "AppServer": return "dns";
			case "OpcServer": return "sensors";//radiating signal - an opc server publishes live tag data
		}
		return "apps";
	}
	//the start timestamp is the tooltip; "up 3h 12m" is what the list is scanned for
	uptime( created:Date ):string{
		const seconds = Math.max( 0, Math.floor((Date.now()-created.getTime())/1000) );
		const days = Math.floor( seconds/86400 ), hours = Math.floor( seconds%86400/3600 ), minutes = Math.floor( seconds%3600/60 );
		if( days ) return `${days}d ${hours}h`;
		if( hours ) return `${hours}h ${minutes}m`;
		if( minutes ) return `${minutes}m`;
		return `${seconds}s`;
	}
	formatMemory(memory:number):string{
		if(memory < 1024) return memory + ' B';
		else if(memory < 1024*1024) return (memory/1024).toFixed(0) + ' KB';
		else if(memory < 1024*1024*1024) return (memory/(1024*1024)).toFixed(0) + ' MB';
		else return (memory/(1024*1024*1024)).toFixed(2) + ' GB';
	}

	connections:Connection[]=[];
	heading = "";
	routeStore = inject( RouteStore );
}
