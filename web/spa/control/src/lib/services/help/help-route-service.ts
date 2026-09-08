import { inject, Injectable } from '@angular/core';
import { ActivatedRouteSnapshot, ResolveFn, Routes } from '@angular/router';
import { RouteService } from '../route-service';
import { RouteStore } from '../route-store';
import { RouteItem } from '../../pages/component-sidenav/route-item';
import { HELP_TOPICS, HelpTopic, helpTopics } from './help-topic';

//IRouteService for the /help cards page:  the topics as child routes, so the inherited docItems turns them into tiles.
@Injectable()
export class HelpRouteService extends RouteService{
	#topics = helpTopics( inject(HELP_TOPICS, {optional: true}) );
	override children():Promise<Routes>{
		return Promise.resolve( this.#topics.map( t=>({path: t.id, title: t.title, data: {icon: t.icon, summary: t.summary, cardClass: 'card-help'}}) ) );
	}
}

//Resolves /help/:topic to its HelpTopic - undefined when unknown, which the page reports - and seeds the RouteStore with the
//siblings first, so the breadcrumb reads the topic's title rather than the raw segment on the very first visit;  the
//ql-list resolver does the same for ':collectionDisplay'.
export const helpTopicResolver:ResolveFn<HelpTopic|undefined> = ( route:ActivatedRouteSnapshot )=>{
	const topics = helpTopics( inject(HELP_TOPICS, {optional: true}) );
	inject( RouteStore ).setChildren( 'help', topics.map( t=>new RouteItem({path: t.id, title: t.title, icon: t.icon}) ) );
	return topics.find( t=>t.id==route.paramMap.get('topic') );
};
