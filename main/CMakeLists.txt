#file(GLOB_RECURSE MENJIN_SOURCES ./*.c)

idf_component_register(
        SRC_DIRS
        "."
        "system"
        "http"

        INCLUDE_DIRS
        "."
        "system"
        "http"

        EMBED_TXTFILES server_root_cert.pem
#        REQUIRES esp-wolfssl esp-tls
)