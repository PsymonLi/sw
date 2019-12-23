/**
 * This file is generated by the SwaggerTSGenerator.
 * Do not edit.
*/
/* tslint:disable */
import { Validators, FormControl, FormGroup, FormArray, ValidatorFn } from '@angular/forms';
import { minValueValidator, maxValueValidator, minLengthValidator, maxLengthValidator, required, enumValidator, patternValidator, CustomFormControl, CustomFormGroup } from '../../../utils/validators';
import { BaseModel, PropInfoItem } from '../basemodel/base-model';

import { SearchError, ISearchError } from './search-error.model';
import { SearchEntry, ISearchEntry } from './search-entry.model';
import { SearchTenantPreview, ISearchTenantPreview } from './search-tenant-preview.model';
import { SearchTenantAggregation, ISearchTenantAggregation } from './search-tenant-aggregation.model';

export interface ISearchSearchResponse {
    'total-hits'?: string;
    'actual-hits'?: string;
    'time-taken-msecs'?: string;
    'error'?: ISearchError;
    'entries'?: Array<ISearchEntry>;
    'preview-entries'?: ISearchTenantPreview;
    'aggregated-entries'?: ISearchTenantAggregation;
}


export class SearchSearchResponse extends BaseModel implements ISearchSearchResponse {
    /** TotalHits indicates total number of hits matched */
    'total-hits': string = null;
    /** ActualHits indicates the actual hits returned in this response */
    'actual-hits': string = null;
    /** TimeTakenMsecs is the time taken for search response in millisecs */
    'time-taken-msecs': string = null;
    /** Error status for failures */
    'error': SearchError = null;
    /** EntryList is list of all search results with no grouping.
This attribute is populated and valid only in Full request-mode */
    'entries': Array<SearchEntry> = null;
    /** PreviewEntries is a three level grouping of search summary (#hits),
grouped by tenant, category and kind in that order. This attribute
is populated and valid only in Preview request-mode */
    'preview-entries': SearchTenantPreview = null;
    /** AggregatedEntries is a three level grouping of full search results,
Grouped by tenant, category and kind in that order. This attribute
is populated and valid only in Full request-mode */
    'aggregated-entries': SearchTenantAggregation = null;
    public static propInfo: { [prop in keyof ISearchSearchResponse]: PropInfoItem } = {
        'total-hits': {
            description:  `TotalHits indicates total number of hits matched`,
            required: false,
            type: 'string'
        },
        'actual-hits': {
            description:  `ActualHits indicates the actual hits returned in this response`,
            required: false,
            type: 'string'
        },
        'time-taken-msecs': {
            description:  `TimeTakenMsecs is the time taken for search response in millisecs`,
            required: false,
            type: 'string'
        },
        'error': {
            description:  `Error status for failures`,
            required: false,
            type: 'object'
        },
        'entries': {
            description:  `EntryList is list of all search results with no grouping. This attribute is populated and valid only in Full request-mode`,
            required: false,
            type: 'object'
        },
        'preview-entries': {
            description:  `PreviewEntries is a three level grouping of search summary (#hits), grouped by tenant, category and kind in that order. This attribute is populated and valid only in Preview request-mode`,
            required: false,
            type: 'object'
        },
        'aggregated-entries': {
            description:  `AggregatedEntries is a three level grouping of full search results, Grouped by tenant, category and kind in that order. This attribute is populated and valid only in Full request-mode`,
            required: false,
            type: 'object'
        },
    }

    public getPropInfo(propName: string): PropInfoItem {
        return SearchSearchResponse.propInfo[propName];
    }

    public getPropInfoConfig(): { [key:string]:PropInfoItem } {
        return SearchSearchResponse.propInfo;
    }

    /**
     * Returns whether or not there is an enum property with a default value
    */
    public static hasDefaultValue(prop) {
        return (SearchSearchResponse.propInfo[prop] != null &&
                        SearchSearchResponse.propInfo[prop].default != null);
    }

