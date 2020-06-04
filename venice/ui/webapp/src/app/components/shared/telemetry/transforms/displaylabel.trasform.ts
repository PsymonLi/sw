import { MetricTransform, TransformDataset, TransformNames } from './types';

/**
 * Modifies the dataset so that they use the displayName from the
 * field metadata instead of name for the label.
 */
export class DisplayLabelTransform extends MetricTransform<{}> {
  labelFormat: (label: string) => string;
  transformName = TransformNames.DisplayLabelTransform;

  transformDataset(opts: TransformDataset) {
    const fieldData = this.getFieldData(opts.measurement, opts.field);
    opts.dataset.label = fieldData ? fieldData.displayName : null;
    if (this.labelFormat) {
      opts.dataset.label = this.labelFormat(opts.dataset.label);
    }
  }

  load(config: any) {
    if (config && config.labelFormat) {
      this.labelFormat = config.labelFormat;
    }
  }

  save(): {} {
    return {
      labelFormat: this.labelFormat
    };
  }
}

