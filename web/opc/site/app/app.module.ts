import { HttpClientModule } from '@angular/common/http';
import { NgModule } from '@angular/core';
import {COMMA, SPACE} from '@angular/cdk/keycodes';
import {MAT_CHIPS_DEFAULT_OPTIONS } from '@angular/material/chips';
import {FormsModule, ReactiveFormsModule} from '@angular/forms';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import {MatDialogModule} from '@angular/material/dialog';
import {MatSnackBarModule} from '@angular/material/snack-bar';
import { RouterModule } from '@angular/router';
import {MatAutocompleteModule} from '@angular/material/autocomplete';
import {MatSelectModule} from '@angular/material/select';
import {NavBar} from 'jde-spa';

import { AppComponent } from './app.component';
import { AppRoutingModule } from './app_routing_module';
//import { AuthGuard } from '../../projects/jde-access/src/lib/services/auth.guard';

@NgModule({
  	imports: [
		AppComponent,
		RouterModule, HttpClientModule, BrowserAnimationsModule, FormsModule, ReactiveFormsModule,
		MatDialogModule, MatSnackBarModule, MatAutocompleteModule, MatSelectModule,// MatInputModule,//MatFormFieldModule,
		AppRoutingModule,NavBar
  ],
  providers: [
		{provide: MAT_CHIPS_DEFAULT_OPTIONS,useValue: {separatorKeyCodes: [COMMA, SPACE]} }
	],
//	bootstrap: [AppComponent]
})
export class AppModule { }