import { provideHttpClient } from "@angular/common/http";
import { ApplicationConfig } from '@angular/core';
import { provideAnimationsAsync } from '@angular/platform-browser/animations/async';
import { provideRouter } from '@angular/router';
import { AppService, DefaultErrorService,LocalStorageProfile } from 'jde-framework'
import {AccessService} from 'jde-access';
import {EnvironmentService} from './services/environment.service';
import { DisabledAuthService } from 'jde-material';
import { routes } from './app_routing_module';

export const appConfig: ApplicationConfig = {
  providers: [provideAnimationsAsync(),
		provideHttpClient(),
		provideRouter(routes),
		{provide: 'IEnvironment', useClass: EnvironmentService},
		{provide: 'IErrorService', useClass: DefaultErrorService},
		//{provide: 'IGraphQL', useClass: AccessService},
		{provide: 'IProfile', useClass: LocalStorageProfile},
		{provide: 'IAuth', useClass: DisabledAuthService},
		{provide: 'AppService', useClass: AppService},
		{provide: 'AccessService', useClass: AccessService},
	]
};
