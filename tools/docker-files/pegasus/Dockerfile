FROM centos
COPY nic/ /nic/
ENV LD_LIBRARY_PATH /nic/lib
ENV LOG_DIR /var/log/pensando/
ENTRYPOINT ["/nic/bin/pegasus"]
