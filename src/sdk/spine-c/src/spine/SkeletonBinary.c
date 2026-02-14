/*
 * Spine binary skeleton reader for spine-c 2.1.25.
 * Custom Nexon addition - not part of the original 2.1.25 release.
 * Reconstructed from binary analysis of MapleStoryT v1029.
 *
 * Binary format (spine 2.1.x):
 *   - Integers: big-endian 4 bytes (readInt) or variable-length (readVarint)
 *   - Floats: big-endian 4 bytes reinterpreted as IEEE 754
 *   - Strings: varint length (char count + 1), followed by modified UTF-8 bytes
 *   - Colors: 4 bytes big-endian RGBA (each byte / 255.0)
 *   - Attachment types: 0=Region, 1=BoundingBox, 2=Mesh, 3=SkinnedMesh
 *   - Slot timeline types: 3=Attachment, 4=Color
 *   - Bone timeline types: 0=Scale, 1=Rotate, 2=Translate, 5=FlipX, 6=FlipY
 *   - Curve types: 0=Linear, 1=Stepped, 2=Bezier
 */

#include <spine/SkeletonBinary.h>
#include <spine/extension.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * BufferedStream - simple byte buffer reader
 * ---------------------------------------------------------------------------*/

typedef struct {
    const unsigned char* buffer;
    unsigned int length;
    unsigned int position;
} _BinaryInput;

/* ---------------------------------------------------------------------------
 * Primitive readers
 * ---------------------------------------------------------------------------*/

static unsigned char readByte (_BinaryInput* input) {
    if (input->position < input->length)
        return input->buffer[input->position++];
    return 0xFF;
}

static signed char readSByte (_BinaryInput* input) {
    return (signed char)readByte(input);
}

static int readBoolean (_BinaryInput* input) {
    return readByte(input) != 0;
}

/* Read 4-byte big-endian integer */
static int readInt (_BinaryInput* input) {
    unsigned char b0 = readByte(input);
    unsigned char b1 = readByte(input);
    unsigned char b2 = readByte(input);
    unsigned char b3 = readByte(input);
    return (int)(b3 | (b2 << 8) | (b1 << 16) | (b0 << 24));
}

/* Read variable-length integer (same encoding as spine 3.x readVarint) */
static int readVarint (_BinaryInput* input, int/*bool*/ optimizePositive) {
    unsigned char b = readByte(input);
    int value = b & 0x7F;
    if (b & 0x80) {
        b = readByte(input);
        value |= (b & 0x7F) << 7;
        if (b & 0x80) {
            b = readByte(input);
            value |= (b & 0x7F) << 14;
            if (b & 0x80) {
                b = readByte(input);
                value |= (b & 0x7F) << 21;
                if (b & 0x80)
                    value |= (readByte(input) & 0x7F) << 28;
            }
        }
    }
    if (!optimizePositive)
        value = (int)(((unsigned int)value >> 1) ^ (unsigned int)(-(value & 1)));
    return value;
}

/* Read 4-byte big-endian float */
static float readFloat (_BinaryInput* input) {
    union {
        int intValue;
        float floatValue;
    } u;
    u.intValue = readInt(input);
    return u.floatValue;
}

/* Read modified UTF-8 string. Returns malloc'd string, caller must FREE. */
static char* readString (_BinaryInput* input) {
    int length = readVarint(input, 1);
    char* string;
    if (length <= 1)
        return 0;
    length--;
    string = MALLOC(char, length + 1);
    memcpy(string, input->buffer + input->position, (size_t)length);
    input->position += (unsigned int)length;
    string[length] = '\0';
    return string;
}

/* Read color as 4-byte big-endian RGBA */
static void readColor (_BinaryInput* input, float* r, float* g, float* b, float* a) {
    int rgba = readInt(input);
    *r = (float)((rgba >> 24) & 0xFF) / 255.0f;
    *g = (float)((rgba >> 16) & 0xFF) / 255.0f;
    *b = (float)((rgba >> 8) & 0xFF) / 255.0f;
    *a = (float)(rgba & 0xFF) / 255.0f;
}

/* ---------------------------------------------------------------------------
 * Array readers
 * ---------------------------------------------------------------------------*/

