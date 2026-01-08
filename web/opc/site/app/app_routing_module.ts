import { NgModule } from '@angular/core';
import {Routes, ROUTES, RouterModule} from '@angular/router';
import { ComponentSidenav } from 'jde-spa';

import{ DetailResolver, Cards, LoginPageComponent, QLList, QLListResolver, QLListRouteService, HomeRouteService, Apps, AppResolver } from 'jde-framework';
import { AccessService, AuthGuard, Group, GroupDetail, Role, RoleDetail, User, UserDetail } from 'jde-access';
import{ CnnctnDetailResolver, GatewayDetail, GatewayRouteService, GatewayCnnctnRouteService,GatewayService, NodeDetail, NodeResolver, OpcNodeRouteService, OpcServerRouteService, SettingsRouteService, GatewayResolver, ClientDetail } from 'jde-opc';


const accessProvider = { provide: 'IGraphQL', useClass: AccessService };
const gatewayProvider = { provide: 'IGraphQL', useClass: GatewayService };
const qlListProvider = { provide: 'IRouteService', useClass: QLListRouteService };
const opcNodeRouteProvider = { provide: 'IRouteService', useClass: OpcNodeRouteService };;

export const routes: Routes = [
	{ path: '', title: "Home", component: Cards, data: {summary: "Welcome" },
		canActivate: [AuthGuard],
		providers: [  {provide: 'IRouteService', useClass: HomeRouteService} ]},
	{ path: 'login', component: LoginPageComponent, data: {name: "Login", summary: "Login to Site"} },
	{ path: 'gateways', title: "Gateways", canActivate: [AuthGuard], component: Cards,
		providers: [{provide: 'IRouteService', useClass: GatewayRouteService}],
		data: {summary: "Available Gateways",}
	},
	{ path: 'gateways/:gateway', title: ":gateway", canActivate: [AuthGuard], component: Cards,
		providers: [{provide: 'IRouteService', useClass: GatewayCnnctnRouteService}],
		data: {summary: "Available Connections",}
	},
	{
		path: 'gateways/:gateway/:connection', component: ComponentSidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '**',
				component: NodeDetail,
				providers: [ NodeResolver, opcNodeRouteProvider ],
				canActivate: [AuthGuard],
				data: { summary: "Opc Gateway Detail", collectionName: "serverConnections" },
				resolve: { pageData: NodeResolver },
				runGuardsAndResolvers: "pathParamsOrQueryParamsChange"
			}
		]
	},
