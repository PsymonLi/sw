import { Component, Input, OnInit, ViewEncapsulation } from '@angular/core';
import { Icon } from '@app/models/frontend/shared/icon.interface';
import { Animations } from '@app/animations';
import { OnChanges } from '@angular/core/src/metadata/lifecycle_hooks';
import { Router } from '@angular/router';
import { UIConfigsService } from '@app/services/uiconfigs.service';

export interface Stat {
  value: any;
  description: string;
  tooltip?: string;
  url?: string;
  onClick?: () => void;
  arrowDirection?: StatArrowDirection;
  statColor?: string;
}

export enum StatArrowDirection {
  UP = 'UP',
  DOWN = 'DOWN',
  HIDDEN = 'HIDDEN'
}

export enum CardStates {
  READY = 'READY',
  LOADING = 'LOADING',
  FAILED = 'FAILED',
  NO_DATA = 'NO_DATA'
}

export interface BaseCardOptions {
  title: string;
  backgroundIcon: Icon;
  themeColor?: string;
  icon: Icon;
  lastUpdateTime?: string;
  timeRange?: string;
  cardState?: CardStates;
}

@Component({
  selector: 'app-basecard',
  templateUrl: './basecard.component.html',
  styleUrls: ['./basecard.component.scss'],
  animations: [Animations],
  encapsulation: ViewEncapsulation.None
})
export class BasecardComponent implements OnInit, OnChanges {
  hasHover: boolean = false;
  cardStates = CardStates;

  @Input() title: string;
  @Input() themeColor: string;
  @Input() statColor: string = '#77a746';
  @Input() backgroundIcon: Icon;
  @Input() backgroundIconCentered: boolean = false;
  @Input() icon: Icon;
  @Input() lastUpdateTime: string;
  @Input() timeRange: string;
  // When set to true, card contents will fade into view
  @Input() cardState: CardStates = CardStates.LOADING;
  @Input() menuItems = [];
  // input to toggle the new gradient theme
  @Input() gradTheme: boolean = false;


  showGraph: boolean = false;

  // custom css
  themeCss = {};

  constructor(private router: Router, protected uiconfigsService: UIConfigsService) { }

  ngOnChanges(changes) {
  }

  ngOnInit() {
    this.gradTheme = this.uiconfigsService.isFeatureEnabled('showDashboardHiddenCharts');
    this.generateThemeCss();
  }

  /* changes the custom css of the cards
  updates this.themeCss based on this.gradTheme */
  generateThemeCss() {
    if (!this.gradTheme) {
      this.themeCss = {
        'box-shadow': '0 0 3px 0 ' + this.themeColor,
        'border': this.hasHover ? 'solid 1px ' + this.themeColor : 'solid 1px #cec5bd'
      };
    }
  }
}
