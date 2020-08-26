import { Component, OnInit, Input, ViewChild, ChangeDetectionStrategy, OnChanges, SimpleChanges } from '@angular/core';
import { UIChart } from 'primeng';
import { TopRulesUtil, RuleHitCount } from '@app/components/dashboard/widgets/firewalltopten/TopRulesUtil';
import { ControllerService } from '@app/services/controller.service';
import { BaseComponent} from '@app/components/base/base.component';
import { Utility } from '@app/common/Utility';
import { SecuritySGRule, ISecuritySGRule } from '@sdk/v1/models/generated/security';
import { TrimUIFields } from '@sdk/v1/utils/utility';
import { TableCol } from '@app/components/shared/tableviewedit';
import { MenuItems } from '@app/components/shared/dsbdwidgetheader/dsbdwidgetheader.component';

@Component({
  selector: 'app-toprulescard',
  templateUrl: './toprulescard.component.html',
  styleUrls: ['./toprulescard.component.scss'],
})
export class ToprulescardComponent extends BaseComponent implements OnInit, OnChanges {


  @ViewChild('chart') pieChart: UIChart;

  @Input() data: any = [];
  @Input() title: any = {};
  @Input() dataReady: boolean = false;
  @Input() ruleMap: any = {};
  @Input() ruleAction: string = '';

  updateTime: any = '';
  clickMode: boolean = false;
  pieColors: any = TopRulesUtil.pieColors;
  indexMap: {[key: string]: number} = {};
  chartData: any = {};
  highlightedRule: string = '';
  clickedElement: any = null;
  menuItems: MenuItems[] = [
  ];

  options: any = {
    layout: {
      padding: {
          left: 15,
          right: 15,
          top: 15,
          bottom: 15
      }
    },
    tooltips: {
      enabled: true,
      displayColors: false,
      titleFontFamily: 'Fira Sans Condensed',
      titleFontSize: 14,
      bodyFontFamily: 'Fira Sans Condensed',
      bodyFontSize: 13,
      footerFontStyle: 'normal',
      callbacks: {
        label: function (tooltipItem, data) {
          const index = tooltipItem.index;
          const actualPercent = data.alldata.data[index] * 100 / data.alldata.totalHits;
          return TopRulesUtil.getHitsText(data.alldata.data[index]) + ' Hits - ' + TopRulesUtil.reducePercentagePrecision(actualPercent) + '%';
        },
      }
    },
    title: {
      display: false
    },
    legend: {
      display: false
    },
    cutoutPercentage: 60,
    circumference: 2 * Math.PI,
    plugins: {
      datalabels: {
        backgroundColor: function (context) {
          return context.dataset.backgroundColor;
        },
        borderColor: 'white',
        borderRadius: 25,
        borderWidth: 2,
        color: 'white',
        display: function (context) {
          return context.dataIndex > 0;
        },
        font: {
          weight: 'bold',
          family: 'Fira Sans Condensed'
        },
        formatter: Math.round
      }
    },
    animation: {
      duration: 0,
    },
    hover: {
      mode: 'nearest',
      intersect: true,
      onHover: (e, element) => {
        if (!this.clickMode) {
          this.pieChart.chart.controller.update();
          if (element.length > 0) {
            this.highlightedRule = element[0]._model.label;
            element[0]._model.outerRadius = element[0]._chart.outerRadius + 5;
          } else {
            this.highlightedRule = '';
          }
        }
      }
    },
    onClick: (e, element) => {
      if (element.length) {
        if (this.clickedElement) {
          this.clickedElement._model.outerRadius = this.clickedElement._chart.outerRadius;
        }
        if (this.clickedElement && this.clickedElement._model.label === element[0]._model.label) {
          this.clickedElement = null;
          this.clickMode = false;
        } else {
          this.clickedElement = element[0];
          element[0]._model.outerRadius = element[0]._chart.outerRadius + 5;
          this.clickMode = true;
        }
      } else {
        this.clickMode = false;
        if (this.clickedElement) {
          this.clickedElement._model.outerRadius = this.clickedElement._chart.outerRadius;
          this.clickedElement = null;
        }
      }
    }
  };

  constructor(protected controllerService: ControllerService) {
    super(controllerService);
   }