    /**
     * constructor
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    constructor(values?: any, setDefaults:boolean = true) {
        super();
        this['error'] = new SearchError();
        this['entries'] = new Array<SearchEntry>();
        this['preview-entries'] = new SearchTenantPreview();
        this['aggregated-entries'] = new SearchTenantAggregation();
        this._inputValue = values;
        this.setValues(values, setDefaults);
    }

    /**
     * set the values for both the Model and the Form Group. If a value isn't provided and we have a default, we use that.
     * @param values Can be used to set a webapi response to this newly constructed model
    */
    setValues(values: any, fillDefaults = true): void {
        if (values && values['total-hits'] != null) {
            this['total-hits'] = values['total-hits'];
        } else if (fillDefaults && SearchSearchResponse.hasDefaultValue('total-hits')) {
            this['total-hits'] = SearchSearchResponse.propInfo['total-hits'].default;
        } else {
            this['total-hits'] = null
        }
        if (values && values['actual-hits'] != null) {
            this['actual-hits'] = values['actual-hits'];
        } else if (fillDefaults && SearchSearchResponse.hasDefaultValue('actual-hits')) {
            this['actual-hits'] = SearchSearchResponse.propInfo['actual-hits'].default;
        } else {
            this['actual-hits'] = null
        }
        if (values && values['time-taken-msecs'] != null) {
            this['time-taken-msecs'] = values['time-taken-msecs'];
        } else if (fillDefaults && SearchSearchResponse.hasDefaultValue('time-taken-msecs')) {
            this['time-taken-msecs'] = SearchSearchResponse.propInfo['time-taken-msecs'].default;
        } else {
            this['time-taken-msecs'] = null
        }
        if (values) {
            this['error'].setValues(values['error'], fillDefaults);
        } else {
            this['error'].setValues(null, fillDefaults);
        }
        if (values) {
            this.fillModelArray<SearchEntry>(this, 'entries', values['entries'], SearchEntry);
        } else {
            this['entries'] = [];
        }
        if (values) {
            this['preview-entries'].setValues(values['preview-entries'], fillDefaults);
        } else {
            this['preview-entries'].setValues(null, fillDefaults);
        }
        if (values) {
            this['aggregated-entries'].setValues(values['aggregated-entries'], fillDefaults);
        } else {
            this['aggregated-entries'].setValues(null, fillDefaults);
        }
        this.setFormGroupValuesToBeModelValues();
    }


    protected getFormGroup(): FormGroup {
        if (!this._formGroup) {
            this._formGroup = new FormGroup({
                'total-hits': CustomFormControl(new FormControl(this['total-hits']), SearchSearchResponse.propInfo['total-hits']),
                'actual-hits': CustomFormControl(new FormControl(this['actual-hits']), SearchSearchResponse.propInfo['actual-hits']),
                'time-taken-msecs': CustomFormControl(new FormControl(this['time-taken-msecs']), SearchSearchResponse.propInfo['time-taken-msecs']),
                'error': CustomFormGroup(this['error'].$formGroup, SearchSearchResponse.propInfo['error'].required),
                'entries': new FormArray([]),
                'preview-entries': CustomFormGroup(this['preview-entries'].$formGroup, SearchSearchResponse.propInfo['preview-entries'].required),
                'aggregated-entries': CustomFormGroup(this['aggregated-entries'].$formGroup, SearchSearchResponse.propInfo['aggregated-entries'].required),
            });
            // generate FormArray control elements
            this.fillFormArray<SearchEntry>('entries', this['entries'], SearchEntry);
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('error') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('error').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('entries') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('entries').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('preview-entries') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('preview-entries').get(field);
                control.updateValueAndValidity();
            });
            // We force recalculation of controls under a form group
            Object.keys((this._formGroup.get('aggregated-entries') as FormGroup).controls).forEach(field => {
                const control = this._formGroup.get('aggregated-entries').get(field);
                control.updateValueAndValidity();
            });
        }
        return this._formGroup;
    }

    setModelToBeFormGroupValues() {
        this.setValues(this.$formGroup.value, false);
    }

    setFormGroupValuesToBeModelValues() {
        if (this._formGroup) {
            this._formGroup.controls['total-hits'].setValue(this['total-hits']);
            this._formGroup.controls['actual-hits'].setValue(this['actual-hits']);
            this._formGroup.controls['time-taken-msecs'].setValue(this['time-taken-msecs']);
            this['error'].setFormGroupValuesToBeModelValues();
            this.fillModelArray<SearchEntry>(this, 'entries', this['entries'], SearchEntry);
            this['preview-entries'].setFormGroupValuesToBeModelValues();
            this['aggregated-entries'].setFormGroupValuesToBeModelValues();
        }
    }
}

