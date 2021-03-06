import { ArrayChunkUtility } from '@app/common/ArrayChunkUtility';


describe('ArrayChunkUtility', () => {
  const data = [];
  const newData = [];
  const newData2 = [];
  for (let index = 0; index < 300; index++) {
    data.push(index);
    newData.push(index + 'new');
    newData2.push(index + 'new2');
  }

  it('should return correct data chunks', () => {
    const chunkUtility = new ArrayChunkUtility(data);
    let res = chunkUtility.requestChunk(50, 83);
    expect(res.length).toBe(33);
    expect(res[0]).toBe(50);
    expect(res[32]).toBe(82);
    expect(chunkUtility.length).toBe(300);

    // Update data but dont switch
    chunkUtility.updateData(newData);

    // Should return the exact same data
    res = chunkUtility.getLastRequestedChunk();
    expect(res.length).toBe(33);
    expect(res[0]).toBe(50);
    expect(res[32]).toBe(82);

    // Switch to new data
    chunkUtility.switchToNewData();

    res = chunkUtility.getLastRequestedChunk();
    expect(res.length).toBe(33);
    expect(res[0]).toBe('50new');
    expect(res[32]).toBe('82new');

    res = chunkUtility.requestChunk(200, 250);
    expect(res.length).toBe(50);
    expect(res[0]).toBe('200new');
    expect(res[49]).toBe('249new');

  });

  it('should support enabling/disabling instant update', () => {
    const chunkUtility = new ArrayChunkUtility(data, true);
    chunkUtility.updateData(newData);

    // should switch to data automatcially
    let res = chunkUtility.requestChunk(50, 83);
    expect(res.length).toBe(33);
    expect(res[0]).toBe('50new');
    expect(res[32]).toBe('82new');

    // shouldn't auto switch anymore
    chunkUtility.disableInstantUpdate();
    chunkUtility.updateData(data);
    res = chunkUtility.requestChunk(50, 83);
    expect(res.length).toBe(33);
    expect(res[0]).toBe('50new');
    expect(res[32]).toBe('82new');


    // update with instant switch
    chunkUtility.updateData(newData2, true);
    res = chunkUtility.requestChunk(50, 83);
    expect(res.length).toBe(33);
    expect(res[0]).toBe('50new2');
    expect(res[32]).toBe('82new2');

  });

  it('should through errors on invalid requests', () => {
    spyOn(console, 'error');

    const chunkUtility = new ArrayChunkUtility(data);
    chunkUtility.getLastRequestedChunk();
    expect(console.error).toHaveBeenCalledTimes(1);
    expect(console.error).toHaveBeenCalledWith('invalid chunk request');

    let res = chunkUtility.requestChunk(50, 83);
    chunkUtility.requestChunk(null, 2);
    expect(console.error).toHaveBeenCalledTimes(2);
    expect(console.error).toHaveBeenCalledWith('invalid chunk request');
    chunkUtility.requestChunk(0, null);
    expect(console.error).toHaveBeenCalledTimes(3);
    expect(console.error).toHaveBeenCalledWith('invalid chunk request');
    // The previous valid chunk request shouldn't have been overwritten
    res = chunkUtility.getLastRequestedChunk();
    expect(res.length).toBe(33);
  });

  it('should sort primitives', () => {
    const chunkUtility = new ArrayChunkUtility(data);
    let res = chunkUtility.requestChunk(50, 83);
    expect(res.length).toBe(33);
    expect(res[0]).toBe(50);
    expect(res[32]).toBe(82);

    // sort in reverse
    chunkUtility.sort(null, -1);
    res = chunkUtility.requestChunk(0, 50);
    expect(res.length).toBe(50);
    console.log(res.slice(40, 49));
    expect(res[0]).toBe(299);
    expect(res[49]).toBe(250);

    chunkUtility.updateData(newData);
    chunkUtility.switchToNewData();
    // Should have already sorted the newData
    res = chunkUtility.getLastRequestedChunk();
    expect(res.length).toBe(50);
    // Sorted by string now
    expect(res[0]).toBe('9new');
    expect(res[49]).toBe('55new');
  });

  it('should sort objects', () => {
    const dataObjects = [];

    const oneDayAgo = new Date(new Date().getTime() - (24 * 60 * 60 * 1000));
    for (let index = 0; index < 48; index++) {
      const time = new Date(oneDayAgo.getTime() + (index * 30 * 60 * 1000));
      dataObjects.push({
        index: 47 - index,
        'nested-property': {
          time: time
        }
      });
    }
    const chunkUtility = new ArrayChunkUtility(dataObjects);
    chunkUtility.sort('index', 1);
    let res = chunkUtility.requestChunk(0, 10);
    expect(res.length).toBe(10);
    expect(res[0].index).toBe(0);
    expect(res[9].index).toBe(9);

    // sort time
    chunkUtility.sort('nested-property.time', -1);
    res = chunkUtility.getLastRequestedChunk();
    expect(res.length).toBe(10);
    expect(res[0]['nested-property'].time.getTime()).toEqual(oneDayAgo.getTime() + (47 * 30 * 60 * 1000));
    expect(res[9]['nested-property'].time.getTime()).toEqual(oneDayAgo.getTime() + (38 * 30 * 60 * 1000));
  });

});
