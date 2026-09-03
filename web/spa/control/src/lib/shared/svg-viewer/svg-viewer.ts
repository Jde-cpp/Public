import { HttpClient } from '@angular/common/http';
import {Component, ElementRef, OnInit, inject, input } from '@angular/core';

@Component({
    selector: 'docs-svg-viewer',
    template: '<div class="docs-svg-viewer" aria-hidden="true"></div>',
    standalone: false
})
export class SvgViewer implements OnInit {
  src = input<string>();
  scaleToContainer = input<boolean>();

  private elementRef:ElementRef = inject( ElementRef );
  private http:HttpClient = inject( HttpClient );

  ngOnInit() {
    if (this.src()) {
      this.fetchAndInlineSvgContent(this.src()!);
    }
  }

  private inlineSvgContent(template: string) {
    this.elementRef.nativeElement.innerHTML = template;

    if (this.scaleToContainer()) {
      const svg = this.elementRef.nativeElement.querySelector('svg');
      svg.setAttribute('width', '100%');
      svg.setAttribute('height', '100%');
      svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    }
  }

  private fetchAndInlineSvgContent(path: string): void {
    const svgAbsPath = getAbsolutePathFromSrc(path);
    this.http.get(svgAbsPath, {responseType: 'text'}).subscribe(svgResponse => {
      this.inlineSvgContent(svgResponse);
    });
  }
}

function getAbsolutePathFromSrc(src: string) {
  return src.slice(src.indexOf('assets/') - 1);
}