static float* readFloatArray (_BinaryInput* input, int* outCount, float scale) {
    int n = readVarint(input, 1);
    float* arr;
    int i;
    *outCount = n;
    if (n <= 0) return 0;
    arr = MALLOC(float, n);
    if (scale == 1.0f) {
        for (i = 0; i < n; ++i)
            arr[i] = readFloat(input);
    } else {
        for (i = 0; i < n; ++i)
            arr[i] = readFloat(input) * scale;
    }
    return arr;
}

static int* readShortArray (_BinaryInput* input, int* outCount) {
    int n = readVarint(input, 1);
    int* arr;
    int i;
    *outCount = n;
    if (n <= 0) return 0;
    arr = MALLOC(int, n);
    for (i = 0; i < n; ++i) {
        unsigned char hi = readByte(input);
        unsigned char lo = readByte(input);
        arr[i] = (hi << 8) | lo;
    }
    return arr;
}

static int* readIntArray (_BinaryInput* input, int* outCount) {
    int n = readVarint(input, 1);
    int* arr;
    int i;
    *outCount = n;
    if (n <= 0) return 0;
    arr = MALLOC(int, n);
    for (i = 0; i < n; ++i)
        arr[i] = readVarint(input, 1);
    return arr;
}

/* ---------------------------------------------------------------------------
 * Curve reader
 * ---------------------------------------------------------------------------*/

static void readCurve (_BinaryInput* input, spCurveTimeline* timeline, int frameIndex) {
    unsigned char type = readByte(input);
    if (type == 1) {
        spCurveTimeline_setStepped(timeline, frameIndex);
    } else if (type == 2) {
        float cx1 = readFloat(input);
        float cy1 = readFloat(input);
        float cx2 = readFloat(input);
        float cy2 = readFloat(input);
        spCurveTimeline_setCurve(timeline, frameIndex, cx1, cy1, cx2, cy2);
    }
}

/* ---------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------------*/

static spSkin* readSkin (_BinaryInput* input, spSkeletonData* skeletonData,
                         const char* skinName, float scale,
                         spAttachmentLoader* attachmentLoader, int nonessential);

static spAttachment* readAttachment (_BinaryInput* input, spSkin* skin,
                                     const char* attachmentName, float scale,
                                     spAttachmentLoader* attachmentLoader, int nonessential);

static void readAnimation (_BinaryInput* input, const char* name,
                           spSkeletonData* skeletonData, int animationIndex, float scale);

/* Error reporting (defined in SkeletonJson.c) */
extern void _spSkeletonJson_setError (spSkeletonJson* self, void* root,
                                      const char* value1, const char* value2);

/* ---------------------------------------------------------------------------
 * ReadAttachment
 * ---------------------------------------------------------------------------*/

