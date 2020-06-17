"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var forms_1 = require("@angular/forms");
function isEmptyInputValue(value) {
    // we don't check for string here so it also works with arrays
    return value == null || value.length === 0;
}
exports.maxValueValidator = function (required) {
    return function (control) {
        if (control.value != undefined) {
            var actual = Number(control.value);
            if (isNaN(actual) || actual > required) {
                return {
                    maxValue: {
                        valid: false,
                        required: required,
                        actual: actual,
                        message: "Value must be less than " + required
                    }
                };
            }
        }
        return null;
    };
};
exports.minValueValidator = function (required) {
    return function (control) {
        if (control.value != undefined) {
            var actual = Number(control.value);
            if (isNaN(actual) || actual < required) {
                return {
                    minValue: {
                        valid: false,
                        required: required,
                        actual: actual,
                        message: "Value must be greater than " + required
                    }
                };
            }
        }
        return null;
    };
};
exports.minLengthValidator = function (required) {
    return function (control) {
        if (isEmptyInputValue(control.value)) {
            return null; // don't validate empty values to allow optional controls
        }
        var length = control.value ? control.value.length : 0;
        if (length < required) {
            return {
                'minlength': {
                    'requiredLength': required,
                    'actualLength': length,
                    message: "Value must be at least " + required + " characters"
                }
            };
        }
        return null;
    };
};
exports.maxLengthValidator = function (required) {
    return function (control) {
        if (isEmptyInputValue(control.value)) {
            return null; // don't validate empty values to allow optional controls
        }
        var length = control.value ? control.value.length : 0;
        if (length > required) {
            return {
                'maxlength': {
                    'requiredLength': required,
                    'actualLength': length,
                    message: "Value must be less than " + required + " characters"
                }
            };
        }
        return null;
    };
};
// export const requiredValidatorCreator = (parentControlName: string, nullParentItem: any, defaultParentItem: any): ValidatorFn => {
//     return (control: AbstractControl): ValidationErrors | null => {
//         // If a control has a parent control, we check if the parent control requires that the child is filled.
//         // If not, we check if the child is actually filled ( not null/ defaults)
//         if (control.parent != null && control.parent.parent != null) {
//             const grandParent: any = control.parent.parent
//             if (grandParent._venice_sdk_propInfo != null) {
//                 // constparentControlName = Object.keys(grandParent.controls).find(name => control.parent === grandParent.controls[name])
//                 grandParent._venice_sdk_propInfo[grandParent] != null
//                 && grandParent._venice_sdk_propInfo[parentControlName].required === false) {
//                     // Check if we have null/defaults
//                     const currVal = control.parent.value
//                     if (_.isEqual(currVal, nullParentItem) || _.isEqual(defaultParentItem)) {
//                         // Item is not filled out and not required by parent
//                         return null;
//                     }
//             }
//         }
//         return isEmptyInputValue(control.value) ? { required: { 'required': true, message: "This field is required" } } : null;
//     }
// }
// }
/**
 * This control is modified to simulate how objects are validated
 * in golang. Since some object fields are pointers to
 * other objects, it's possible to have a not required pointer
 * to an object that then has required fields.
 * Ex. Query spec -> Pagination spec -> count
 *
 * If pagination spec is povided as null, then no further validation is done
 * However, in our venice-sdk we create all nested objects.
 * This control skips required validation if the control above is not required, and all sibling level forms are untouched.
 *
 * @param control
 */
exports.required = function (control) {
    if (control.parent == null || control.parent instanceof forms_1.FormArray) {
        return isEmptyInputValue(control.value) ? { required: { 'required': true, message: "This field is required" } } : null;
    }
    var parent = control.parent;
    if (parent._venice_sdk == null || parent._venice_sdk.required) {
        return isEmptyInputValue(control.value) ? { required: { 'required': true, message: "This field is required" } } : null;
    }
    // Check if all controls under the parent are either null or they're default value
    var isModified = Object.keys(control.parent.controls).some(function (key) {
        var c = control.parent.controls[key];
        if (c.value == null || c.value.length === 0) {
            // empty values
            return false;
        }
        else if (c._venice_sdk != null && c.value == c._venice_sdk.default) {
            // default value
            return false;
        }
        return true;
    });
    if (!isModified) {
        return null;
    }
    // if (customControl.parent != null || customControl.parent._venice_sdk != null) {
    //     customControl._venice_sdk.required
    // }
    return isEmptyInputValue(control.value) ? { required: { 'required': true, message: "This field is required" } } : null;
};
// required must be the enum type to check
exports.enumValidator = function (required) {
    return function (control) {
        if (control.value !== undefined && control.value !== null && control.value !== '') {
            var actual = control.value;
            if (!required[actual]) {
                return {
                    enum: {
                        valid: false,
                        required: required,
                        actual: actual,
                        message: "Value must be one of the given options"
                    }
                };
            }
        }
        return null;
    };
};
exports.patternValidator = function (pattern, message) {
    if (!pattern)
        return forms_1.Validators.nullValidator;
    var regex;
    var regexStr;
    if (typeof pattern === 'string') {
        regexStr = '';
        if (pattern.charAt(0) !== '^')
            regexStr += '^';
        regexStr += pattern;
        if (pattern.charAt(pattern.length - 1) !== '$')
            regexStr += '$';
        regex = new RegExp(regexStr);
    }
    else {
        regexStr = pattern.toString();
        regex = pattern;
    }
    return function (control) {
        if (isEmptyInputValue(control.value)) {
            return null; // don't validate empty values to allow optional controls
        }
        var value = control.value;
        return regex.test(value) ? null :
            {
                'pattern': {
                    'requiredPattern': regexStr, 'actualValue': value,
                    'message': message
                }
            };
    };
};
exports.CustomFormControl = function (formControl, propInfo) {
    formControl._venice_sdk = {
        description: propInfo.description,
        hint: propInfo.hint,
        default: propInfo.default,
    };
    return formControl;
};
exports.CustomFormGroup = function (formGroup, isRequired) {
    formGroup._venice_sdk = {
        required: isRequired,
    };
    return formGroup;
};
