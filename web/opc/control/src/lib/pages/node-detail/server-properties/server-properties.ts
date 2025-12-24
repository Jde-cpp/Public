import { Component, computed, model, OnDestroy, OnInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatFormFieldModule, MatLabel } from '@angular/material/form-field';
import {MatInputModule} from '@angular/material/input';
import { Field, form, readonly, disabled } from '@angular/forms/signals';
import { Server } from '../../../model/Server';

@Component( {
	  selector: 'server-properties',
    templateUrl: './server-properties.html',
		styleUrls: ['./server-properties.scss'],
		host: {class:'main-content mat-drawer-container my-content'},
    imports: [CommonModule,Field, MatFormFieldModule, MatInputModule]
})
export class ServerProperties implements OnInit {
	ngOnInit(): void {
	}
	server = model.required<Server>();
	form = form(this.server, (server) => {
		const isDisabled = ()=>true;
		disabled( server.applicationName, isDisabled );
		disabled( server.applicationType, isDisabled );
		disabled( server.productUri, isDisabled );
		disabled( server.applicationUri, isDisabled );
		disabled( server.gatewayServerUri, isDisabled );
		disabled( server.discoveryProfileUri, isDisabled );
		disabled( server.discoveryUrls, isDisabled );
		disabled( server.policy, isDisabled );
		disabled( server.mode, isDisabled );
	});
}
