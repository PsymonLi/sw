This readme provides the steps to update the minigoldfw on minio to release newer minigoldfw

Steps to release minigoldfw to minio:
1. The nic/minigoldfw job in jobd builds the minigoldfw as part of the hourly build job.
2. Find the Release Tag number in jobd for the hourly build from which you want to release the minigold fw
3. Minigold fw can be found under this path: /vol/builds/hourly/< jobd release tag >/usr/src/github.com/pensando/sw/nic/buildroot/output_minigold/images/naples_minigoldfw.tar
4. Copy the naples_minigoldfw.tar from jobd artifact directory to pensando/sw/platform/minigoldfw/naples/ directory as naples_fw.tar filename.
5. Bump up the version of minigoldfw under pensando/sw/minio/VERSION.
6. Run this command to upload the new minigoldfw to minio: cd pensando/sw/ && UPLOAD=1 ./scripts/create-assets.sh
7. Commit the changes in pensando/sw/minio/VERSION file and push to github.com
8. Create PR with this change in pensando/sw/minio/VERSION file and get it merged to master of pensando/sw repo. Once the PR is merged, everyone will have new minigoldfw when the rebase their worskspace with master branch.

