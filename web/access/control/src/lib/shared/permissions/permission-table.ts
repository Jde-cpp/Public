import { CommonModule } from "@angular/common";
import { AfterViewInit, Component, Inject, model, OnDestroy, OnInit, signal, ViewChild } from "@angular/core";
import { MatCheckbox, MatCheckboxChange } from "@angular/material/checkbox";
import { MatSortModule, Sort } from "@angular/material/sort";
import { MatTable, MatTableModule } from "@angular/material/table";
import { assert, EnumKeysPipe, IErrorService, IProfile, Settings } from "jde-framework";
import { Permission, Rights } from "../../model/Permission";
import { AccessService } from "../../services/access.service";
import { Resource } from "../../model/Resource";

@Component({
		selector: 'permission-table',
		templateUrl: 'permission-table.html',
		styleUrls: ['permission-table.scss'],
		imports: [CommonModule, MatTableModule, MatCheckbox, EnumKeysPipe, MatSortModule],
})
export class PermissionTable implements OnInit, AfterViewInit, OnDestroy{
	constructor( @Inject('AccessService') private accessService: AccessService, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private cnsle: IErrorService )
	{}

	async ngOnInit(){
		let resources = await this.accessService.loadResources();
		for( const resource of resources ){ //.filter(x=>x.allowed!=Rights.None)
			let permission = this.permissions().find( x=>x.resource?.id==resource.id );
			if( permission ){
				permission.resource = resources.find( x=>x.id==resource.id );
				assert( permission.resource, `Resource not found: ${resource.id}` );
			}else
				permission =  new Permission( {resource: new Resource( resource )} );
			this.availablePermissions.push( permission );
		}
		//let sort = this.profile.value.sort;
		let sort = {active: "schema,resource", direction: "asc"};
		for( const col of sort.active.split(",").filter(x=>this.displayedColumnNames.includes(x)).reverse() )
			this.sortData( {active:col, direction: sort.direction} );
		this.isLoading.set( false );
	}
	async ngAfterViewInit(){
//		this.profile = new Settings<UserSettings>( UserSettings, "permission-table", this.profileService );
//		await this.profile.loadedPromise;
	}
	ngOnDestroy(){
//		this.profile.save();
	}
	sortData($event){
		this.availablePermissions = this.availablePermissions.sort((a,b)=>{
			let y:number;
			if( ["schema", "resource", "deleted", "target"].includes($event.active) ){
				let col:string = $event.active=="resource" ? "name" : $event.active;
				y = a.resource[col].localeCompare( b.resource[col] );
			}else{
				let right = <number><any>Rights[$event.active];
				let value = (x:Permission)=>{ return this.isAllowed(x, right) ? 1 : this.isDenied(x, right) ? -1 : 0; };
				y = value(b) - value(a);
			}
			return $event.direction=="asc" ? y : -y;
		});
		if( this.table )
			this.table.renderRows();
	}

	toggle( $event:MatCheckboxChange|MouseEvent, permission: Permission, rights:Rights ):void{
		if( rights==Rights.All )
			rights = permission.resource.availableRights;
		if( this.isAllowed(permission, rights) ){//allowed->denied
			permission.allowed &= ~rights;
			permission.denied |= rights;
		}else if( this.isDenied( permission, rights ) ){//denied->none
			permission.denied &= ~rights;
		}else{//none->allowed
			if( rights==Rights.None )
				permission.allowed = Rights.None;
			else
				permission.allowed |= rights;
		}
		let permissions = this.permissions();
		let existing = permissions.find( x=>x.resource?.id==permission.resource.id );
		if( existing ){
			existing = permission;
			this.permissions.set( [...permissions] );
		}else
			this.permissions.set( [...permissions,permission] );
	}

	removeUnavailable( permission: Permission, allowed:boolean ):void{
		if( allowed )
			permission.allowed &= permission.resource.availableRights;
		else
			permission.denied &= permission.resource.availableRights;
	}

	isAllowed( permission: Permission, rights:Rights ):boolean{
		let allowed = (permission.allowed ?? 0);
		if( rights==Rights.All )
			rights = permission.resource.availableRights;
//		if( permission.resource.target=="providerTypes" && rights==Rights.Create )
//			console.log( `allowed=${allowed}`);
		let y = ( rights==Rights.None && !allowed && !permission.denied )
			|| (rights!=Rights.None && (allowed & rights)==rights);
//		if( permission.resource.target=="providerTypes" && rights==Rights.Create )
//			console.log( `allowed=${y}`);
		return y;
	}

	isDenied( permission: Permission, rights:Rights ):boolean{
		let denied = (permission.denied ?? 0);
		if( rights==Rights.All )
			rights = permission.resource.availableRights;
		return rights!=Rights.None && (denied & rights)==rights;
	}
	isAvailable( permission: Permission, rights:Rights ):boolean{
		return rights==Rights.None || (permission.resource.availableRights & rights)!=0;
	}

	profile:Settings<UserSettings>;
	permissions=model.required<Permission[]>();
	availablePermissions:Permission[] = [];
	rights = Rights;
	//get sort(){ return this.profile.value.sort; }
	get sort():Sort{ return {active: "schema,resource", direction: "asc"}; }
	@ViewChild('mainTable',{static: false}) table!:MatTable<Permission>;
	isLoading = signal<boolean>( true );
	get displayedColumnNames():string[]{
		let y = ["schema", "resource", "deleted"];
		for( const kv of EnumKeysPipe.prototype.transform.call(this, Rights) )
			y.push( kv.value );
		return y;
	}
}
class UserSettings{
	assign( value:UserSettings ){ this.sort = value.sort; this.showDeleted = value.showDeleted; }
	sort:Sort = {active: "schema,resource", direction: "asc"};
	showDeleted:boolean = false;
}
