# edk2-staging - Intel/HttpProxy branch

Staging branch for HTTP proxy feature intercept for Intel product. Based on code-first edk2-staging/HttpProxy branch.

Relevant BugZilla links:
- https://bugzilla.tianocore.org/show_bug.cgi?id=3951

## Scope of required work
1. MdeModulePkg/UefiBootManagerLib - adjust matching of HTTP boot device path
2. NetworkPkg/HttpDxe - support for HTTP CONNECT requests, TLS adjustments
3. NetworkPkg/HttpBootDxe - device path parsing, establishing connection with proxy
