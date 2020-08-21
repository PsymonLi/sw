Pakcage *searchvos* provides functionality for indexing and searching flow logs.

# Motivation
Elastic consumes huge amount of CPU & Memory resources for indexing flow logs. This custom indexing approach does almost the same work using a fraction of resources. The benchmark comparison is given in the following slides https://docs.google.com/presentation/d/1GIDH9KANQyFrVN7rHvtiSLapaPE5hefoTMHReRZPz1Y/edit

#  Index Creation
1. Indexing of CSV files against IP addresses a.k.a. Slower index 
-- The csv.gz files containing flow logs and stored in minio are indexed against the source ip, destination ip and <source ip,destination ip> pair.
-- The index is stored in minio itself for resiliency. Its stored in the following pattern:
    ```<tenantName>.indexmeta/processedindices/shardID/timestamp/<startTs>-<endTs>```
where startTs and endTs represents the timeperiod for which flow logs are indexed in that file.
-- Index sharding: 
    - The index is divided across shards for achieving data distribution and faster search. 
    - Source IP and Destination IP are used for sharding the index.
    - Number of shards is set to 63. 
    - Cosnistent haashing is not used, hence there is a possibility of overloading a particular shard with more data which will result in slower search for the data present in that shard.

2. Indexing of Raw logs against IP addresses a.k.a. Faster index
-- This index will be maintained only for last 24 hours of flow logs.
-- The raw flow logs are indexed against the source ip, destination ip and <source ip,destination ip> pair.
-- The index and the logs are stored outside minio for achieving faster search. As a side effect the index can get lost if the node dies or Spyglass moves to another node.
-- The index resiliency is achieved by also storing gzipped format of the index & logs in minio. If Spyglass moves to another node then it will download all the old indices from minio. The performance numbers for downloading the index at 10K-CPS rate are given in the slides referenced above.
-- Faster search is achieved by storing file offsets in the index and using file Seek & ReadAt operations for reading, decompressing and unmarshling only the data that is needed for the query. No files are memory mapped yet, which could be an optimization for future.
-- The index is stored outside minio using the following format:
    ```
    <tenantName>.rawlogs/rawlogs/shardID/timestamp/<startTs>-<endTs>.index
    <tenantName>.rawlogs/rawlogs/flows/uuid.flows
    ```
    where:
    index file: Contains the index from source ip, destination ip etc. to the offsets in the flows file.
    flows file: Contains the flow records.
    startTs and endTs: Represent the timeperiod for which flow logs are indexed in that file.
-- Index sharding: 
    - The index is divided across shards for achieving index distribution and faster search. 
    - Source IP and Destination IP are used for sharding the index.
    - Number of shards is set to 63. 
    - Cosnistent haashing is not used, hence there is a possibility of overloading a particular shard with more data which will result in slower search for the data present in that shard.
    - Note that only the index is sharded, data is not sharded i.e. one ".flows" file is created corresponding to all the index shards. 
    
# Index Persistence 
1. Both the indices are persisted every 10 minutes or 6M flows. 
2. If Spyglass crashes then re-indexing is redone for last 10 minutes.
3. The slower index is persisted in minio.
4. The faster index is persisted both outside minio and in minio (for resiliency). 
-- As a result storage consumption is 3x for this index. 2x-minio + 1x-local.
-- The storage consumed by custom indexing is more then Elastic. (reason not known yet).

# Search
1. Search rules
-- If the query spans more then 24 hours then it will get handled by slower index.
-- If the query is within last 24 hours then it will get handled by faster index but if index recreation is going on then the query will get handled by slower index.
-- Every query will first check the index present in memory and then move over to the index present on disk.
-- If both startTs and endTs are zero then the system will search the last 24 hours.
-- If only endTs is zero then we will search the startTs, startTs+24 hours.
-- If only startTs is zero, then we will error out.
2. Search parameters:
-- Source IP
-- Destination IP (optional)
-- Source Port (optional)
-- Destintaion Port (ooptional)
-- Protocol (optional)
3. The search API is a streaming API <details to be filled in by Vishal Jain>
4. The search queries are handed to the slower index while index recreation is going on (happens when Spyglass moves to another node). ***TBD - what happens when index recreation fails?***

# Unsupported Features
1. Searching logs for any to any ip addtess, as a result, on UI we will not be able to show latest logs
from any to any IP address. 
-- The flows are present in the ".flows" file, but are not in sorted order, they are appeneded to the file in the order 
received by indexer. 
-- Its possible to return latest logs in unsorted order, will that suffice?
2. Searching just by DSC-ID
-- Searching just by DSC-ID would need adding one more inverted index for DSC-ID, which itself is straight forward, but 
depending on how many flows are getting reported by a DSC every 10 mins, it could become a memory issue.

# Comments and answers
1. The faster index is stored outside minio but the same disk partition?
Yes, we have only one disk partition for all data (other than config) as of now.
2. Is the flows file rate limited on the size or number of records?
Yes, its persisted every 6M flows or 10 mins.
3. Does the system write to disk every 10 mins?
Slower index: We update minio every 10 mins or 6M flows.
Faster index: We update minio every 10 mins or 6M flows, but we keep writing flows to a temp file locally for saving memory, we write flows to the temp file every 1000th flow. At the time of updating minio, we move the file from the temp location to the final location outside minio as well.
4. How easy is it to scale this solution if we are planning to support more flows/sec/card in the future? Wouldn't it impact spyglass and minio performance?
This change is getting done for supporting 10K/20K CPS across the cluster. We probably can go upto 50K CPS in our cluster with 1 Spyglass. After that we would have to convert Spyglass to daemonset. That said, we cannot go very high then 50K CPS, for doing that we would have to spin up more nodes just for data analytics. In general other companies do data analysis on a separate set of appliances, but we are doing it on our PSM cluster to begin with. Minio have not shown any scale issues till now, we have tried writing to it at a very high scale.
Custom indexing is definitely more undertaking as compared to using Elastic, but the amount of resources saved seems to outweigh the benefits of Elastic running in PSM cluster. If we had a separate analytics cluster then things could have been different.
5. Looks like the indexing is based on IPs, Do we have to enhance this to support search on other fields.
Yes, in future we may start adding labels, workload names etc. to the logs. With that we would have to create a reverse index for those things as well. It will need more CPU, memory and storage resources, which I think Elastic would need too.
6. 1 index for 10mins or 6M data -> 144 indices per day. If the search spans for 24hrs or more, then a minimum of 144 files has to be searched and results have to be aggregated? Is that right? If so, what is the complexity of it?
That's correct. It's hard to come to a figure for defining complexity. The complexity is linear to the volume of fwlogs generated in last 24 hours. In general, reading the index itself is very fast but reading the flows is slower. For slower index reading is done concurrently across all 3 minio instances, for faster index reading is done concurrently across all the files within the hour. In general, the system searches hour by hour i.e. if the user asks for 24 hours of logs, it will search hour by hour and stream the results to the user. The query performance is also given in the slides. Slide 30 and onwards have the latest results.
7. Do we rebuild the cache for slower and faster index if spyglass/minio crashes? And the cache has one day worth of index?
The slower index is generated for all the data present in minio i.e. last 30 days. The faster index is generated only for the last 24 hours. Both of them are stored on disk. We have caching as well which we call IndexCache and ResultsCache. These are LRU caches and cache the index/results of recent queries.
8. Where are the faster indices stored?
Local: As of now at /data/flowlogslocal/tenantname.rawlogs/ on the node where Spyglass is running.
Minio: tenantName.rawlogs/
9. Are flow logs stored twice in minio, raw ones and the indexed ones?
Yes, for last 24 hours.