
def validate_fields(parameters, input_data, result, reference_data=None):

    def append_error(field, error_type, result, msg):
        if field in result[error_type]:
            result[error_type][field].append(msg)
        else:
            result[error_type][field] = [msg]

    for field, properties in parameters.items():
        value = input_data.get(field, None)
        ref_val = reference_data.get(field) if reference_data else None
        field_type = properties.get('type', str)
        is_required = properties.get('required', str)
        default_val = properties.get('default', None)
        if 'fields' in properties and isinstance(properties.get('fields'), dict):
            result = validate_fields(properties.get('fields'), input_data.get(field), result,
                                     reference_data.get(field) if reference_data else None)
        if value not in [None, '']:
            if not isinstance(value, field_type):
                msg = '%s has invalidate date type expected %s given %s' % (field, field_type, type(value))
                append_error(field, 'errors', result, msg)
            if default_val:
                if default_val not in value:
                    msg = "Field '%s' value %s is mismatched with default value %s" % (field, value, default_val)
                    append_error(field, 'errors', result, msg)
            if ref_val:
                if value != ref_val:
                    msg = "Field '%s' value %s is mismatched with ref value %s" % (field, value, ref_val)
                    append_error(field, 'errors', result, msg)
        else:
            if is_required:
                msg = '%s is required given empty or missing' % field
                append_error(field, 'missing_fields', result, msg)
    return result