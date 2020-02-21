/*
 * Copyright (C) 2018 Francesc Alted, Aleix Alcacer.
 * Copyright (C) 2019-present Blosc Development team <blosc@blosc.org>
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#include "caterva.h"
#include <string.h>
#include <assert.h>
#include "caterva_blosc.h"
#include "caterva_plainbuffer.h"



int caterva_context_new(caterva_config_t *cfg, caterva_context_t **ctx) {
    CATERVA_ERROR_NULL(cfg);
    CATERVA_ERROR_NULL(ctx);

    (*ctx) = (caterva_context_t *) cfg->alloc(sizeof(caterva_context_t));
    if (!(*ctx)) {
        DEBUG_PRINT("Allocation fails");
        return CATERVA_ERR_NULL_POINTER;
    }

    (*ctx)->cfg = (caterva_config_t *) cfg->alloc(sizeof(caterva_config_t));
    if (!(*ctx)) {
        DEBUG_PRINT("Allocation fails");
        return CATERVA_ERR_NULL_POINTER;
    }

    return CATERVA_SUCCEED;
}


int caterva_context_free(caterva_context_t **ctx) {
    CATERVA_ERROR_NULL(ctx);

    void (*auxfree)(void *) = (*ctx)->cfg->free;
    auxfree((*ctx)->cfg);
    auxfree(*ctx);

    return CATERVA_SUCCEED;
}


int caterva_array_empty(caterva_context_t *ctx, caterva_params_t *params, caterva_storage_t *storage,
                        caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(params);
    CATERVA_ERROR_NULL(storage);
    CATERVA_ERROR_NULL(array);

    if (storage->backend == CATERVA_STORAGE_BLOSC) {
        CATERVA_ERROR(caterva_blosc_array_empty(ctx, params, storage, array));
    } else {
        CATERVA_ERROR(caterva_plainbuffer_array_empty(ctx, params, storage, array));
    }

    (*array)->filled = false;
    (*array)->nparts = 0;

    return CATERVA_SUCCEED;
}


int caterva_array_from_frame(caterva_context_t *ctx, blosc2_frame *frame, bool copy, caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(frame);
    CATERVA_ERROR_NULL(array);

    CATERVA_ERROR(caterva_blosc_from_frame(ctx, frame, copy, array));
    if ((*array) == NULL) {
        DEBUG_PRINT("Error creating a caterva container from a frame");
        return CATERVA_ERR_NULL_POINTER;
    }
    return CATERVA_SUCCEED;
}


int caterva_array_from_sframe(caterva_context_t *ctx, uint8_t *sframe, int64_t len, bool copy, caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(sframe);
    CATERVA_ERROR_NULL(array);

    CATERVA_ERROR(caterva_blosc_from_sframe(ctx, sframe, len, copy, array));

    return CATERVA_SUCCEED;
}


int caterva_array_from_file(caterva_context_t *ctx, const char *filename, bool copy, caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(filename);
    CATERVA_ERROR_NULL(array);

    CATERVA_ERROR(caterva_blosc_from_file(ctx, filename, copy, array));

    return CATERVA_SUCCEED;
}


int caterva_array_free(caterva_context_t *ctx, caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(array);

    if (*array) {
        switch ((*array)->storage) {
            case CATERVA_STORAGE_BLOSC:caterva_blosc_array_free(ctx, array);
                break;
            case CATERVA_STORAGE_PLAINBUFFER:caterva_plainbuffer_array_free(ctx, array);
                break;
        }
        ctx->cfg->free(*array);
    }
    return CATERVA_SUCCEED;
}

/*
int caterva_update_shape(caterva_array_t *carr, caterva_dims_t *shape) {
    CATERVA_ERROR_NULL(carr);
    CATERVA_ERROR_NULL(shape);

    int rc;
    carr->empty = false;
    switch (carr->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_update_shape(carr, shape);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_update_shape(carr, shape);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}
*/

int caterva_array_append(caterva_context_t *ctx, caterva_array_t *array, void *chunk, int64_t chunksize) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(array);
    CATERVA_ERROR_NULL(chunk);

    if (chunksize != (int64_t) array->chunksize * array->itemsize) {
        CATERVA_ERROR(CATERVA_ERR_INVALID_ARGUMENT);
    }
    if (array->filled) {
        CATERVA_ERROR(CATERVA_ERR_CONTAINER_FILLED);
    }
    switch (array->storage) {
        case CATERVA_STORAGE_BLOSC:
            CATERVA_ERROR(caterva_blosc_array_append(ctx, array, chunk, chunksize));
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            CATERVA_ERROR(caterva_plainbuffer_array_append(ctx, array, chunk, chunksize));
            break;
        default:
            CATERVA_ERROR(CATERVA_ERR_INVALID_STORAGE);
    }

    array->nparts++;
    if (array->nparts == array->extendedesize / array->chunksize) {
        array->filled = true;
    }

    return CATERVA_SUCCEED;
}


int caterva_array_from_buffer(caterva_context_t *ctx, caterva_params_t *params, caterva_storage_t *storage,
                              void *buffer, int64_t buffersize, caterva_array_t **array) {
    CATERVA_ERROR_NULL(ctx);
    CATERVA_ERROR_NULL(params);
    CATERVA_ERROR_NULL(storage);
    CATERVA_ERROR_NULL(buffer);
    CATERVA_ERROR_NULL(array);

    CATERVA_ERROR(caterva_array_empty(ctx, params, storage, array));

    if (buffersize != (int64_t) (*array)->size * (*array)->itemsize) {
        CATERVA_ERROR(CATERVA_ERR_INVALID_ARGUMENT);
    }

    switch ((*array)->storage) {
        case CATERVA_STORAGE_BLOSC:
            CATERVA_ERROR(caterva_blosc_array_from_buffer(ctx, *array, buffer, buffersize));
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            CATERVA_ERROR(caterva_plainbuffer_array_from_buffer(ctx, *array, buffer, buffersize));
            break;
        default:
            CATERVA_ERROR(CATERVA_ERR_INVALID_STORAGE);
    }

    return CATERVA_SUCCEED;
}


