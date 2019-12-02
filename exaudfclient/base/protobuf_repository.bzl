def _protobuf_local_repository_impl(repository_ctx):
    if 'PROTOBUF_LIBRARY_PREFIX' in repository_ctx.os.environ:
        library_prefix = repository_ctx.os.environ['PROTOBUF_LIBRARY_PREFIX']
    else:
        fail("Environment Variable PROTOBUF_LIBRARY_PREFIX not found")
    print("protobuf library prefix in environment specified; %s"%library_prefix)

    if 'PROTOBUF_INCLUDE_PREFIX' in repository_ctx.os.environ:
        include_prefix = repository_ctx.os.environ['PROTOBUF_INCLUDE_PREFIX']
    else:
        fail("Environment Variable PROTOBUF_INCLUDE_PREFIX not found")
    print("protobuf include prefix in environment specified; %s"%include_prefix)
    build_file_content = """
cc_library(
    name = "{name}",
    srcs = glob(["opt/conda/lib/libprotobuf*.so"]),
    hdrs = glob(["opt/conda/include/**/*.h","opt/conda/include/**/*.inc", "opt/conda/include/**/*.hpp"]),
    includes = ["opt/conda/include"],
    visibility = ["//visibility:public"]
)""".format( name=repository_ctx.name)
    print(build_file_content)

    repository_ctx.symlink(library_prefix, "./opt/conda/lib")
    repository_ctx.symlink(include_prefix, "./opt/conda/include")
    repository_ctx.file("BUILD", build_file_content)

protobuf_local_repository = repository_rule(
    implementation=_protobuf_local_repository_impl,
    local = True,
    environ = ["PROTOBUF_LIBRARY_PREFIX","PROTOBUF_BIN","PROTOBUF_INCLUDE_PREFIX"])

