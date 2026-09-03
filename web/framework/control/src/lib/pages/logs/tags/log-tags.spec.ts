import { TestBed } from '@angular/core/testing';
import { ELogLevel } from 'jde-proto/Log';
import { LogTags } from './log-tags';

const make = ( stored:Record<string,string> )=>{
	const fixture = TestBed.createComponent( LogTags );
	fixture.componentRef.setInput( 'tags', stored );
	fixture.componentRef.setInput( 'catalogue', ['default', 'sql', 'socket'] );
	fixture.detectChanges();
	return fixture.componentInstance;
};

describe( 'LogTags.entries', ()=>{
	beforeEach( ()=>TestBed.configureTestingModule({}) );

	//angular-review3 #10: the default row is SYNTHESIZED when nothing is stored (logTags.h falls back to Information).
	//Reporting it as an override made LogSettingsPanel.save diff `undefined != 'Information'` and write a tag-0/Information
	//row for all three sinks on every Save - pinning the instance's default over its configured level, across restarts.
	it( 'omits a synthesized default the user never touched', ()=>{
		const tags = make( {sql: "Debug"} );
		expect( tags.entries() ).toEqual( {sql: "Debug"} );
	} );

	it( 'omits it even when the sink has no stored tags at all', ()=>{
		expect( make({}).entries() ).toEqual( {} );
	} );

	it( 'reports a default the instance really stored', ()=>{
		expect( make({default: "Warning", sql: "Debug"}).entries() ).toEqual( {default: "Warning", sql: "Debug"} );
	} );

	//choosing Information IS a change when the configured default is something else, so a touched row always counts.
	it( 'reports the default once the user picks a level, Information included', ()=>{
		const tags = make( {} );
		const row = tags.dataSource.find( r=>tags.isDefault(r) )!;
		tags.onLevelChange( row, ELogLevel.Information );
		expect( tags.entries() ).toEqual( {default: "Information"} );
	} );

	it( 'reports a changed default level', ()=>{
		const tags = make( {} );
		tags.onLevelChange( tags.dataSource.find(r=>tags.isDefault(r))!, ELogLevel.Warning );
		expect( tags.entries() ).toEqual( {default: "Warning"} );
	} );

	it( 'still skips the trailing empty row', ()=>{
		const tags = make( {sql: "Debug"} );
		expect( tags.dataSource.some(r=>!r.tag) ).toBe( true );
		expect( Object.keys(tags.entries()) ).toEqual( ["sql"] );
	} );
} );
