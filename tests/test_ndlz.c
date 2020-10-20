/*
    Copyright (C) 2014  Francesc Alted
    http://blosc.org
    License: BSD 3-Clause (see LICENSE.txt)

    Example program demonstrating use of the Blosc filter from C code.

    To compile this program:

    $ gcc -O many_compressors.c -o many_compressors -lblosc2

    To run:

    $ ./test_ndlz
    Blosc version info: 2.0.0a6.dev ($Date:: 2018-05-18 #$)
    Using 4 threads (previously using 1)
    Using blosclz compressor
    Compression: 4000000 -> 57577 (69.5x)
    Succesful roundtrip!
    Using lz4 compressor
    Compression: 4000000 -> 97276 (41.1x)
    Succesful roundtrip!
    Using lz4hc compressor
    Compression: 4000000 -> 38314 (104.4x)
    Succesful roundtrip!
    Using zlib compressor
    Compression: 4000000 -> 21486 (186.2x)
    Succesful roundtrip!
    Using zstd compressor
    Compression: 4000000 -> 10692 (374.1x)
    Succesful roundtrip!

 */

#include <stdio.h>
#include <blosc2.h>
#include <ndlz.h>
#include <ndlz.c>
#include "test_common.h"

#define SHAPE1 32
#define SHAPE2 32
#define SIZE SHAPE1 * SHAPE2
#define SHAPE {SHAPE1, SHAPE2}
#define OSIZE (17 * SIZE / 16) + 9 + 8 + BLOSC_MAX_OVERHEAD

static int test_ndlz(void *data, int nbytes, int typesize, int ndim, caterva_params_t params, caterva_storage_t storage) {

    uint8_t *data2 = (uint8_t*) data;
    caterva_array_t *array;
    caterva_context_t *ctx;
    caterva_config_t cfg = CATERVA_CONFIG_DEFAULTS;
    caterva_context_new(&cfg, &ctx);
    caterva_array_from_buffer(ctx, data2, nbytes, &params, &storage, &array);

    int isize = (int) array->sc->chunksize;
    uint8_t *data_in = malloc(isize);
    blosc2_schunk_decompress_chunk(array->sc, 0, data_in, isize);

    int32_t *blockshape = storage.properties.blosc.blockshape;
    int osize = (17 * isize / 16) + 9 + 8 + BLOSC_MAX_OVERHEAD;
    int dsize = isize;
    int csize;
    uint8_t data_out[osize];
    uint8_t data_dest[dsize];
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;

    /* Create a context for compression */
    cparams.typesize = typesize;
    cparams.compcode = BLOSC_NDLZ;
    cparams.filters[BLOSC2_MAX_FILTERS - 1] = BLOSC_SHUFFLE;
    cparams.clevel = 5;
    cparams.nthreads = 1;
    cparams.ndim = ndim;
    cparams.blockshape = blockshape;
    cparams.blocksize = blockshape[0] * blockshape[1] * typesize;
    if (cparams.blocksize < BLOSC_MIN_BUFFERSIZE) {
        printf("\n Blocksize is letter than min \n");
    }

    /* Create a context for decompression */
    dparams.nthreads = 1;
    dparams.schunk = NULL;

    blosc2_context *cctx;
    blosc2_context *dctx;
    cctx = blosc2_create_cctx(cparams);
    dctx = blosc2_create_dctx(dparams);
    /*
    printf("\n data \n");
    for (int i = 0; i < nbytes; i++) {
    printf("%u, ", data2[i]);
    }
    */

    /* Compress with clevel=5 and shuffle active  */
    csize = blosc2_compress_ctx(cctx, isize, data_in, data_out, osize);
    if (csize == 0) {
        printf("Buffer is uncompressible.  Giving up.\n");
        return 0;
    }
    else if (csize < 0) {
        printf("Compression error.  Error code: %d\n", csize);
        return csize;
    }

    printf("Compression: %d -> %d (%.1fx)\n", isize, csize, (1. * isize) / csize);

    /* Decompress  */
    dsize = blosc2_decompress_ctx(dctx, data_out, data_dest, dsize);
    if (dsize <= 0) {
        printf("Decompression error.  Error code: %d\n", dsize);
        return dsize;
    }
/*
    printf("data_in: \n");
    for (int i = 0; i < nbytes; i++) {
        printf("%u, ", data_in[i]);
    }

    printf("\n out \n");
    for (int i = 0; i < osize; i++) {
        printf("%u, ", data_out[i]);
    }
    printf("\n dest \n");
    for (int i = 0; i < nbytes; i++) {
        printf("%u, ", data_dest[i]);
    }
*/
    for (int i = 0; i < nbytes; i++) {
        if (data_in[i] != data_dest[i]) {
            printf("i: %d, data %u, dest %u", i, data_in[i], data_dest[i]);
            printf("\n Decompressed data differs from original!\n");
            return -1;
        }
    }

    printf("Succesful roundtrip!\n");
    return dsize - csize;
}

