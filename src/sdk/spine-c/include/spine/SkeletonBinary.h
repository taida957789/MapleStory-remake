/*
 * Spine binary skeleton reader for spine-c 2.1.25.
 * Custom Nexon addition - not part of the original 2.1.25 release.
 * Reconstructed from binary analysis of MapleStoryT v1029.
 *
 * Reuses spSkeletonJson for configuration (scale, attachmentLoader)
 * since the struct layout is identical to what the binary reader needs.
 */

#ifndef SPINE_SKELETONBINARY_H_
#define SPINE_SKELETONBINARY_H_

#include <spine/SkeletonJson.h>
#include <spine/SkeletonData.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reads skeleton data from a binary buffer.
 * @param self Provides scale and attachmentLoader configuration (reuses spSkeletonJson struct).
 * @param binary Pointer to the binary skeleton data.
 * @param length Length of the binary data in bytes.
 * @return The skeleton data, or NULL on failure. */
spSkeletonData* spSkeletonBinary_readSkeletonData (spSkeletonJson* self, const void* binary, unsigned int length);

/* Reads skeleton data from a binary file on disk.
 * @param self Provides scale and attachmentLoader configuration.
 * @param path File path to the binary skeleton data (.skel file).
 * @return The skeleton data, or NULL on failure. */
spSkeletonData* spSkeletonJson_readSkeletonBinaryFile (spSkeletonJson* self, const char* path);

#ifdef __cplusplus
}
#endif

#endif /* SPINE_SKELETONBINARY_H_ */
