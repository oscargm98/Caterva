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

#include "test_common.h"

static void test_to_buffer(caterva_context_t *ctx, int8_t ndim, int8_t itemsize, int64_t *shape,
                           caterva_storage_backend_t backend, int64_t *chunkshape, int64_t *blockshape, bool enforceframe,
                           char* filename, void *result) {

    caterva_params_t params;
    params.itemsize = itemsize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = backend;
    switch (backend) {
        case CATERVA_STORAGE_PLAINBUFFER:
            break;
        case CATERVA_STORAGE_BLOSC:
            storage.properties.blosc.filename = filename;
            storage.properties.blosc.enforceframe = enforceframe;
            for (int i = 0; i < ndim; ++i) {
                storage.properties.blosc.chunkshape[i] = chunkshape[i];
                storage.properties.blosc.blockshape[i] = blockshape[i];
            }
            break;
        default:
            CATERVA_TEST_ERROR(CATERVA_ERR_INVALID_STORAGE);
    }

    /* Create original data */
    int64_t buffersize = itemsize;
    for (int i = 0; i < ndim; ++i) {
        buffersize *= shape[i];
    }
    printf("\n bufdest size %ld \n", (buffersize/itemsize));
    /* Create caterva_array_t with original data */
    caterva_array_t *src;
    CATERVA_TEST_ERROR(caterva_array_from_buffer(ctx, result, buffersize, &params, &storage, &src));

    /* Create dest buffer */
    uint8_t *destbuffer = malloc(buffersize);

    /* Fill dest buffer with a slice*/
    CATERVA_TEST_ERROR(caterva_array_to_buffer(ctx, src, destbuffer, buffersize));

    printf("\n to buffer \n");
    for (int i=0; i<(buffersize/itemsize); i++) {
        printf("%f, ", ((double*) destbuffer)[i]);
    }
    /* Assert results */
    assert_buf(destbuffer, result, itemsize, buffersize/itemsize, 1e-14);

    free(destbuffer);
    CATERVA_TEST_ERROR(caterva_array_free(ctx, &src));
}


LWTEST_DATA(to_buffer) {
    caterva_context_t *ctx;
};

LWTEST_SETUP(to_buffer) {
    caterva_config_t cfg = CATERVA_CONFIG_DEFAULTS;
    caterva_context_new(&cfg, &data->ctx);
}

LWTEST_TEARDOWN(to_buffer) {
    caterva_context_free(&data->ctx);
}

/*
LWTEST_FIXTURE(to_buffer, ndim_2) {
    const int8_t ndim = 2;
    int64_t shape_[] = {10, 10};
    int64_t pshape_[] = {2, 3};
    int64_t spshape_[] = {1, 2};

    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_,  enforceframe, filename, result);
    free(result);
}

LWTEST_FIXTURE(to_buffer, ndim_2_2) {
    const int8_t ndim = 2;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {5, 6};
    int64_t pshape_[] = {3, 3};
    int64_t spshape_[] = {2, 2};
    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size *itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
 }
*/
/*
LWTEST_FIXTURE(to_buffer, ndim_3_no_sp) {
    const int8_t ndim = 3;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {10, 10, 10};
    int64_t pshape_[] = {3, 5, 2};
    int64_t spshape_[] = {3, 3, 2};
    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
 }
*/
LWTEST_FIXTURE(to_buffer, ndim_3) {
    const int8_t ndim = 3;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {5, 6, 3};
    int64_t pshape_[] = {4, 3, 3};
    int64_t spshape_[] = {3, 3, 2};

    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
}
/*
LWTEST_FIXTURE(to_buffer, ndim_5_no_sp) {
    const int8_t ndim = 5;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {10, 10, 10, 10, 10};
    int64_t pshape_[] = {3, 5, 2, 4, 5};
    int64_t spshape_[] = {3, 3, 2, 3, 5};
    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
}

LWTEST_FIXTURE(to_buffer, ndim_6) {
    const int8_t ndim = 6;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {10, 10, 10, 10, 10, 10};
    int64_t pshape_[] = {3, 4, 2, 2, 2, 3};
    int64_t spshape_[] = {2, 3, 1, 2, 1, 2};
    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_,  enforceframe, filename, result);
    free(result);
 }

LWTEST_FIXTURE(to_buffer, ndim_7) {
    const int8_t ndim = 7;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {4, 5, 3, 4, 4, 5, 2};
    int64_t pshape_[] = {3, 4, 2, 2, 2, 3, 1};
    int64_t spshape_[] = {2, 3, 2, 2, 2, 3, 1};
    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }
    printf("bufsize %ld", buf_size);
    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
}*/
/*
LWTEST_FIXTURE(to_buffer, ndim_3_hard) {
    const int8_t ndim = 3;
    uint8_t itemsize = sizeof(double);
    caterva_storage_backend_t backend = CATERVA_STORAGE_BLOSC;
    bool enforceframe = false;
    char *filename = NULL;

    int64_t shape_[] = {252, 252, 252};
    int64_t pshape_[] = {64, 64, 64};
    int64_t spshape_[] = {16, 16, 16};

    int64_t buf_size = 1;
    for (int i = 0; i < ndim; ++i) {
        buf_size *= (shape_[i]);
    }
    double *result = (double *) malloc((size_t)buf_size * itemsize);
    for (int64_t i = 0; i < buf_size; ++i) {
        result[i] = (double) i;
    }

    test_to_buffer(data->ctx, ndim, itemsize, shape_, backend, pshape_, spshape_, enforceframe, filename, result);
    free(result);
 }
 */