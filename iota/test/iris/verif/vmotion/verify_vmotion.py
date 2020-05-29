import iota.harness.api as api

def Main(tc):
    vmotion_enabled = getattr(tc.args, 'vmotion_enable', False)
    if not vmotion_enabled:
        return api.types.status.SUCCESS

    if getattr(tc, 'skip', False):
        return api.types.status.SUCCESS

    if hasattr(tc, 'vmotion_cntxt'):
        api.Logger.info('vMotion time-profile: %s' % ','.join(map(str, tc.vmotion_cntxt.TimeProfile)))
        return tc.vmotion_resp 
    
    api.Logger.error("vMotion enabled but no vmotion context available") 
    return api.types.status.FAILURE