/*	{ path: 'gateways',
		component: ComponentSidenav,
		children :[
			{ path: ':target',     component: NodeDetail, canActivate: [AuthGuard], resolve: { pageData: NodeResolver }, providers: [NodeResolver,opcNodeRouteProvider], runGuardsAndResolvers: "pathParamsOrQueryParamsChange" }/*,
			{ path: ':target/:id', component: NodeDetail, canActivate: [AuthGuard], resolve: { pageData: NodeResolver }, providers: [NodeResolver,opcNodeRouteProvider], runGuardsAndResolvers: "paramsChange" },* /
		]
	},
*/
	{ path: 'access', title: "Access", component: Cards, providers: [qlListProvider], canActivate: [AuthGuard], data: {
		summary: "Configure User Access"
	} },
	{ path: 'access', component: ComponentSidenav, canActivate: [AuthGuard], providers:[qlListProvider],
			children :[
				{ path: 'users/:target',
					component: UserDetail,
					providers: [ DetailResolver<User>, accessProvider ],
					resolve: { pageData: DetailResolver<User> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: 'groups/:target',
					component: GroupDetail,
					providers: [ DetailResolver<Group>, accessProvider ],
					resolve: { pageData: DetailResolver<Group> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: 'roles/:target',
					component: RoleDetail,
					providers: [ DetailResolver<Role>, accessProvider ],
					data: { summary: "Role Detail" },
					resolve: { pageData: DetailResolver<Role> },
					canActivate: [AuthGuard],
					runGuardsAndResolvers: "paramsChange"
				},
				{ path: ':collectionDisplay',
					component: QLList,
					runGuardsAndResolvers: "paramsChange",
					providers: [ QLListResolver, accessProvider ],
					resolve: { data: QLListResolver },
					canActivate: [AuthGuard],
					data: { collections: [
						{ path:"users", data:{showAdd:false} },
						{ path:"groups", data:{collectionName: "groupings"} },
						"roles",
						{ path:"resources", data:{canPurge:false} }
					]}
				},
			]
	},
	{
		path: 'apps',
		title: "Applications",
		canActivate: [AuthGuard],
		component: Apps,
		providers: [ AppResolver, accessProvider ],
		resolve: { connections: AppResolver },
	},
	// {
	// 	path: 'settings/appServer', title: "App Server", component: Applications, canActivate: [AuthGuard], data: { summary: "Applications" },
	// },
	// {
	// 	path: 'settings/gateways', title: "Gateways", providers: [{provide: 'IRouteService', useClass: GatewayRouteService}], component: Cards, canActivate: [AuthGuard],
	// 	data: { summary: "Gateways Connected" },
	// },
	// {
	// 	path: 'settings/opcServers', title: "OPC Servers", providers: [{provide: 'IRouteService', useClass: OpcServerRouteService}], component: Cards, canActivate: [AuthGuard],
	// 	data: { summary: "OPC Servers Connected" },
	// },
	{
		path: 'apps/gateways/:instance', title: ":instance", component: ComponentSidenav, canActivate: [AuthGuard],
		children :[
			{
				path: '',
				component: GatewayDetail,
				providers:[ GatewayResolver, gatewayProvider],
				resolve: {data : GatewayResolver},
				canActivate: [AuthGuard]
			},
			{
				path: ':connection',
				component: ClientDetail,
				providers: [ CnnctnDetailResolver, gatewayProvider ],
				canActivate: [AuthGuard],
				data: { summary: "Opc Connection", collectionName: "serverConnections" },
				resolve: { pageData: CnnctnDetailResolver }
			}
		]
	},

//	{ path: 'settings', component: ComponentSidenav, providers: [qlListProvider],
//		children:
//		[
//			{ path: 'applications', component: Applications, title: "Applications", canActivate: [AuthGuard], data: { summary: "View Applications" } },
			// 	{ path: 'gateways', title: "Gateways", canActivate: [AuthGuard], component: Cards,
			// 	children :[
			// 		{ path: ':target',     component: NodeDetail, canActivate: [AuthGuard], resolve: { pageData: NodeResolver }, providers: [NodeResolver,opcNodeRouteProvider], runGuardsAndResolvers: "pathParamsOrQueryParamsChange" }/*,
			// 		{ path: ':target/:id', component: NodeDetail, canActivate: [AuthGuard], resolve: { pageData: NodeResolver }, providers: [NodeResolver,opcNodeRouteProvider], runGuardsAndResolvers: "paramsChange" },*/
			// 	]
			// },

			// {
			// 	path: ':collectionDisplay',
			// 	component: QLList,
			// 	providers:[ QLListResolver, gatewayProvider],
			// 	resolve: {data : QLListResolver},
			// 	canActivate: [AuthGuard],
			// 	data: { collections: [
			// 		{ path:"clients", title: "OPC Clients", data:{summary: "Change OPC Clients on Gateway", collectionName: "clients"} },
			// 	]}
			// },
			// {
			// 	path: 'gateways/:target',
			// 	component: GatewayDetail,
			// 	providers: [ DetailResolver<ServerCnnctn>, gatewayProvider ],
			// 	canActivate: [AuthGuard],
			// 	data: { summary: "Opc Gateway Detail" },
			// 	resolve: { pageData: DetailResolver<ServerCnnctn> }
			// }
//		]
//	}
];
function setRoutes(){
	return routes;
}

@NgModule( { imports: [RouterModule.forRoot([])], exports: [RouterModule],
	providers: [
		{ provide: ROUTES, useFactory: setRoutes, multi: true },AuthGuard]
})
export class AppRoutingModule
{}