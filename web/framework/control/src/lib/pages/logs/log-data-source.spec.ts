import { signal } from '@angular/core';
import { ELogLevel } from 'jde-proto/Log';
import { Guid } from '../../model/guid';
import { LogDataSource } from './log-data-source';
import { Entry, LogView } from './log-entry';

const guid = ( n:number )=>new Guid( n.toString(16).padStart(32, '0') );
const entry = ( line:number, template:number ):Entry=>( {templateId: guid(template), argIds: [], level: ELogLevel.Information, tags: [], line, time: new Date(0), userId: 0, fileId: guid(1), functionId: guid(1), hidden: false} );
const noFilter = { messageIds: [], message: undefined, level: ELogLevel.NoLog };

//angular-review #44: filterData's selection-follow was dead code - it paged to 0 whatever was selected, so hiding a message
//template scrolled the selected row out of view.  Six entries, two per page; the pages the source publishes are captured.
describe( 'LogDataSource.filterData', ()=>{
	const create = ( count:number, limit:number )=>{
		const ds = new LogDataSource( signal({limit} as unknown as LogView) );
		for( let i=0; i<count; ++i )
			ds.allEntries.push( entry(i, 10+i) );
		const pages:number[][] = [];
		ds.connect( null as any ).subscribe( p=>pages.push(p.map(e=>e.line)) );
		return { ds, pages };
	};

	it( 'pages to the selected entry after the filter hides rows above it', ()=>{
		const { ds, pages } = create( 6, 2 );
		ds.allEntries[4].selected = true;//visible index 4 today; hiding entry 0 makes it visible index 3, which is page 1 (start 2)
		ds.filterData( {...noFilter, messageIds: [guid(10)]} );
		expect( pages.at(-1) ).toEqual( [3, 4] );
		expect( ds.allEntries[4].selected ).toBe( true );
	} );

	it( 'pages to the first page and drops the selection when the filter hides the selected entry', ()=>{
		const { ds, pages } = create( 6, 2 );
		ds.allEntries[4].selected = true;
		ds.filterData( {...noFilter, messageIds: [guid(14)]} );
		expect( pages.at(-1) ).toEqual( [0, 1] );
		expect( ds.allEntries[4].selected ).toBeFalsy();
	} );

	it( 'pages to the first page when nothing is selected', ()=>{
		const { ds, pages } = create( 6, 2 );
		ds.allEntries[1].hidden = true;//a stale flag from an earlier filter is recomputed, not trusted
		ds.filterData( noFilter );
		expect( pages.at(-1) ).toEqual( [0, 1] );
	} );
} );
