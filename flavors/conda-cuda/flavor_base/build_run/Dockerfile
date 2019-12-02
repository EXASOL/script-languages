FROM {{language_deps}}

COPY --from={{build_deps}} /usr /usr
COPY --from={{build_deps}} /lib /lib
COPY --from={{build_deps}} /bin /bin
COPY --from={{build_deps}} /opt /opt
COPY --from={{build_deps}} /etc /etc


RUN ldconfig

RUN mkdir /exaudf_src /exaudf
COPY src/ /exaudf_src/

WORKDIR /exaudf_src/
ENV CC /opt/conda/bin/x86_64-conda_cos6-linux-gnu-cc
ENV CXX /opt/conda/bin/x86_64-conda_cos6-linux-gnu-c++
ENV PROTOBUF_INCLUDE_PREFIX /opt/conda/include/
RUN ["/bin/bash", "-c", "bash build.sh --config no-tty --config optimize --config python --config fast-binary-py3 --config verbose"]
RUN cp -r -L bazel-bin/* /exaudf

RUN ./test_udfclient.sh /exaudf/exaudfclient_py3

COPY --from={{build_deps}} /build_info /build_info
