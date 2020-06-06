FROM centos
COPY nic/ /nic/
ENV LD_LIBRARY_PATH /nic/lib
ENV LOG_DIR /var/log/pensando/
ENV NBASE_INI_DIR /nic/conf/
ENTRYPOINT ["/nic/bin/pegasus"]
