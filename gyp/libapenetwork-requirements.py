from functools import partial
import deps

def buildOpenSSL():
    configure = "./config"
    if deps.system == "Darwin":
        configure = "./Configure darwin64-x86_64-cc -no-shared"

    deps.buildDep("libssl", "openssl", [configure, "make build_crypto", "make build_ssl"], outlibs=["openssl/libssl", "openssl/libcrypto"])

def registerDeps():
    deps.registerDep("openssl",
        partial(deps.downloadDep, "openssl", deps.depsURL + "/openssl-1.0.1g.tar.gz", "openssl"),
        buildOpenSSL)
