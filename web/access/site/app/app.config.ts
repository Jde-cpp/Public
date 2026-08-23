import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { provideRouter, withComponentInputBinding } from '@angular/router';
import { AppService, LocalStorageProfile } from 'jde-framework'
import {AccessService} from 'jde-access';
import {EnvironmentService} from './services/environment-service';
import { routes } from './app.routes';

export const appConfig: ApplicationConfig = {
  providers: [
		provideHttpClient(),
		provideRouter(routes, withComponentInputBinding()),
		{provide: 'IEnvironment', useClass: EnvironmentService},
		{provide: 'IProfile', useClass: LocalStorageProfile},
		{provide: 'IAuth', useClass: AppService},
		{provide: 'AppService', useClass: AppService},
		{provide: 'AccessService', useClass: AccessService},
	]
};