  ngOnInit() {

  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes.hasOwnProperty('dataReady') && changes.dataReady.currentValue) {
      this.sortRuleMetrics();
      this.updateChart();
      this.updateTime = Date.now();
      this.setMenuItems();
    }
  }

  rowClick(label: string) {
    const meta = this.pieChart._data.datasets[0]._meta;
    const keys = Object.keys(meta);
    const chartElements: any[] = meta[keys[0]].data;
    const pieIndex: number = this.indexMap[label];
    chartElements[pieIndex]._model.outerRadius = chartElements[pieIndex]._chart.outerRadius + 5;
    this.clickedElement = chartElements[pieIndex];
    this.clickMode = true;
    chartElements[pieIndex].draw();
    this.pieChart.chart.render();
  }

  setMenuItems() {
    this.menuItems = [{
      text: 'Export JSON',
      onClick: () => {
        this.exportJson();
      }
    }
  ];
  }

  exportJson() {
    const exportObj = {
      'topRules': this.chartData.labels.map((label, index) => {
        return {ruleHash: label, hits: this.chartData.alldata.data[index], ruleObj: this.ruleMap[label].rule};
      })
    };
    const fieldName = 'top-' + this.ruleAction + '-rules.json';
    Utility.exportContent(JSON.stringify(exportObj, null, 2), 'text/json;charset=utf-8;', fieldName);
    Utility.getInstance().getControllerService().invokeInfoToaster('Data exported', 'Please find ' + fieldName + ' in your downloads folder');
  }

  sortRuleMetrics() {
    const desc = (a: RuleHitCount, b: RuleHitCount) => {
      if (a.hits > b.hits) {
        return -1;
      } else if (a.hits < b.hits) {
        return 1;
      } else {
        return 0;
      }
    };
    this.data.sort(desc);
  }

  updateChart() {
    const f = (arr: RuleHitCount[]) => {
      const data = [];
      const labels = [];
      let count = 0;
      const otherHits = 0;
      let totalHits = 0;
      for ( const c of arr) {
        if (count < 10) {
          data.push(c.hits);
          labels.push(c.hash);
          count += 1;
        } else {
          // Others pie temporarily removed, hence the break
          // otherHits += c.hits;
          break;
        }
        totalHits += c.hits;
      }

      // Others pie temporarily removed
      // if (otherHits) {
      //   data.push(otherHits);
      //   labels.push('Others');
      // }
      return {data: data, labels: labels, totalHits: totalHits};
    };

    const dataAllow = f(this.data);

    this.indexMap = {};
    dataAllow.labels.forEach( (label, index) => {
      this.indexMap[label] = index;
    });

    this.chartData = {
      labels: dataAllow['labels'],
      alldata: dataAllow,
      datasets: [
        {
          data: dataAllow['data'].map(v => Math.max(v / dataAllow['totalHits'] * 100, 1)),
          backgroundColor: this.pieColors,
          borderColor: '#000000',
          borderWidth: 0.5,
          borderAlign: 'center',
          hoverRadius: 30,
        }
      ]
    };
  }

  getHitsText(hits: number) {
    return TopRulesUtil.getHitsText(hits);
  }

  reducePercentagePrecision(number: number) {
    return TopRulesUtil.reducePercentagePrecision(number);
  }

  getRuleDetail(): string {
    const ruleHash = this.clickedElement._model.label;
    const rule = TrimUIFields(new SecuritySGRule(this.ruleMap[ruleHash].rule).getModelValues());
    const msg = this.getJSONStringList(rule);
    return msg;
  }

  getJSONStringList(obj: any): string {
    const list = [];
    Utility.traverseJSONObject(obj, 0, list, this);
    return list.join('<br/>');
  }

  formatApp(rule: ISecuritySGRule) {
    let protoPorts = [];
    let apps = [];
    if (rule['apps'] && rule['apps'].length > 0) {
      apps = rule['apps'];
      // return 'Apps:' + apps.join('');
      return '-';
    }
    if (rule['proto-ports'] != null) {
      protoPorts = rule['proto-ports'].map((entry) => {
        if (entry.ports == null || entry.ports.length === 0) {
          return entry.protocol;
        }
        return entry.protocol + '/' + entry.ports;
      });
    }
    return protoPorts.join(', ');
  }

}
