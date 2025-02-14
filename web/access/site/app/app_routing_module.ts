import { NgModule } from '@angular/core';
import {Routes, ROUTES, RouterModule} from '@angular/router';

import { ComponentCategoryList } from 'jde-material';
import { ComponentSidenav } from 'jde-material';

import{ Applications, GraphQLComponent, GraphQLDetailComponent, LoginPageComponent } from 'jde-framework';
import{ AccessService } from 'jde-access';
import { RoleDetail } from '../../projects/jde-access/src/lib/pages/roles/role-detail';
import { RoleResolver } from '../../projects/jde-access/src/lib/services/role.resolver';


export const routes: Routes = [
	{ path: '', title: "Home", component: ComponentCategoryList, data: { name: "Home", summary: "Welcome" } },
	{ path: 'login', title: "Login", component: LoginPageComponent, data: {name: "Login", summary: "Login to Site"} },
	{
		path: 'access',
		component: ComponentSidenav,
		title: "Access",
		data: { name: "Access" },
		children :[
			{ path: '', title: "Settings", component: ComponentCategoryList, data: { summary: "Access" } },
			{ path: 'users/:id', component: GraphQLDetailComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ excludedColumns:["isGroup","password"]}
			}},
			{ path: 'users', title: "Users", component: GraphQLComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ summary: "View/Modify Users", excludedColumns:["isGroup","password"]}
			}},
			{ path: 'groups/:id', component: GraphQLDetailComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ excludedColumns:["isGroup"] }
			}},
			{ path: 'groups', title: "Groups", component: GraphQLComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ summary: "View/Modify Groups", type:"groupings", excludedColumns:["isGroup"] }
			}},
			{
				path: 'roles/:target',
				component: RoleDetail,
				providers: [
					RoleResolver,
					{provide: 'IGraphQL', useClass: AccessService}],
				resolve: { roleData: RoleResolver }
			},
			{ path: 'roles', title: "Roles", component: GraphQLComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ summary: "View/Modify Roles" }
			}},
			{ path: 'resources/:id', component: GraphQLDetailComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}] },
			{ path: 'resources', title: "Resources", component: GraphQLComponent, providers: [ {provide: 'IGraphQL', useClass: AccessService}], data: {
				pageSettings:{ summary: "View/Modify Resources", excludedColumns:["criteria"]}
			}},
		]
	},
	{
		path: 'settings',
		title: "Settings",
		component: ComponentSidenav,
		data: { summary: "Settings Main Menu" },
		children :[
			{ path: '', title: "Settings", component: ComponentCategoryList, data: { summary: "Settings Sub outlet" } },
			{ path: 'applications', title:"Applications", component: Applications, data: { summary: "View Applications" } },
		]
	}
];
function setRoutes(){
	return routes;
}

@NgModule( { imports: [RouterModule.forRoot([])], exports: [RouterModule],
	providers: [{
      provide: ROUTES,
      useFactory: setRoutes,
			multi: true
	}]})
 export class AppRoutingModule {
 }
