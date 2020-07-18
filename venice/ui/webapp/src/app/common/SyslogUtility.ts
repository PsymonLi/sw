import { TableUtility } from '@app/components/shared/tableviewedit/tableutility';
import { Utility } from '@app/common/Utility';

export class SyslogUtility {
  public static formatTargets(data: any, isForExport: boolean = false) {
    if (data == null) {
      return '';
    }
    const retArr = [];
    data.forEach((req) => {
      let targetStr: string = '';
      for (const k in req) {
        if (req.hasOwnProperty(k) && k !== '_ui' && k !== 'credentials') {
          if (req[k]) {
            targetStr += Utility.makeFirstLetterUppercase(k) + ':' +  req[k] + ', ';
          }
        }
      }
      if (targetStr.length === 0) {
        targetStr += '*';
      } else {
        targetStr = targetStr.slice(0, -2);
      }
      retArr.push(targetStr);
    });
    if (isForExport) {
      return TableUtility.displayListInLinesInCvsColumn(retArr);
    }
    return TableUtility.displayListInLinesInColumn(retArr);
  }
  public static formatSyslogExports(data) {
    if (data == null) {
      return '';
    }
    let targetStr: string = '';
    if (data.format) {
      targetStr += 'Format:' +  data.format.replace('syslog-', '').toUpperCase() + ', ';
    }
    for (const k in data.config) {
      if (data.config.hasOwnProperty(k) && k !== '_ui') {
        if (data.config[k]) {
          targetStr += Utility.makeFirstLetterUppercase(k) + ':' +  data.config[k] + ', ';
        }
      }
    }
    if (targetStr.length === 0) {
      targetStr += '*';
    } else {
      targetStr = targetStr.slice(0, -2);
    }
    return [targetStr];
  }
}
