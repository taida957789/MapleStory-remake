// spine-c platform extension implementation
// Provides the 3 required callbacks: texture create/dispose, file read

#include <spine/extension.h>
#include <spine/Atlas.h>

#include <cstdio>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

void _spAtlasPage_createTexture(spAtlasPage* self, const char* /*path*/)
{
    // TODO: Load texture via WzCanvas / SDL and store in self->rendererObject
    self->rendererObject = nullptr;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
    // TODO: Release the texture stored in self->rendererObject
    self->rendererObject = nullptr;
}

char* _spUtil_readFile(const char* path, int* length)
{
    // Default implementation: read file from disk
    return _readFile(path, length);
}

#ifdef __cplusplus
}
#endif