int no_matches() {
    int ndim = 2;
    int typesize = 1;
    int32_t shape[8] = {24, 36};
    int32_t chunkshape[8] = {24, 36};
    int32_t blockshape[8] = {12, 12};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i++) {
        data[i] = i;
    }
    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int no_matches_pad() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {19, 21};
    int32_t chunkshape[8] = {19, 21};
    int32_t blockshape[8] = {11, 13};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i++) {
        data[i] = (-i^2) * 111111 - (-i^2) * 11111 + i * 1111 - i * 110 + i;
    }
    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int all_elem_eq() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {64, 64};
    int32_t chunkshape[8] = {64, 64};
    int32_t blockshape[8] = {32, 32};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i++) {
        data[i] = 1;
    }
    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int all_elem_pad() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {29, 31};
    int32_t chunkshape[8] = {29, 31};
    int32_t blockshape[8] = {12, 14};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i++) {
        data[i] = 1;
    }
    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int same_cells() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {32, 32};
    int32_t chunkshape[8] = {32, 32};
    int32_t blockshape[8] = {25, 23};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i += 4) {
        data[i] = 0;
        data[i + 1] = 1111111;
        data[i + 2] = 2;
        data[i + 3] = 1111111;
    }

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int same_cells_pad() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {31, 30};
    int32_t chunkshape[8] = {31, 30};
    int32_t blockshape[8] = {25, 23};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < (isize / 4); i++) {
        data[i * 4] = (uint32_t *) 11111111;
        data[i * 4 + 1] = (uint32_t *) 99999999;
    }

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int some_matches() {
    int ndim = 2;
    int typesize = 1;
    int32_t shape[8] = {256, 256};
    int32_t chunkshape[8] = {256, 256};
    int32_t blockshape[8] = {64, 64};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < isize; i++) {
        data[i] = i;
    }
    for (int i = SIZE / 2; i < SIZE; i++) {
        data[i] = 1;
    }

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int padding_some() {
    int ndim = 2;
    int typesize = 1;
    int32_t shape[8] = {215, 233};
    int32_t chunkshape[8] = {215, 233};
    int32_t blockshape[8] = {98, 119};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < 2 * isize / 3; i++) {
        data[i] = 0;
    }
    for (int i = 2 * isize / 3; i < isize; i++) {
        data[i] = i;
    }

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int pad_some_32() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {37, 29};
    int32_t chunkshape[8] = {37, 29};
    int32_t blockshape[8] = {17, 18};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    for (int i = 0; i < 2 * isize / 3; i++) {
        data[i] = 0;
    }
    for (int i = 2 * isize / 3; i < isize; i++) {
        data[i] = i;
    }

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int image1() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {300, 450};
    int32_t chunkshape[8] = {300, 450};
    int32_t blockshape[8] = {77, 65};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int image2() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {800, 1200};
    int32_t chunkshape[8] = {800, 1200};
    int32_t blockshape[8] = {117, 123};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res2.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int image3() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {256, 256};
    int32_t chunkshape[8] = {256, 256};
    int32_t blockshape[8] = {64, 64};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res3.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int image4() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {64, 64};
    int32_t chunkshape[8] = {64, 64};
    int32_t blockshape[8] = {32, 32};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res4.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    return result;
}

int image5() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {641, 1140};
    int32_t chunkshape[8] = {641, 1140};
    int32_t blockshape[8] = {128, 128};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res5.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    free(data);
    return result;
}

int image6() {
    int ndim = 2;
    int typesize = 4;
    int32_t shape[8] = {256, 256};
    int32_t chunkshape[8] = {256, 256};
    int32_t blockshape[8] = {64, 64};
    int isize = (int)(shape[0] * shape[1]);
    int nbytes = typesize * isize;
    uint32_t data[isize];
    FILE *f = fopen("/mnt/c/Users/sosca/CLionProjects/Caterva/examples/res6.bin", "rb");
    fread(data, sizeof(data), 1, f);
    fclose(f);

    caterva_params_t params;
    params.itemsize = typesize;
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    caterva_storage_t storage = {0};
    storage.backend = CATERVA_STORAGE_BLOSC;
    for (int i = 0; i < ndim; ++i) {
        storage.properties.blosc.chunkshape[i] = chunkshape[i];
        storage.properties.blosc.blockshape[i] = blockshape[i];
    }

    /* Run the test. */
    int result = test_ndlz(data, nbytes, typesize, ndim, params, storage);
    free(data);
    return result;
}


int main(void) {

    int result;
/*
    result = no_matches();
    printf("no_matches: %d obtained \n \n", result);
    result = no_matches_pad();
    printf("no_matches_pad: %d obtained \n \n", result);
    result = all_elem_eq();
    printf("all_elem_eq: %d obtained \n \n", result);
    result = all_elem_pad();
    printf("all_elem_pad: %d obtained \n \n", result);

    result = same_cells();
    printf("same_cells: %d obtained \n \n", result);
    result = same_cells_pad();
    printf("same_cells_pad: %d obtained \n \n", result);
    result = some_matches();
    printf("some_matches: %d obtained \n \n", result);
    result = padding_some();
    printf("pad_some: %d obtained \n \n", result);
    result = pad_some_32();
    printf("pad_some_32: %d obtained \n \n", result);
*/
    result = image1();
    printf("image1 with padding: %d obtained \n \n", result);
    result = image2();
    printf("image2 with  padding: %d obtained \n \n", result);
    result = image3();
    printf("image3 with NO padding: %d obtained \n \n", result);
    result = image4();
    printf("image4 with NO padding: %d obtained \n \n", result);
    result = image5();
    printf("image5 with padding: %d obtained \n \n", result);
    result = image6();
    printf("image6 with NO padding: %d obtained \n \n", result);

}