static spAttachment* readAttachment (_BinaryInput* input, spSkin* skin,
                                     const char* attachmentName, float scale,
                                     spAttachmentLoader* attachmentLoader, int nonessential) {
    char* name = readString(input);
    const char* useName;
    unsigned char type;
    spAttachment* attachment;

    if (!name || strlen(name) == 0) {
        FREE(name);
        useName = attachmentName;
    } else {
        useName = name;
    }

    type = readByte(input);

    switch (type) {
    case 0: { /* Region */
        char* path = readString(input);
        const char* usePath = (path && strlen(path) > 0) ? path : useName;
        spRegionAttachment* region;

        attachment = spAttachmentLoader_newAttachment(attachmentLoader, skin,
            SP_ATTACHMENT_REGION, attachmentName, usePath);
        if (!attachment) {
            FREE(name);
            FREE(path);
            return 0;
        }

        region = SUB_CAST(spRegionAttachment, attachment);
        MALLOC_STR(region->path, usePath);
        region->x = readFloat(input) * scale;
        region->y = readFloat(input) * scale;
        region->scaleX = readFloat(input);
        region->scaleY = readFloat(input);
        region->rotation = readFloat(input);
        region->width = readFloat(input) * scale;
        region->height = readFloat(input) * scale;
        readColor(input, &region->r, &region->g, &region->b, &region->a);
        spRegionAttachment_updateOffset(region);

        FREE(name);
        FREE(path);
        return SUPER(region);
    }
    case 1: { /* BoundingBox */
        int verticesCount;
        float* vertices;
        spBoundingBoxAttachment* box;

        attachment = spAttachmentLoader_newAttachment(attachmentLoader, skin,
            SP_ATTACHMENT_BOUNDING_BOX, attachmentName, 0);
        if (!attachment) {
            FREE(name);
            return 0;
        }

        box = SUB_CAST(spBoundingBoxAttachment, attachment);
        vertices = readFloatArray(input, &verticesCount, scale);
        CONST_CAST(int, box->verticesCount) = verticesCount;
        box->vertices = vertices;

        FREE(name);
        return SUPER(box);
    }
    case 2: { /* Mesh */
        char* path = readString(input);
        const char* usePath = (path && strlen(path) > 0) ? path : useName;
        spMeshAttachment* mesh;
        int count;

        attachment = spAttachmentLoader_newAttachment(attachmentLoader, skin,
            SP_ATTACHMENT_MESH, attachmentName, usePath);
        if (!attachment) {
            FREE(name);
            FREE(path);
            return 0;
        }

        mesh = SUB_CAST(spMeshAttachment, attachment);
        MALLOC_STR(mesh->path, usePath);

        /* regionUVs */
        mesh->regionUVs = readFloatArray(input, &count, 1.0f);
        /* triangles */
        mesh->triangles = readShortArray(input, &count);
        CONST_CAST(int, mesh->trianglesCount) = count;
        /* vertices */
        mesh->vertices = readFloatArray(input, &count, scale);
        CONST_CAST(int, mesh->verticesCount) = count;

        spMeshAttachment_updateUVs(mesh);

        readColor(input, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
        mesh->hullLength = readVarint(input, 1);

        if (nonessential) {
            mesh->edges = readIntArray(input, &count);
            CONST_CAST(int, mesh->edgesCount) = count;
            mesh->width = readFloat(input) * scale;
            mesh->height = readFloat(input) * scale;
        }

        FREE(name);
        FREE(path);
        return SUPER(mesh);
    }
    case 3: { /* SkinnedMesh */
        char* path = readString(input);
        const char* usePath = (path && strlen(path) > 0) ? path : useName;
        spSkinnedMeshAttachment* mesh;
        int uvsCount, trianglesCount, vertexCount;
        float* uvs;
        int* triangles;
        int i;

        /* Weights/bones capacity */
        int weightsCapacity, bonesCapacity;
        int weightsCount = 0, boneIndicesCount = 0;
        float* weights;
        int* bones;

        attachment = spAttachmentLoader_newAttachment(attachmentLoader, skin,
            SP_ATTACHMENT_SKINNED_MESH, attachmentName, usePath);
        if (!attachment) {
            FREE(name);
            FREE(path);
            return 0;
        }

        mesh = SUB_CAST(spSkinnedMeshAttachment, attachment);
        MALLOC_STR(mesh->path, usePath);

        /* UVs */
        uvs = readFloatArray(input, &uvsCount, 1.0f);
        /* Triangles */
        triangles = readShortArray(input, &trianglesCount);
        /* Vertex count */
        vertexCount = readVarint(input, 1);

        /* Pre-allocate with generous capacity */
        weightsCapacity = 9 * uvsCount;
        bonesCapacity = 3 * uvsCount;
        weights = MALLOC(float, weightsCapacity);
        bones = MALLOC(int, bonesCapacity);

        for (i = 0; i < vertexCount; ++i) {
            int boneCount = (int)readFloat(input);
            int j;

            /* Grow bones array if needed */
            if (boneIndicesCount + 1 + boneCount * 1 > bonesCapacity) {
                int newCap = bonesCapacity * 2;
                int* newBones = MALLOC(int, newCap);
                memcpy(newBones, bones, (size_t)boneIndicesCount * sizeof(int));
                FREE(bones);
                bones = newBones;
                bonesCapacity = newCap;
            }
            bones[boneIndicesCount++] = boneCount;

            for (j = 0; j < boneCount; ++j) {
                int boneIndex = (int)readFloat(input);
                float x = readFloat(input) * scale;
                float y = readFloat(input) * scale;
                float weight = readFloat(input);

                /* Grow bones array */
                if (boneIndicesCount + 1 > bonesCapacity) {
                    int newCap = bonesCapacity * 2;
                    int* newBones = MALLOC(int, newCap);
                    memcpy(newBones, bones, (size_t)boneIndicesCount * sizeof(int));
                    FREE(bones);
                    bones = newBones;
                    bonesCapacity = newCap;
                }
                bones[boneIndicesCount++] = boneIndex;

                /* Grow weights array */
                if (weightsCount + 3 > weightsCapacity) {
                    int newCap = weightsCapacity * 2;
                    float* newWeights = MALLOC(float, newCap);
                    memcpy(newWeights, weights, (size_t)weightsCount * sizeof(float));
                    FREE(weights);
                    weights = newWeights;
                    weightsCapacity = newCap;
                }
                weights[weightsCount++] = x;
                weights[weightsCount++] = y;
                weights[weightsCount++] = weight;
            }
        }

        mesh->bonesCount = boneIndicesCount;
        mesh->bones = bones;
        mesh->weightsCount = weightsCount;
        mesh->weights = weights;
        mesh->trianglesCount = trianglesCount;
        mesh->triangles = triangles;
        mesh->uvsCount = uvsCount;
        mesh->regionUVs = uvs;

        spSkinnedMeshAttachment_updateUVs(mesh);

        readColor(input, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
        mesh->hullLength = 2 * readVarint(input, 1);

        if (nonessential) {
            int edgesCount;
            mesh->edges = readIntArray(input, &edgesCount);
            mesh->edgesCount = edgesCount;
            mesh->width = readFloat(input) * scale;
            mesh->height = readFloat(input) * scale;
        }

        FREE(name);
        FREE(path);
        return SUPER(mesh);
    }
    default:
        FREE(name);
        return 0;
    }
}

/* ---------------------------------------------------------------------------
 * ReadSkin
 * ---------------------------------------------------------------------------*/

static spSkin* readSkin (_BinaryInput* input, spSkeletonData* skeletonData,
                         const char* skinName, float scale,
                         spAttachmentLoader* attachmentLoader, int nonessential) {
    int slotCount = readVarint(input, 1);
    int i;
    spSkin* skin;
    (void)skeletonData;

    if (slotCount == 0)
        return 0;

    skin = spSkin_create(skinName);

    for (i = 0; i < slotCount; ++i) {
        int slotIndex = readVarint(input, 1);
        int attachmentCount = readVarint(input, 1);
        int j;
        for (j = 0; j < attachmentCount; ++j) {
            char* attName = readString(input);
            spAttachment* att = readAttachment(input, skin, attName, scale, attachmentLoader, nonessential);
            if (att)
                spSkin_addAttachment(skin, slotIndex, attName, att);
            FREE(attName);
        }
    }

    return skin;
}

/* ---------------------------------------------------------------------------
 * ReadAnimation
 * ---------------------------------------------------------------------------*/

/* Max timelines capacity (grows dynamically) */
#define INITIAL_TIMELINE_CAPACITY 64

typedef struct {
    spTimeline** items;
    int count;
    int capacity;
} _TimelineArray;

static void _timelineArray_init (_TimelineArray* arr) {
    arr->count = 0;
    arr->capacity = INITIAL_TIMELINE_CAPACITY;
    arr->items = MALLOC(spTimeline*, arr->capacity);
}

static void _timelineArray_push (_TimelineArray* arr, spTimeline* timeline) {
    if (arr->count == arr->capacity) {
        spTimeline** newItems;
        arr->capacity *= 2;
        newItems = MALLOC(spTimeline*, arr->capacity);
        memcpy(newItems, arr->items, (size_t)arr->count * sizeof(spTimeline*));
        FREE(arr->items);
        arr->items = newItems;
    }
    arr->items[arr->count++] = timeline;
}

static void readAnimation (_BinaryInput* input, const char* name,
                           spSkeletonData* skeletonData, int animationIndex, float scale) {
    _TimelineArray timelines;
    float duration = 0;
    int i, n, ii, nn;
    spAnimation* animation;

    _timelineArray_init(&timelines);

    /* Slot timelines */
    n = readVarint(input, 1);
    for (i = 0; i < n; ++i) {
        int slotIndex = readVarint(input, 1);
        nn = readVarint(input, 1);
        for (ii = 0; ii < nn; ++ii) {
            unsigned char timelineType = readByte(input);
            int frameCount = readVarint(input, 1);

            if (timelineType == 3) { /* Attachment */
                spAttachmentTimeline* timeline = spAttachmentTimeline_create(frameCount);
                int frameIndex;
                timeline->slotIndex = slotIndex;
                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    char* attName = readString(input);
                    spAttachmentTimeline_setFrame(timeline, frameIndex, time, attName);
                    FREE(attName);
                }
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[frameCount - 1];
                    if (lastTime > duration) duration = lastTime;
                }
            } else if (timelineType == 4) { /* Color */
                spColorTimeline* timeline = spColorTimeline_create(frameCount);
                int frameIndex;
                timeline->slotIndex = slotIndex;
                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    float r, g, b, a;
                    readColor(input, &r, &g, &b, &a);
                    spColorTimeline_setFrame(timeline, frameIndex, time, r, g, b, a);
                    if (frameIndex < frameCount - 1)
                        readCurve(input, SUPER(timeline), frameIndex);
                }
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[5 * frameCount - 5];
                    if (lastTime > duration) duration = lastTime;
                }
            }
        }
    }

    /* Bone timelines */
    n = readVarint(input, 1);
    for (i = 0; i < n; ++i) {
        int boneIndex = readVarint(input, 1);
        nn = readVarint(input, 1);
        for (ii = 0; ii < nn; ++ii) {
            unsigned char timelineType = readByte(input);
            int frameCount = readVarint(input, 1);

            if (timelineType == 1) { /* Rotate */
                spRotateTimeline* timeline = spRotateTimeline_create(frameCount);
                int frameIndex;
                timeline->boneIndex = boneIndex;
                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    float angle = readFloat(input);
                    spRotateTimeline_setFrame(timeline, frameIndex, time, angle);
                    if (frameIndex < frameCount - 1)
                        readCurve(input, SUPER(timeline), frameIndex);
                }
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[2 * frameCount - 2];
                    if (lastTime > duration) duration = lastTime;
                }
            } else if (timelineType == 0 || timelineType == 2) { /* Scale or Translate */
                spTranslateTimeline* timeline;
                int frameIndex;
                float tlScale = 1.0f;
                if (timelineType == 2) {
                    timeline = spTranslateTimeline_create(frameCount);
                    tlScale = scale;
                } else {
                    timeline = (spTranslateTimeline*)spScaleTimeline_create(frameCount);
                }
                timeline->boneIndex = boneIndex;
                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    float x = readFloat(input) * tlScale;
                    float y = readFloat(input) * tlScale;
                    spTranslateTimeline_setFrame(timeline, frameIndex, time, x, y);
                    if (frameIndex < frameCount - 1)
                        readCurve(input, SUPER(timeline), frameIndex);
                }
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[3 * frameCount - 3];
                    if (lastTime > duration) duration = lastTime;
                }
            } else if (timelineType == 5 || timelineType == 6) { /* FlipX or FlipY */
                spFlipTimeline* timeline = spFlipTimeline_create(frameCount, timelineType == 5);
                int frameIndex;
                timeline->boneIndex = boneIndex;
                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    int flip = readBoolean(input);
                    spFlipTimeline_setFrame(timeline, frameIndex, time, flip);
                }
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[2 * frameCount - 2];
                    if (lastTime > duration) duration = lastTime;
                }
            }
        }
    }

    /* IK constraint timelines */
    n = readVarint(input, 1);
    for (i = 0; i < n; ++i) {
        int ikIndex = readVarint(input, 1);
        int frameCount = readVarint(input, 1);
        spIkConstraintTimeline* timeline = spIkConstraintTimeline_create(frameCount);
        int frameIndex;
        timeline->ikConstraintIndex = ikIndex;
        for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
            float time = readFloat(input);
            float mix = readFloat(input);
            signed char bendDirection = readSByte(input);
            spIkConstraintTimeline_setFrame(timeline, frameIndex, time, mix, bendDirection);
            if (frameIndex < frameCount - 1)
                readCurve(input, SUPER(timeline), frameIndex);
        }
        _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
        if (frameCount > 0) {
            float lastTime = timeline->frames[3 * frameCount - 3];
            if (lastTime > duration) duration = lastTime;
        }
    }

    /* FFD timelines */
    n = readVarint(input, 1);
    for (i = 0; i < n; ++i) {
        int skinIndex = readVarint(input, 1);
        spSkin* skin = skeletonData->skins[skinIndex];
        int slotCount = readVarint(input, 1);
        int si;
        for (si = 0; si < slotCount; ++si) {
            int slotIndex = readVarint(input, 1);
            int attCount = readVarint(input, 1);
            int ai;
            for (ai = 0; ai < attCount; ++ai) {
                char* attName = readString(input);
                const char* lookupName = attName;
                spAttachment* attachment;
                int verticesCount = 0;
                int frameCount;
                spFFDTimeline* timeline;
                float* tempVertices;
                int frameIndex;

                if (!lookupName) lookupName = "";
                attachment = spSkin_getAttachment(skin, slotIndex, lookupName);

                if (attachment) {
                    if (attachment->type == SP_ATTACHMENT_MESH) {
                        verticesCount = SUB_CAST(spMeshAttachment, attachment)->verticesCount;
                    } else if (attachment->type == SP_ATTACHMENT_SKINNED_MESH) {
                        verticesCount = SUB_CAST(spSkinnedMeshAttachment, attachment)->weightsCount / 3 * 2;
                    }
                }

                frameCount = readVarint(input, 1);
                timeline = spFFDTimeline_create(frameCount, verticesCount);
                timeline->slotIndex = slotIndex;
                timeline->attachment = attachment;

                tempVertices = CALLOC(float, verticesCount);

                for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    float time = readFloat(input);
                    int end = readVarint(input, 1);

                    if (end == 0) {
                        /* No deformation: use base mesh vertices for mesh type, zeros otherwise */
                        if (attachment && attachment->type == SP_ATTACHMENT_MESH) {
                            float* meshVerts = SUB_CAST(spMeshAttachment, attachment)->vertices;
                            memcpy(tempVertices, meshVerts, (size_t)verticesCount * sizeof(float));
                        } else {
                            memset(tempVertices, 0, (size_t)verticesCount * sizeof(float));
                        }
                    } else {
                        int start = readVarint(input, 1);
                        int vi;
                        memset(tempVertices, 0, (size_t)start * sizeof(float));
                        if (scale == 1.0f) {
                            for (vi = start; vi < start + end; ++vi)
                                tempVertices[vi] = readFloat(input);
                        } else {
                            for (vi = start; vi < start + end; ++vi)
                                tempVertices[vi] = readFloat(input) * scale;
                        }
                        memset(tempVertices + start + end, 0,
                               (size_t)(verticesCount - start - end) * sizeof(float));

                        /* For mesh type, add base vertices */
                        if (attachment && attachment->type == SP_ATTACHMENT_MESH) {
                            float* meshVerts = SUB_CAST(spMeshAttachment, attachment)->vertices;
                            for (vi = 0; vi < verticesCount; ++vi)
                                tempVertices[vi] += meshVerts[vi];
                        }
                    }

                    spFFDTimeline_setFrame(timeline, frameIndex, time, tempVertices);
                    if (frameIndex < frameCount - 1)
                        readCurve(input, SUPER(timeline), frameIndex);
                }

                FREE(tempVertices);
                _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
                if (frameCount > 0) {
                    float lastTime = timeline->frames[frameCount - 1];
                    if (lastTime > duration) duration = lastTime;
                }

                FREE(attName);
            }
        }
    }

    /* Draw order timelines */
    n = readVarint(input, 1);
    if (n > 0) {
        spDrawOrderTimeline* timeline = spDrawOrderTimeline_create(n, skeletonData->slotsCount);
        for (i = 0; i < n; ++i) {
            int offsetCount = readVarint(input, 1);
            int* drawOrder = MALLOC(int, skeletonData->slotsCount);
            int* unchanged = MALLOC(int, skeletonData->slotsCount - offsetCount);
            int originalIndex = 0, unchangedIndex = 0;
            int oi, di;
            float time;

            for (di = skeletonData->slotsCount - 1; di >= 0; --di)
                drawOrder[di] = -1;

            for (oi = 0; oi < offsetCount; ++oi) {
                int slotIdx = readVarint(input, 1);
                /* Fill unchanged entries before this slot */
                while (originalIndex != slotIdx)
                    unchanged[unchangedIndex++] = originalIndex++;
                drawOrder[originalIndex + readVarint(input, 1)] = originalIndex;
                ++originalIndex;
            }

            /* Fill remaining unchanged */
            while (originalIndex < skeletonData->slotsCount)
                unchanged[unchangedIndex++] = originalIndex++;

            /* Fill gaps in drawOrder with unchanged entries (reverse) */
            for (di = skeletonData->slotsCount - 1; di >= 0; --di) {
                if (drawOrder[di] == -1)
                    drawOrder[di] = unchanged[--unchangedIndex];
            }

            time = readFloat(input);
            spDrawOrderTimeline_setFrame(timeline, i, time, drawOrder);
            FREE(drawOrder);
            FREE(unchanged);
        }
        _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
        {
            float lastTime = timeline->frames[n - 1];
            if (lastTime > duration) duration = lastTime;
        }
    }

    /* Event timelines */
    n = readVarint(input, 1);
    if (n > 0) {
        spEventTimeline* timeline = spEventTimeline_create(n);
        for (i = 0; i < n; ++i) {
            float time = readFloat(input);
            int eventIndex = readVarint(input, 1);
            spEventData* eventData = skeletonData->events[eventIndex];
            spEvent* event = spEvent_create(eventData);

            event->intValue = readVarint(input, 0);
            event->floatValue = readFloat(input);

            if (readBoolean(input)) {
                /* Has custom string */
                char* str = readString(input);
                if (str && strlen(str) > 0) {
                    MALLOC_STR(event->stringValue, str);
                }
                FREE(str);
            } else {
                /* Use default string from event data */
                if (eventData->stringValue) {
                    MALLOC_STR(event->stringValue, eventData->stringValue);
                }
            }

            spEventTimeline_setFrame(timeline, i, time, event);
        }
        _timelineArray_push(&timelines, SUPER_CAST(spTimeline, timeline));
        {
            float lastTime = timeline->frames[n - 1];
            if (lastTime > duration) duration = lastTime;
        }
    }

    /* Build animation */
    animation = spAnimation_create(name, timelines.count);
    animation->duration = duration;
    for (i = 0; i < timelines.count; ++i)
        animation->timelines[i] = timelines.items[i];

    skeletonData->animations[animationIndex] = animation;

    FREE(timelines.items);
}

