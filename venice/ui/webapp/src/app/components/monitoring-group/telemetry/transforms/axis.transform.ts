import { TransformGraphOptions, GraphTransform, TransformDataset } from './types';
import { FormGroup, FormControl } from '@angular/forms';
import { patternValidator } from '@sdk/v1/utils/validators';
import { ChartYAxe, ChartOptions } from 'chart.js';
import { getFieldData } from '../utility';
import { Utility } from '@app/common/Utility';

interface CommonAxisFormInf {
  showAxis: boolean;
  label: string;
  showLabel: boolean;
  showGridlines: boolean;
  min: string;
  max: string;
}

interface AxisFormInf {
  xAxis: CommonAxisFormInf;
  yAxis: CommonAxisFormInf;
}

/**
 * Transforms the X and Y axis labels.
 */
export class AxisTransform extends GraphTransform {
  transformName = 'AxisTransform';

  axisFormGroup: FormGroup = new FormGroup({
    xAxis: AxisTransform.CreateCommonAxisFormGroup(),
    yAxis: AxisTransform.CreateCommonAxisFormGroup(),
  });

  public static CreateCommonAxisFormGroup() {
    return new FormGroup({
      showAxis: new FormControl(true),
      label: new FormControl(''),
      showLabel: new FormControl(true),
      showGridlines: new FormControl(''),
      min: new FormControl('', [patternValidator('^[0-9]*$', 'Must be a numeric value')]),
      max: new FormControl('', [patternValidator('^[0-9]*$', 'Must be a numeric value')]),
    });
  }

  /**
   * Behavior:
   * If there is one data type, it is graphed on the same axis by default,
   * and if it is a percentage: we fix the scaling to be 0-100 percent.
   * We also set its unit type to be the label.
   *
   * If there is more than one data type, we don't put a label and
   * default to either auto, or what the user has entered
   *
   * We never set any of the form controls (except initialization)
   * That way, user selected state is preserved and the transform is idempotent
   *
   * For all the fields that are of type Bytes, we find the max and determine
   * an appropriate scaling.
   *
   * This transform also adds the units to the chart tooltips.
   *
   * TODO: add option whether scale is soft or hard limit
   * TODO: If there are two data types, graph on 2 different y axis (one left and one right)
   *
   * @param opts
   */
  transformGraphOptions(opts: TransformGraphOptions) {
    if (opts.graphOptions.scales == null) {
      opts.graphOptions.scales = {};
    }
    if (opts.graphOptions.scales.yAxes == null || opts.graphOptions.scales.yAxes.length === 0) {
      opts.graphOptions.scales.yAxes = [{}];
    }
    this.setYAxis(opts);
  }

  convertBytes(opts: TransformGraphOptions) {
    const byteData = opts.data.filter( (d) => {
      return d.units === 'Bytes';
    });
    if (byteData.length === 0) {
      return 0;
    }
    // For data that is in bytes, we scale them all
    let max = 0;
    byteData.forEach( (d) => {
      d.series.values.forEach( (v) => {
        const val = v[d.fieldIndex];
        max = Math.max(val, max);
      });
    });
    const byteUnits = Utility.getBytesType(max);
    byteData.forEach( (d) => {
      d.units = byteUnits;
      d.dataset.data = (<any>d.dataset.data).map( (point) => {
        point.y = Utility.scaleBytes(point.y, 2, byteUnits);
        return point;
      });
    });
    if (opts.graphOptions.tooltips.callbacks == null) {
      opts.graphOptions.tooltips.callbacks = {};
    }
  }

  // Attempts to set label to either the user given unit, or an autodetected one
  // Sets whether the label is visible based on the user's selection
  setYAxis(opts: TransformGraphOptions) {
    const axisValues: AxisFormInf = this.axisFormGroup.value;
    const yAxisOptions = opts.graphOptions.scales.yAxes[0];
    yAxisOptions.ticks = {};

    yAxisOptions.scaleLabel = {
      labelString: '',
      fontColor: '#676763',
      fontFamily: 'Fira Sans Condensed',
    };

    // We have no data
    if (opts.data.length === 0) {
      return;
    }

    yAxisOptions.scaleLabel.display = axisValues.yAxis.showLabel;
    yAxisOptions.scaleLabel.labelString = axisValues.yAxis.label.trim();

    // User has set display to be off, or has given a label so we don't need to
    // generate one
    if (!yAxisOptions.scaleLabel.display || axisValues.yAxis.label !== '') {
      return;
    }

    this.convertBytes(opts);

    // Auto detecting label
    let isSameUnit = true;
    let isSameField = true;
    let previousUnit;
    let previousField;
    opts.data.forEach( (d) => {
      if (previousUnit == null) {
        previousUnit = d.units;
        previousField = d.field;
      }
      if (previousField !== d.field) {
        isSameField = false;
      }
      if (previousUnit !== d.units) {
        isSameUnit = false;
      }
    });

    if (isSameField) {
      // We put the field name as the x axis
      yAxisOptions.scaleLabel.labelString = getFieldData(opts.data[0].measurement, opts.data[0].field).displayName;
      if (previousUnit === 'Percent') {
        yAxisOptions.scaleLabel.labelString += ' (%)';
      } else if (previousUnit != null) {
        yAxisOptions.scaleLabel.labelString += ' (' + previousUnit + ')';
      }
    } else if (isSameUnit && previousUnit != null) {
      yAxisOptions.scaleLabel.labelString = previousUnit;
    } else {
      yAxisOptions.scaleLabel.labelString = '';
    }


    if (opts.graphOptions.tooltips.callbacks == null) {
      opts.graphOptions.tooltips.callbacks = {};
    }

    opts.graphOptions.tooltips.callbacks.label = ((tooltipItem, data) => {
      let label = data.datasets[tooltipItem.datasetIndex].label;
      let labelVal: any = tooltipItem.yLabel;
      const labelValInt = parseInt(labelVal, 10);
      if (!isNaN(labelValInt)) {
        labelVal = labelVal.toFixed(2);
      }
      label += '  ' + labelVal;

      let units = opts.data[tooltipItem.datasetIndex].units;
      if (units != null) {
        if (units === 'Percent') {
          units = '%';
        }
        return  label  + ' ' + units;
      }
      return label as any;
    });

    // Setting scales
    // we either set to user value, 0-100 for percent, or leave
    // blank to let chart js auto detect
    const min = parseInt(axisValues.yAxis.min, 10);
    if (axisValues.yAxis.min  !== '' && !isNaN(min)) {
      yAxisOptions.ticks.min = min;
    } else if (previousUnit === 'Percent') {
      yAxisOptions.ticks.suggestedMin = 0;
    }

    const max = parseInt(axisValues.yAxis.max, 10);
    if (axisValues.yAxis.max !== '' && !isNaN(max)) {
      yAxisOptions.ticks.max = max;
    } else if (previousUnit === 'Percent') {
      yAxisOptions.ticks.suggestedMax = 100;
    }
  }

}

