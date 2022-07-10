# buildifier: disable=module-docstring
def _execute_command(ctx, cmd):
    result = ctx.execute(cmd)
    if result.return_code != 0:
        return None
    else:
        return result.stdout

def _execute_pkg_config(ctx, pkg_name, options):
    return _execute_command(ctx, ["pkg-config", pkg_name] + options)

def _pkg_config_include(ctx, pkg_name):
    result = _execute_pkg_config(ctx, pkg_name, ["--cflags-only-I"])
    return result.strip()[len("-I"):]

def _pkg_config_linkdir(ctx, pkg_name):
    result = _execute_pkg_config(ctx, pkg_name, ["--libs-only-L"])
    return result.strip()[len("-L"):]

def _pkg_config_linkopt(ctx, pkg_name):
    result = _execute_pkg_config(ctx, pkg_name, ["--libs-only-l"])
    return result.strip()[len("-l"):]

def get_dynamic_lib(linkdir, linkopt):
    return "%s/lib%s.dylib" % (linkdir, linkopt)

def get_external_header_dir(ctx, pkg_name, include):
    external = ctx.path("external_headers_" + pkg_name)
    ctx.symlink(ctx.path(include), external)
    return external.basename

def get_external_lib(ctx, pkg_name, linkdir, linkopt):
    external = ctx.path("external_libs_" + pkg_name)
    ctx.symlink(ctx.path(linkdir), external)
    return external.basename + "/lib" + linkopt + ".dylib"

# buildifier: disable=module-docstring
def _pkg_config_shared_impl(ctx):
    if ctx.which("pkg-config") == None:
        fail("pkg-config not found\n")
    
    pkg_name = ctx.attr.name
    if _execute_pkg_config(ctx, pkg_name, ["--exists"]) == None:
        fail("pkg-config '%s' not found" % (pkg_name))

    include = _pkg_config_include(ctx, pkg_name)
    linkdir = _pkg_config_linkdir(ctx, pkg_name)
    linkopt = _pkg_config_linkopt(ctx, pkg_name)
    external_lib = get_external_lib(ctx, pkg_name, linkdir, linkopt)
    external_header_dir = get_external_header_dir(ctx, pkg_name, include)
    
    ctx.template("BUILD", Label("//bazel:BUILD.tpl"), substitutions = {
        "%{external_pkg_name}": "external_" + pkg_name,
        "%{external_lib}": external_lib,
        "%{external_header_dir}": external_header_dir
    })

pkg_config_shared = repository_rule(
    attrs = {},
    local = True,
    implementation = _pkg_config_shared_impl
)