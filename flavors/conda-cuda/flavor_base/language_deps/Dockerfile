FROM {{udfclient_deps}}

RUN mkdir -p /build_info/packages/language_deps
COPY language_deps/packages/apt_get_packages /build_info/packages/language_deps

RUN apt-get update && \
    apt-get -y install $(cat /build_info/packages/language_deps/apt_get_packages) && \
    locale-gen en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 && \
    apt-get -y clean && \
    apt-get -y autoremove && \
    ldconfig

RUN /opt/conda/bin/conda install -y -v -c nvidia -c rapidsai -c conda-forge -c defaults rapids=0.10 python=3.6 numpy pandas cython gcc_linux-64 gxx_linux-64 gfortran_linux-64 swig bazel protobuf libprotobuf zeromq  cppzmq
ENV PATH=$PATH:/opt/conda/bin

COPY scripts /scripts
COPY language_deps/packages/pip3_packages /build_info/packages/language_deps

ENV PYTHON3_PREFIX /opt/conda/
ENV PYTHON3_VERSION python3.6
ENV PROTOBUF_BIN=/opt/conda/bin/protoc
ENV PROTOBUF_LIBRARY_PREFIX=/opt/conda/lib
ENV PROTOBUF_INCLUDE_PREFIX=/opt/conda/include