int caterva_array_to_buffer(caterva_array_t *src, void *dest) {
    CATERVA_ERROR_NULL(dest);
    CATERVA_ERROR_NULL(src);

    int rc;
    switch (src->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_to_buffer(src, dest);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_to_buffer(src, dest);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


int caterva_array_get_slice_buffer(void *dest, caterva_array_t *src, caterva_dims_t *start,
                                   caterva_dims_t *stop, caterva_dims_t *d_pshape) {
    CATERVA_ERROR_NULL(dest);
    CATERVA_ERROR_NULL(src);
    CATERVA_ERROR_NULL(start);
    CATERVA_ERROR_NULL(stop);
    CATERVA_ERROR_NULL(d_pshape);

    int rc;
    switch (src->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_get_slice_buffer(dest, src, start, stop, d_pshape);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_get_slice_buffer(dest, src, start, stop, d_pshape);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


int caterva_array_get_slice_buffer_no_copy(void **dest, caterva_array_t *src, caterva_dims_t *start,
                                           caterva_dims_t *stop, caterva_dims_t *d_pshape) {
    CATERVA_UNUSED_PARAM(d_pshape);
    int64_t start_[CATERVA_MAXDIM];
    int64_t stop_[CATERVA_MAXDIM];
    int8_t s_ndim = src->ndim;

    caterva_dims_t shape = caterva_get_shape(src);
    int64_t s_shape[CATERVA_MAXDIM];
    for (int i = 0; i < CATERVA_MAXDIM; ++i) {
        start_[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = start->dims[i];
        stop_[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = stop->dims[i];
        s_shape[(CATERVA_MAXDIM - s_ndim + i) % CATERVA_MAXDIM] = shape.dims[i];
    }
    for (int j = 0; j < CATERVA_MAXDIM - s_ndim; ++j) {
        start_[j] = 0;
    }

    int64_t chunk_pointer = 0;
    int64_t chunk_pointer_inc = 1;
    for (int i = CATERVA_MAXDIM - 1; i >= 0; --i) {
        chunk_pointer += start_[i] * chunk_pointer_inc;
        chunk_pointer_inc *= s_shape[i];
    }
    *dest = &src->buf[chunk_pointer * src->ctx->cparams.typesize];

    return 0;
}


int caterva_array_set_slice_buffer(caterva_array_t *dest, void *src, caterva_dims_t *start,
                                   caterva_dims_t *stop) {
    CATERVA_ERROR_NULL(dest);
    CATERVA_ERROR_NULL(src);
    CATERVA_ERROR_NULL(start);
    CATERVA_ERROR_NULL(stop);

    int rc;
    switch (dest->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = CATERVA_ERR_INVALID_STORAGE;
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_set_slice_buffer(dest, src, start, stop);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


int caterva_array_get_slice(caterva_array_t *dest, caterva_array_t *src, caterva_dims_t *start,
                            caterva_dims_t *stop) {
    CATERVA_ERROR_NULL(dest);
    CATERVA_ERROR_NULL(src);
    CATERVA_ERROR_NULL(start);
    CATERVA_ERROR_NULL(stop);

    if (start->ndim != stop->ndim) {
        CATERVA_ERROR(CATERVA_ERR_INVALID_ARGUMENT);
    }
    if (start->ndim != src->ndim) {
        CATERVA_ERROR(CATERVA_ERR_INVALID_ARGUMENT);
    }

    int rc;
    int64_t shape_[CATERVA_MAXDIM];
    for (int i = 0; i < start->ndim; ++i) {
        shape_[i] = stop->dims[i] - start->dims[i];
    }
    for (int i = (int) start->ndim; i < CATERVA_MAXDIM; ++i) {
        shape_[i] = 1;
        start->dims[i] = 0;
    }
    caterva_dims_t shape = caterva_new_dims(shape_, start->ndim);
    rc  = caterva_update_shape(dest, &shape);
    CATERVA_ERROR(rc);

    switch (dest->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_get_slice(dest, src, start, stop);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_get_slice(dest, src, start, stop);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


int caterva_array_squeeze(caterva_array_t *src) {
    CATERVA_ERROR_NULL(src);

    int rc;
    switch (src->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_squeeze(src);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_squeeze(src);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


int caterva_copy(caterva_array_t *dest, caterva_array_t *src) {
    CATERVA_ERROR_NULL(dest);
    CATERVA_ERROR_NULL(src);

    int rc;
    switch (dest->storage) {
        case CATERVA_STORAGE_BLOSC:
            rc = caterva_blosc_copy(dest, src);
            break;
        case CATERVA_STORAGE_PLAINBUFFER:
            rc = caterva_plainbuffer_copy(dest, src);
            break;
        default:
            rc = CATERVA_ERR_INVALID_STORAGE;
    }
    CATERVA_ERROR(rc);

    return rc;
}


caterva_dims_t caterva_get_shape(caterva_array_t *src){
    caterva_dims_t shape = caterva_new_dims(src->shape, src->ndim);
    return shape;
}


caterva_dims_t caterva_get_pshape(caterva_array_t *src) {
    caterva_dims_t pshape;
    for (int i = 0; i < src->ndim; ++i) {
        pshape.dims[i] = src->chunkshape[i];
    }
    pshape.ndim = src->ndim;
    return pshape;
}