/* ---------------------------------------------------------------------------
 * Main Read function
 * ---------------------------------------------------------------------------*/

spSkeletonData* spSkeletonBinary_readSkeletonData (spSkeletonJson* self, const void* binary, unsigned int length) {
    _BinaryInput input;
    spSkeletonData* skeletonData;
    float scale = self->scale;
    spAttachmentLoader* attachmentLoader = self->attachmentLoader;
    int nonessential;
    int i, n;

    input.buffer = (const unsigned char*)binary;
    input.length = length;
    input.position = 0;

    skeletonData = spSkeletonData_create();

    /* Hash */
    skeletonData->hash = readString(&input);

    /* Version */
    skeletonData->version = readString(&input);

    /* Dimensions */
    skeletonData->width = readFloat(&input);
    skeletonData->height = readFloat(&input);

    /* Nonessential */
    nonessential = readBoolean(&input);
    if (nonessential) {
        /* Images path - skip */
        char* imagesPath = readString(&input);
        FREE(imagesPath);
    }

    /* Bones */
    n = readVarint(&input, 1);
    skeletonData->bonesCount = n;
    skeletonData->bones = MALLOC(spBoneData*, n);
    for (i = 0; i < n; ++i) {
        char* boneName = readString(&input);
        int parentIndex = readVarint(&input, 1) - 1;
        spBoneData* parent = (parentIndex >= 0) ? skeletonData->bones[parentIndex] : 0;
        spBoneData* bone = spBoneData_create(boneName, parent);

        bone->x = readFloat(&input) * scale;
        bone->y = readFloat(&input) * scale;
        bone->scaleX = readFloat(&input);
        bone->scaleY = readFloat(&input);
        bone->rotation = readFloat(&input);
        bone->length = readFloat(&input) * scale;
        bone->flipX = readBoolean(&input);
        bone->flipY = readBoolean(&input);
        bone->inheritScale = readBoolean(&input);
        bone->inheritRotation = readBoolean(&input);

        if (nonessential)
            readInt(&input); /* color - skip */

        skeletonData->bones[i] = bone;
        FREE(boneName);
    }

    /* IK constraints */
    n = readVarint(&input, 1);
    skeletonData->ikConstraintsCount = n;
    skeletonData->ikConstraints = MALLOC(spIkConstraintData*, n);
    for (i = 0; i < n; ++i) {
        char* ikName = readString(&input);
        spIkConstraintData* ik = spIkConstraintData_create(ikName);
        int boneCount = readVarint(&input, 1);
        int j;
        FREE(ikName);

        ik->bonesCount = boneCount;
        ik->bones = MALLOC(spBoneData*, boneCount);
        for (j = 0; j < boneCount; ++j)
            ik->bones[j] = skeletonData->bones[readVarint(&input, 1)];

        ik->target = skeletonData->bones[readVarint(&input, 1)];
        ik->mix = readFloat(&input);
        ik->bendDirection = readSByte(&input);

        skeletonData->ikConstraints[i] = ik;
    }

    /* Slots */
    n = readVarint(&input, 1);
    skeletonData->slotsCount = n;
    skeletonData->slots = MALLOC(spSlotData*, n);
    for (i = 0; i < n; ++i) {
        char* slotName = readString(&input);
        int boneIndex = readVarint(&input, 1);
        spSlotData* slot = spSlotData_create(slotName, skeletonData->bones[boneIndex]);
        char* attName;
        FREE(slotName);

        readColor(&input, &slot->r, &slot->g, &slot->b, &slot->a);

        attName = readString(&input);
        if (attName && strlen(attName) > 0)
            MALLOC_STR(slot->attachmentName, attName);
        FREE(attName);

        /* Nexon binary stores blendMode (0=normal, 1=additive, 2=multiply, 3=screen).
         * spine-c 2.1.25 only has additiveBlending bool. */
        slot->additiveBlending = (readVarint(&input, 1) == 1) ? 1 : 0;

        skeletonData->slots[i] = slot;
    }

    /* Default skin */
    {
        spSkin* defaultSkin = readSkin(&input, skeletonData, "default", scale, attachmentLoader, nonessential);
        if (defaultSkin)
            skeletonData->defaultSkin = defaultSkin;
    }

    /* Additional skins */
    {
        int skinsCount = readVarint(&input, 1) + 1;
        skeletonData->skinsCount = skinsCount;
        skeletonData->skins = MALLOC(spSkin*, skinsCount);
        skeletonData->skins[0] = skeletonData->defaultSkin;
        for (i = 1; i < skinsCount; ++i) {
            char* skinName = readString(&input);
            skeletonData->skins[i] = readSkin(&input, skeletonData, skinName, scale, attachmentLoader, nonessential);
            FREE(skinName);
        }
    }

    /* Events */
    n = readVarint(&input, 1);
    skeletonData->eventsCount = n;
    skeletonData->events = MALLOC(spEventData*, n);
    for (i = 0; i < n; ++i) {
        char* eventName = readString(&input);
        spEventData* eventData = spEventData_create(eventName);
        FREE(eventName);

        eventData->intValue = readVarint(&input, 0);
        eventData->floatValue = readFloat(&input);

        {
            char* str = readString(&input);
            if (str && strlen(str) > 0)
                MALLOC_STR(eventData->stringValue, str);
            FREE(str);
        }

        skeletonData->events[i] = eventData;
    }

    /* Animations */
    n = readVarint(&input, 1);
    skeletonData->animationsCount = n;
    skeletonData->animations = MALLOC(spAnimation*, n);
    for (i = 0; i < n; ++i) {
        char* animName = readString(&input);
        readAnimation(&input, animName, skeletonData, i, scale);
        FREE(animName);
    }

    return skeletonData;
}

/* ---------------------------------------------------------------------------
 * File reader
 * ---------------------------------------------------------------------------*/

spSkeletonData* spSkeletonJson_readSkeletonBinaryFile (spSkeletonJson* self, const char* path) {
    int length;
    spSkeletonData* skeletonData;
    char* data = _spUtil_readFile(path, &length);
    if (!data) {
        _spSkeletonJson_setError(self, 0, "Unable to read skeleton file: ", path);
        return 0;
    }
    skeletonData = spSkeletonBinary_readSkeletonData(self, data, (unsigned int)length);
    FREE(data);
    return skeletonData;
}
