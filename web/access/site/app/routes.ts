import { NgModule } from '@angular/core';
import {Routes, RouterModule} from '@angular/router';

import { ComponentCategoryList } from 'jde-material';
import { ComponentSidenav } from 'jde-material';

import{ GraphQLComponent } from 'jde-framework';
import{ GraphQLDetailComponent } from 'jde-framework';

export const routes: Routes =
[
	{
		path: 'settings',
		component: ComponentSidenav,
		data: { name: "Settings" },
		children :
		[
			{ path: 'opcServers/:id', component: GraphQLDetailComponent },
			{ path: 'opcServers', component: GraphQLComponent, data: { name: "OpcServers", summary: "View/Modify Servers" } },
			{ path: 'users/:id', component: GraphQLDetailComponent },
			{ path: 'users', component: GraphQLComponent, data: { name: "Users", summary: "View/Modify Users" } },
			{ path: 'roles/:id', component: GraphQLDetailComponent },
			{ path: 'roles', component: GraphQLComponent, data: { name: "Roles", summary: "View/Modify Roles" } },
			{ path: 'groups/:id', component: GraphQLDetailComponent },
			{ path: 'groups', component: GraphQLComponent, data: { name: "Groups", summary: "View/Modify Groups" } },
			{ path: '', component: ComponentCategoryList, data: { name: "Settings", summary: "Site Settings" } }
		]
	}
];

@NgModule( { imports: [RouterModule.forRoot(routes)], exports: [RouterModule]} )
 export class AppRoutingModule {
 }