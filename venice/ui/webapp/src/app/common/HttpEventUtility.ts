
enum EventTypes {
  create = 'Created',
  update = 'Updated',
  delete = 'Deleted',
}

/**
 * Utility for handling event from watch streams. It can be used for any watch API.
 * It processes the events and keeps the updated object in a readonly array.
 * All updates to the array are done in place, so consumers can just maintain a reference to the array
 * to always have the latest data.
 *
 * ngOnChange will never be triggered for any bindings that use this array. Instead to detect a change
 * consumers should use an ngDoCheck with the trackBy method in this class. An example of how to do this
 * is in eventAlertPolicies and alertDestinations.
 *
 * In order to keep the array in chronological ordering, it
 * inserts new items into the beginning of the array.
 */
export class HttpEventUtility<T> {
  private dataArray: Array<T> = [];
  // Maps object name to index in data array
  private dataMapping: { [objName: string]: number } = {};
  private filter: (object: T) => boolean;
  private objectConstructor: any;
  private isSingleton: boolean;

  /**
   * @param objectConstructor   Constructor that will be called on all
   *                            the objects before being put in the array
   *
   * @param isSingleton         Whether the events are for a singleton object
   *                            Supports object being renamed if its a singleton
   *
   * @param filter              If the filter returns false for an object,
   *                            it won't be added to the array
   */
  constructor(objectConstructor: any = null, isSingleton: boolean = false, filter: (object: any) => boolean = null) {
    this.objectConstructor = objectConstructor;
    this.isSingleton = isSingleton;
    this.filter = filter;
  }

  /**
   * Can be used by other components as an efficient way to
   * check if the array has changed
   */
  public static trackBy(index: number, item: any): string {
    return item.meta.name + ' - ' + item.meta['mod-time'];
  }

  public processEvents(eventChunk): ReadonlyArray<T> {
    try {
      const events = eventChunk.events;
      events.forEach(event => {
        let obj;
        if (this.objectConstructor != null) {
          obj = new this.objectConstructor(event.object);
        } else {
          obj = event.object;
        }
        const objName = obj.meta.name;
        if (this.filter != null && !this.filter(obj)) {
          return;
        }
        let index;
        switch (event.type) {
          case EventTypes.create:
            this.addItem(obj, objName);
            break;
          case EventTypes.delete:
            this.deleteItem(objName);
            break;
          case EventTypes.update:
            if (this.isSingleton && this.dataArray.length > 0) {
              // Can only be one object, so all updates are happening
              // to the one object we have
              index = 0;
            } else {
              index = this.dataMapping[objName];
            }
            if (index != null) {
              // We move the object to the end to maintain
              // last modified time ordering.
              this.deleteItem(objName);
              this.addItem(obj, objName);
            } else {
              console.error('Update event received but object was not found');
            }
            break;
          default:
            break;
        }
      });

      return this.dataArray;
    } catch (e) {
      console.error(e, 'This is likely due to the backend response being in an unsupported format');
    }
  }

  /**
   *
   * @param obj Object from the watch stream
   * @param objName Key to be used for the dataMapping
   */
  private addItem(obj: T, objName: string): void {
    const index = this.dataArray.length;
    this.dataArray.unshift(obj);
    this.dataMapping[objName] = this.dataArray.length - 1;
  }

  private deleteItem(objName: string): void {
    const index = this.dataMapping[objName];
    const spliceIndex = this.getIndex(objName);
    this.dataArray.splice(spliceIndex, 1);
    delete this.dataMapping[objName];
    // Decrement index of every element before
    // the one we removed in the array.
    // since we flip indexes in getIndexes, we
    // can just use indexes from dataMapping and
    // remove any with a greater index.
    for (const key in this.dataMapping) {
      if (this.dataMapping.hasOwnProperty(key)) {
        const value = this.dataMapping[key];
        if (value > index) {
          this.dataMapping[key] = value - 1;
        }
      }
    }
  }

  /**
   * Since we insert new elements in the front of the array,
   * the mapping from objName to index is inverted, with the last
   * element in the array having an index of 0.
   */
  private getIndex(objName: string): number {
    const index = this.dataMapping[objName];
    return this.dataArray.length - 1 - index;
  }

  public hasItem(objName: string): boolean {
    return this.dataMapping[objName] != null;
  }

  public get array(): ReadonlyArray<T> {
    return this.dataArray;
  }

}
