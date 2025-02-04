import { Injectable, Inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { IEnvironment } from 'jde-material';
import { IErrorService, IGraphQL, AppService } from 'jde-framework';


@Injectable( {providedIn: 'root'} )
export class AccessService extends AppService implements IGraphQL{
	constructor( http: HttpClient, @Inject('IEnvironment') environment: IEnvironment, @Inject('IErrorService') cnsle: IErrorService ){
		super( http, environment, cnsle );
	}
};
