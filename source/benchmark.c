/*
 *  Benchmark demonstration program
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *  Copyright 2017, 2021 NXP. Not a Contribution
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)

#if defined(FREESCALE_KSDK_BM)

#include "mbedtls/version.h"
#include <stdio.h>
#include "fsl_debug_console.h"
#include "fsl_clock.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#if defined(MBEDTLS_NXP_SSSAPI)
#include "sssapi_mbedtls.h"
#else
#include "ksdk_mbedtls.h"
#endif

#define mbedtls_printf PRINTF
#define mbedtls_snprintf snprintf
#define MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED -0x0072 /**< The requested feature is not supported by the platform */     
#define mbedtls_exit(x) \
    do                  \
    {                   \
    } while (1)
#define mbedtls_free free
#define fflush(x) \
    do            \
    {             \
    } while (0)

#else
#include "mbedtls/platform.h"
#endif
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_exit       exit
#define mbedtls_printf     printf
#define mbedtls_snprintf   snprintf
#define mbedtls_free       free
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#if !defined(MBEDTLS_TIMING_C) && !defined(FREESCALE_KSDK_BM)
int main(void)
{
    mbedtls_printf("MBEDTLS_TIMING_C not defined.\n");
    return( 0 );
}
#else

#include <string.h>
#include <stdlib.h>

#include "mbedtls/timing.h"

#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
//#include "mbedtls/ripemd160.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
//#include "mbedtls/sha512.h"

//#include "mbedtls/arc4.h"
//#include "mbedtls/des.h"
//#include "mbedtls/aes.h"
//#include "mbedtls/aria.h"
//#include "mbedtls/blowfish.h"
//#include "mbedtls/camellia.h"
#include "mbedtls/chacha20.h"
//#include "mbedtls/gcm.h"
#include "mbedtls/ccm.h"
//#include "mbedtls/chachapoly.h"
//#include "mbedtls/cmac.h"
#include "mbedtls/poly1305.h"

#include "mbedtls/havege.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hmac_drbg.h"

#include "mbedtls/rsa.h"
#include "mbedtls/dhm.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"

#include "mbedtls/error.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#include "fsl_device_registers.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CORE_CLK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

/*
 * For heap usage estimates, we need an estimate of the overhead per allocated
 * block. ptmalloc2/3 (used in gnu libc for instance) uses 2 size_t per block,
 * so use that as our baseline.
 */
#define MEM_BLOCK_OVERHEAD  ( 2 * sizeof( size_t ) )

/*
 * Size to use for the alloc buffer if MEMORY_BUFFER_ALLOC_C is defined.
 */
#define HEAP_SIZE       (1u << 16)  /* 64k */

#define BUFSIZE         1024
#define HEADER_FORMAT   "  %-24s :  "
#define TITLE_LEN       25

#define OPTIONS                                                         \
    "md4, md5, ripemd160, sha1, sha256, sha512,\n"                      \
    "arc4, des3, des, camellia, blowfish, chacha20,\n"                  \
    "aes_cbc, aes_gcm, aes_ccm, aes_xts, chachapoly,\n"                 \
    "aes_cmac, des3_cmac, poly1305\n"                                   \
    "havege, ctr_drbg, hmac_drbg\n"                                     \
    "rsa, dhm, ecdsa, ecdh.\n"

#if defined(MBEDTLS_ERROR_C)
#define PRINT_ERROR                                                     \
        mbedtls_strerror( ret, ( char * )tmp, sizeof( tmp ) );          \
        mbedtls_printf( "FAILED: %s\n", tmp );
#else
#define PRINT_ERROR                                                     \
        mbedtls_printf( "FAILED: -0x%04x\n", (unsigned int) -ret );
#endif

#define TIME_AND_TSC(TITLE, CODE)                                                                        \
    do                                                                                                   \
    {                                                                                                    \
        uint32_t ii, jj;                                                                                 \
        uint64_t tsc1, tsc2;                                                                             \
        int ret = 0;                                                                                     \
                                                                                                         \
        mbedtls_printf(HEADER_FORMAT, TITLE);                                                            \
        fflush(stdout);                                                                                  \
                                                                                                         \
        benchmark_mbedtls_set_alarm(1);                                                                  \
        tsc1 = benchmark_mbedtls_timing_hardclock();                                                     \
        for (ii = 1; ret == 0 && !benchmark_mbedtls_timing_alarmed; ii++)                                \
        {                                                                                                \
            ret = CODE;                                                                                  \
            benchmark_mbedtls_poll_alarm();                                                              \
        }                                                                                                \
                                                                                                         \
        tsc2 = benchmark_mbedtls_timing_hardclock();                                                     \
        for (jj = 0; ret == 0 && jj < 1024; jj++)                                                        \
        {                                                                                                \
            ret = CODE;                                                                                  \
        }                                                                                                \
                                                                                                         \
        if (ret != 0)                                                                                    \
        {                                                                                                \
            PRINT_ERROR;                                                                                 \
        }                                                                                                \
        else                                                                                             \
        {                                                                                                \
            mbedtls_printf("%6.2f KB/s,  %6.2f cycles/byte\r\n",                                         \
                           (ii * BUFSIZE / 1024) / (((float)(tsc2 - tsc1)) / CLOCK_GetCoreSysClkFreq()), \
                           (((float)(benchmark_mbedtls_timing_hardclock() - tsc2)) / (jj * BUFSIZE)));   \
        }                                                                                                \
    } while (0)

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) && defined(MBEDTLS_MEMORY_DEBUG)

/* How much space to reserve for the title when printing heap usage results.
 * Updated manually as the output of the following command:
 *
 *  sed -n 's/.*[T]IME_PUBLIC.*"\(.*\)",/\1/p' programs/test/benchmark.c |
 *      awk '{print length+2}' | sort -rn | head -n1
 *
 * This computes the maximum length of a title +2 (because we appends "/s").
 * (If the value is too small, the only consequence is poor alignement.) */
#define TITLE_SPACE 16

#define MEMORY_MEASURE_INIT                                             \
    size_t max_used, max_blocks, max_bytes;                             \
    size_t prv_used, prv_blocks;                                        \
    mbedtls_memory_buffer_alloc_cur_get( &prv_used, &prv_blocks );      \
    mbedtls_memory_buffer_alloc_max_reset( );

#define MEMORY_MEASURE_PRINT( title_len )                               \
    mbedtls_memory_buffer_alloc_max_get( &max_used, &max_blocks );      \
    ii = TITLE_SPACE > (title_len) ? TITLE_SPACE - (title_len) : 1;     \
    while( ii-- ) mbedtls_printf( " " );                                \
    max_used -= prv_used;                                               \
    max_blocks -= prv_blocks;                                           \
    max_bytes = max_used + MEM_BLOCK_OVERHEAD * max_blocks;             \
    mbedtls_printf( "%6u heap bytes", (unsigned) max_bytes );

#else
#define MEMORY_MEASURE_INIT
#define MEMORY_MEASURE_PRINT( title_len )
#endif

#define TIME_PUBLIC(TITLE, TYPE, CODE)                                                                                \
    do                                                                                                                \
    {                                                                                                                 \
        uint32_t ii;                                                                                                  \
        uint64_t tsc;                                                                                                 \
        int ret;                                                                                                      \
        MEMORY_MEASURE_INIT;                                                                                          \
                                                                                                                      \
        mbedtls_printf(HEADER_FORMAT, TITLE);                                                                         \
        fflush(stdout);                                                                                               \
        benchmark_mbedtls_set_alarm(3);                                                                               \
                                                                                                                      \
        ret = 0;                                                                                                      \
        tsc = benchmark_mbedtls_timing_hardclock();                                                                   \
        for (ii = 1; !benchmark_mbedtls_timing_alarmed && !ret; ii++)                                                 \
        {                                                                                                             \
            CODE;                                                                                                     \
            benchmark_mbedtls_poll_alarm();                                                                           \
        }                                                                                                             \
                                                                                                                      \
        if (ret != 0)                                                                                                 \
        {                                                                                                             \
            PRINT_ERROR;                                                                                              \
        }                                                                                                             \
        else                                                                                                          \
        {                                                                                                             \
            mbedtls_printf("%6.2f " TYPE "/s",                                                                        \
                           ((float)ii) / ((benchmark_mbedtls_timing_hardclock() - tsc) / CLOCK_GetCoreSysClkFreq())); \
            MEMORY_MEASURE_PRINT(sizeof(TYPE) + 1);                                                                   \
            mbedtls_printf("\r\n");                                                                                   \
        }                                                                                                             \
    } while (0)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
static int myrand( void *rng_state, unsigned char *output, size_t len )
{
    size_t use_len;
    int rnd;

    if( rng_state != NULL )
        rng_state  = NULL;

    while( len > 0 )
    {
        use_len = len;
        if( use_len > sizeof(int) )
            use_len = sizeof(int);

        rnd = rand();
        memcpy( output, &rnd, use_len );
        output += use_len;
        len -= use_len;
    }

    return( 0 );
}

#define CHECK_AND_CONTINUE( R )                                         \
    {                                                                   \
        int CHECK_AND_CONTINUE_ret = ( R );                             \
        if( CHECK_AND_CONTINUE_ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED ) { \
            mbedtls_printf( "Feature not supported. Skipping.\n" );     \
            continue;                                                   \
        }                                                               \
        else if( CHECK_AND_CONTINUE_ret != 0 ) {                        \
            mbedtls_exit( 1 );                                          \
        }                                                               \
    }

/*
 * Clear some memory that was used to prepare the context
 */
#if defined(MBEDTLS_ECP_C)
void ecp_clear_precomputed( mbedtls_ecp_group *grp )
{
    if( grp->T != NULL )
    {
        size_t i;
        for( i = 0; i < grp->T_size; i++ )
            mbedtls_ecp_point_free( &grp->T[i] );
        mbedtls_free( grp->T );
    }
    grp->T = NULL;
    grp->T_size = 0;
}
#else
#define ecp_clear_precomputed( g )
#endif

/* NXP: Move buffer to NON-CACHED memory because of HW accel */ 
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    AT_NONCACHEABLE_SECTION_INIT(static unsigned char buf[BUFSIZE]);
#else
    unsigned char buf[BUFSIZE];    
#endif /* DCACHE */

#if defined(MBEDTLS_ECP_C)
static int set_ecp_curve( const char *string, mbedtls_ecp_curve_info *curve )
{
    const mbedtls_ecp_curve_info *found =
        mbedtls_ecp_curve_info_from_name( string );
    if( found != NULL )
    {
        *curve = *found;
        return( 1 );
    }
    else
        return( 0 );
}
#endif

typedef struct {
    char md4, md5, ripemd160, sha1, sha256, sha512,
         arc4, des3, des,
         aes_cbc, aes_gcm, aes_ccm, aes_xts, chachapoly,
         aes_cmac, des3_cmac,
         aria, camellia, blowfish, chacha20,
         poly1305,
         havege, ctr_drbg, hmac_drbg,
         rsa, dhm, ecdsa, ecdh;
} todo_list;

#if defined(FREESCALE_KSDK_BM) && !defined(MBEDTLS_TIMING_C)

static volatile uint32_t s_MsCount = 0U;
static volatile int benchmark_mbedtls_timing_alarmed;
static uint64_t s_Timeout;

/*!
 * @brief Milliseconds counter since last POR/reset.
 */
void SysTick_Handler(void)
{
    s_MsCount++;
}

static uint64_t benchmark_mbedtls_timing_hardclock(void)
{
    uint32_t currMsCount;
    uint32_t currTick;
    uint32_t loadTick;

    do
    {
        currMsCount = s_MsCount;
        currTick    = SysTick->VAL;
    } while (currMsCount != s_MsCount);

    loadTick = CLOCK_GetCoreSysClkFreq() / 1000U;
    return (((uint64_t)currMsCount) * loadTick) + loadTick - currTick;
}

static void benchmark_mbedtls_set_alarm(int seconds)
{
    benchmark_mbedtls_timing_alarmed = 0;
    s_Timeout                        = benchmark_mbedtls_timing_hardclock() + (seconds * CLOCK_GetCoreSysClkFreq());
}

static void benchmark_mbedtls_poll_alarm(void)
{
    if (benchmark_mbedtls_timing_hardclock() > s_Timeout)
    {
        benchmark_mbedtls_timing_alarmed = 1;
    }
}
#endif

static int bench_print_features(void)
{
    char *text;
    mbedtls_printf("mbedTLS version %s\r\n", MBEDTLS_VERSION_STRING);
    mbedtls_printf("fsys=%lu\r\n", ((CORE_CLK_FREQ)));
    mbedtls_printf("Using following implementations:\r\n");
#if defined(MBEDTLS_FREESCALE_LTC_SHA256)
    text = "LTC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_MMCAU_SHA256)
    text = "MMCAU HW accelerated";
#elif defined(MBEDTLS_FREESCALE_LPC_SHA256)
    text = "LPC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAU3_SHA256)
    text = "CAU3 HW accelerated";
#elif defined(MBEDTLS_FREESCALE_DCP_SHA256)
    text = "DCP HW accelerated";
#elif defined(MBEDTLS_FREESCALE_HASHCRYPT_SHA256)
    text = "HASHCRYPT HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAAM_SHA256)
    text = "CAAM HW accelerated";
#elif defined(MBEDTLS_NXP_SENTINEL200)
    text = "S200 HW accelerated";
#elif defined(MBEDTLS_NXP_SENTINEL300)
    text = "S300 HW accelerated";
#else
    text = "Software implementation";
#endif
    mbedtls_printf("  SHA: %s\r\n", text);
#if defined(MBEDTLS_FREESCALE_LTC_AES)
    text = "LTC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_MMCAU_AES)
    text = "MMCAU HW accelerated";
#elif defined(MBEDTLS_FREESCALE_LPC_AES)
    text = "LPC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAU3_AES)
    text = "CAU3 HW accelerated";
#elif defined(MBEDTLS_FREESCALE_DCP_AES)
    text = "DCP HW accelerated";
#elif defined(MBEDTLS_FREESCALE_HASHCRYPT_AES)
    text = "HASHCRYPT HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAAM_AES)
    text = "CAAM HW accelerated";
#elif defined(MBEDTLS_NXP_SENTINEL200)
    text = "SW AES, S200 HW accelerated CCM and CMAC";
#elif defined(MBEDTLS_NXP_SENTINEL300)
    text = "SW AES, S300 HW accelerated CCM and CMAC";
#else
    text = "Software implementation";
#endif
    mbedtls_printf("  AES: %s\r\n", text);
#if defined(MBEDTLS_FREESCALE_LTC_AES_GCM)
    text = "LTC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_MMCAU_AES)
    text = "MMCAU HW accelerated";
#elif defined(MBEDTLS_FREESCALE_LPC_AES_GCM)
    text = "LPC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAU3_AES)
    text = "CAU3 HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAAM_AES)
    text = "CAAM HW accelerated";
#else
    text = "Software implementation";
#endif
    mbedtls_printf("  AES GCM: %s\r\n", text);
#if defined(MBEDTLS_FREESCALE_LTC_DES)
    text = "LTC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_MMCAU_DES)
    text = "MMCAU HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAU3_DES)
    text = "CAU3 HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAAM_DES)
    text = "CAAM HW accelerated";
#else
    text = "Software implementation";
#endif
    mbedtls_printf("  DES: %s\r\n", text);
#if defined(MBEDTLS_FREESCALE_LTC_PKHA)
    text = "LTC HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CAU3_PKHA)
    text = "CAU3 HW accelerated";
#elif defined(MBEDTLS_FREESCALE_CASPER_PKHA)
    text = "CASPER HW accelerated ECC256/384/521 and RSA verify";
#elif defined(MBEDTLS_FREESCALE_CAAM_PKHA)
    text = "CAAM HW accelerated";
#elif defined(MBEDTLS_NXP_SENTINEL200)
    text = "S200 HW accelerated ECDSA and ECDH";
#elif defined(MBEDTLS_NXP_SENTINEL300)
    text = "S300 HW accelerated ECDSA and ECDH";
#else
    text = "Software implementation";
#endif
    mbedtls_printf("  Asymmetric cryptography: %s\r\n", text);

    return 0;
}

int main( int argc, char *argv[] )
{
    int i;
    unsigned char tmp[200];
    char title[TITLE_LEN];
    todo_list todo;
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    unsigned char alloc_buf[HEAP_SIZE] = { 0 };
#endif
#if defined(MBEDTLS_ECP_C)
    mbedtls_ecp_curve_info single_curve[2] = {
        { MBEDTLS_ECP_DP_NONE, 0, 0, NULL },
        { MBEDTLS_ECP_DP_NONE, 0, 0, NULL },
    };
    const mbedtls_ecp_curve_info *curve_list = mbedtls_ecp_curve_list( );
#endif
    
#if defined(MBEDTLS_ECP_C)
    (void) curve_list; /* Unused in some configurations where no benchmark uses ECC */
#endif    

#if defined(FREESCALE_KSDK_BM)
    /* HW init */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    if( CRYPTO_InitHardware() != kStatus_Success )
    {
        mbedtls_printf( "Initialization of crypto HW failed\n" );
        mbedtls_exit( MBEDTLS_EXIT_FAILURE );
    }

    /* Init SysTick module */
    /* call CMSIS SysTick function. It enables the SysTick interrupt at low priority */
    SysTick_Config(CLOCK_GetCoreSysClkFreq() / 1000U); /* 1 ms period */
#endif
    bench_print_features();
#if 0 /* We need to run all tests*/
    if( argc <= 1 )
    {
        memset( &todo, 1, sizeof( todo ) );
    }
    else
    {
        memset( &todo, 0, sizeof( todo ) );

        for( i = 1; i < argc; i++ )
        {
            if( strcmp( argv[i], "md4" ) == 0 )
                todo.md4 = 1;
            else if( strcmp( argv[i], "md5" ) == 0 )
                todo.md5 = 1;
            else if( strcmp( argv[i], "sha1" ) == 0 )
                todo.sha1 = 1;
            else if( strcmp( argv[i], "sha256" ) == 0 )
                todo.sha256 = 1;
            else if( strcmp( argv[i], "sha512" ) == 0 )
                todo.sha512 = 1;
            else if( strcmp( argv[i], "chacha20" ) == 0 )
                todo.chacha20 = 1;
            else if( strcmp( argv[i], "rsa" ) == 0 )
                todo.rsa = 1;
#if defined(MBEDTLS_ECP_C)
            else if( set_ecp_curve( argv[i], single_curve ) )
                curve_list = single_curve;
#endif
            else
            {
                mbedtls_printf( "Unrecognized option: %s\n", argv[i] );
                mbedtls_printf( "Available options: " OPTIONS );
            }
        }
    }
#else
    /* Run all tests.*/
    memset(&todo, 1, sizeof(todo));
#endif

    mbedtls_printf( "\n" );

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init( alloc_buf, sizeof( alloc_buf ) );
#endif
    memset( buf, 0xAA, sizeof( buf ) );
    memset( tmp, 0xBB, sizeof( tmp ) );

#if defined(MBEDTLS_MD4_C)
    if( todo.md4 )
        //TIME_AND_TSC( "MD4", mbedtls_md4_ret( buf, BUFSIZE, tmp ) );
#endif

#if defined(MBEDTLS_MD5_C)
    if( todo.md5 )
        //TIME_AND_TSC( "MD5", mbedtls_md5_ret( buf, BUFSIZE, tmp ) );
#endif

#if defined(MBEDTLS_SHA1_C)
    if( todo.sha1 )
        //TIME_AND_TSC( "SHA-1", mbedtls_sha1_ret( buf, BUFSIZE, tmp ) );
#endif

#if defined(MBEDTLS_SHA256_C)
    if( todo.sha256 )
        //TIME_AND_TSC( "SHA-256", mbedtls_sha256_ret( buf, BUFSIZE, tmp, 0 ) );
#endif

#if defined(MBEDTLS_SHA512_C)
    if( todo.sha512 )
       // TIME_AND_TSC( "SHA-512", mbedtls_sha512_ret( buf, BUFSIZE, tmp, 0 ) );
#endif

#if defined(MBEDTLS_AES_C)
#if defined(MBEDTLS_CIPHER_MODE_CBC)
    if( todo.aes_cbc )
    {
        int keysize;
        mbedtls_aes_context aes;
        mbedtls_aes_init( &aes );
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
#ifdef MBEDTLS_AES_ALT_NO_192
            if (keysize == 192)
            {
                continue;
            }
#endif
#ifdef MBEDTLS_AES_ALT_NO_256
            if (keysize == 256)
            {
                continue;
            }
#endif
            //mbedtls_snprintf( title, sizeof( title ), "AES-CBC-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            mbedtls_aes_setkey_enc( &aes, tmp, keysize );

            //TIME_AND_TSC( title,
                //mbedtls_aes_crypt_cbc( &aes, MBEDTLS_AES_ENCRYPT, BUFSIZE, tmp, buf, buf ) );
        }
        mbedtls_aes_free( &aes );
    }
#endif
#if defined(MBEDTLS_CIPHER_MODE_XTS)
    if( todo.aes_xts )
    {
        int keysize;
        mbedtls_aes_xts_context ctx;

        mbedtls_aes_xts_init( &ctx );
        for( keysize = 128; keysize <= 256; keysize += 128 )
        {
#ifdef MBEDTLS_AES_ALT_NO_256
            if (keysize == 256)
            {
                continue;
            }
#endif
            //mbedtls_snprintf( title, sizeof( title ), "AES-XTS-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            mbedtls_aes_xts_setkey_enc( &ctx, tmp, keysize * 2 );

            //TIME_AND_TSC( title,
                    //mbedtls_aes_crypt_xts( &ctx, MBEDTLS_AES_ENCRYPT, BUFSIZE,
                                          // tmp, buf, buf ) );

            mbedtls_aes_xts_free( &ctx );
        }
    }
#endif

#if defined(MBEDTLS_CCM_C)
    if( todo.aes_ccm )
    {
        int keysize;
        mbedtls_ccm_context ccm;

        mbedtls_ccm_init( &ccm );
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
#ifdef MBEDTLS_AES_ALT_NO_192
            if (keysize == 192)
            {
                continue;
            }
#endif
#ifdef MBEDTLS_AES_ALT_NO_256
            if (keysize == 256)
            {
                continue;
            }
#endif
            //mbedtls_snprintf( title, sizeof( title ), "AES-CCM-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            mbedtls_ccm_setkey( &ccm, MBEDTLS_CIPHER_ID_AES, tmp, keysize );

            //TIME_AND_TSC( title,
                   // mbedtls_ccm_encrypt_and_tag( &ccm, BUFSIZE, tmp,
                        //12, NULL, 0, buf, buf, tmp, 16 ) );

            mbedtls_ccm_free( &ccm );
        }
    }
#endif
#if defined(MBEDTLS_CHACHAPOLY_C)
    if( todo.chachapoly )
    {
        mbedtls_chachapoly_context chachapoly;

        mbedtls_chachapoly_init( &chachapoly );
        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );

        mbedtls_snprintf( title, sizeof( title ), "ChaCha20-Poly1305" );

        mbedtls_chachapoly_setkey( &chachapoly, tmp );

        TIME_AND_TSC( title,
                mbedtls_chachapoly_encrypt_and_tag( &chachapoly,
                    BUFSIZE, tmp, NULL, 0, buf, buf, tmp ) );

        mbedtls_chachapoly_free( &chachapoly );
    }
#endif
#if defined(MBEDTLS_CMAC_C)
    if( todo.aes_cmac )
    {
        unsigned char output[16];
        const mbedtls_cipher_info_t *cipher_info;
        mbedtls_cipher_type_t cipher_type;
        int keysize;

        for (keysize = 128, cipher_type = MBEDTLS_CIPHER_AES_128_ECB; keysize <= 256; keysize += 64, cipher_type++)
        {
#ifdef MBEDTLS_AES_ALT_NO_192
            if (keysize == 192)
            {
                continue;
            }
#endif
            //mbedtls_snprintf( title, sizeof( title ), "AES-CMAC-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );

            cipher_info = mbedtls_cipher_info_from_type( cipher_type );

           // TIME_AND_TSC( title,
                         // mbedtls_cipher_cmac( cipher_info, tmp, keysize,
                                             //  buf, BUFSIZE, output ) );
        }

        memset( buf, 0, sizeof( buf ) );
        memset( tmp, 0, sizeof( tmp ) );
       // TIME_AND_TSC( "AES-CMAC-PRF-128",
                     // mbedtls_aes_cmac_prf_128( tmp, 16, buf, BUFSIZE,
                                             //   output ) );
    }
#endif /* MBEDTLS_CMAC_C */
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_CHACHA20_C)
    if ( todo.chacha20 )
    {
        TIME_AND_TSC( "ChaCha20", mbedtls_chacha20_crypt( buf, buf, 0U, BUFSIZE, buf, buf2 ) );

        PRINTF("My textplain: ");
        for(int j = 0; i < buf; i++ ){
        	PRINTF("%x", buf);
        }

        PRINTF("\nMy chyper: ");
        for(int j = 0; i < buf2; i++ ){
        	PRINTF("%x", buf2);
        }

    }
#endif

#if defined(MBEDTLS_POLY1305_C)
    if ( todo.poly1305 )
    {
        TIME_AND_TSC( "Poly1305dsfdsfsd", mbedtls_poly1305_mac( buf, buf, BUFSIZE, buf ) );
    }

#endif


#if defined(MBEDTLS_HAVEGE_C)
    if( todo.havege )
    {
        mbedtls_havege_state hs;
        mbedtls_havege_init( &hs );
       // TIME_AND_TSC( "HAVEGE", mbedtls_havege_random( &hs, buf, BUFSIZE ) );
        mbedtls_havege_free( &hs );
    }
#endif


#if defined(MBEDTLS_HMAC_DRBG_C)
    if( todo.hmac_drbg )
    {
        mbedtls_hmac_drbg_context hmac_drbg;
        const mbedtls_md_info_t *md_info;

        mbedtls_hmac_drbg_init( &hmac_drbg );

#if defined(MBEDTLS_SHA1_C)
        if( ( md_info = mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 ) ) == NULL )
            mbedtls_exit(1);

        if( mbedtls_hmac_drbg_seed( &hmac_drbg, md_info, myrand, NULL, NULL, 0 ) != 0 )
            mbedtls_exit(1);
        //TIME_AND_TSC( "HMAC_DRBG SHA-1 (NOPR)",
               // mbedtls_hmac_drbg_random( &hmac_drbg, buf, BUFSIZE ) );

        if( mbedtls_hmac_drbg_seed( &hmac_drbg, md_info, myrand, NULL, NULL, 0 ) != 0 )
            mbedtls_exit(1);
        mbedtls_hmac_drbg_set_prediction_resistance( &hmac_drbg,
                                             MBEDTLS_HMAC_DRBG_PR_ON );
        //TIME_AND_TSC( "HMAC_DRBG SHA-1 (PR)",
                //mbedtls_hmac_drbg_random( &hmac_drbg, buf, BUFSIZE ) );
#endif

#if defined(MBEDTLS_SHA256_C)
        if( ( md_info = mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 ) ) == NULL )
            mbedtls_exit(1);

        if( mbedtls_hmac_drbg_seed( &hmac_drbg, md_info, myrand, NULL, NULL, 0 ) != 0 )
            mbedtls_exit(1);
        //TIME_AND_TSC( "HMAC_DRBG SHA-256 (NOPR)",
                //mbedtls_hmac_drbg_random( &hmac_drbg, buf, BUFSIZE ) );

        if( mbedtls_hmac_drbg_seed( &hmac_drbg, md_info, myrand, NULL, NULL, 0 ) != 0 )
            mbedtls_exit(1);
        mbedtls_hmac_drbg_set_prediction_resistance( &hmac_drbg,
                                             MBEDTLS_HMAC_DRBG_PR_ON );
        //TIME_AND_TSC( "HMAC_DRBG SHA-256 (PR)",
                //mbedtls_hmac_drbg_random( &hmac_drbg, buf, BUFSIZE ) );
#endif
        mbedtls_hmac_drbg_free( &hmac_drbg );
    }
#endif

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
    if (todo.rsa)
    {
        int keysize;
        mbedtls_rsa_context rsa;

        /* Skip 2048 and 4096 bit keys, generating takes too long */
        for (keysize = 1024; keysize <= 1024; keysize *= 2)
        {
            //mbedtls_snprintf( title, sizeof( title ), "RSA-%d", keysize );

            mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );
            mbedtls_rsa_gen_key( &rsa, myrand, NULL, keysize, 65537 );

            //TIME_PUBLIC( title, " public",
                   // buf[0] = 0;
                   // ret = mbedtls_rsa_public( &rsa, buf, buf ) );

           // TIME_PUBLIC( title, "private",
                 //   buf[0] = 0;
                 //   ret = mbedtls_rsa_private( &rsa, myrand, NULL, buf, buf ) );

            mbedtls_rsa_free( &rsa );
        }
    }
#endif


#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_SHA256_C)
    if( todo.ecdsa )
    {
        mbedtls_ecdsa_context ecdsa;
        const mbedtls_ecp_curve_info *curve_info;
        size_t sig_len;

        memset( buf, 0x2A, sizeof( buf ) );

        for( curve_info = curve_list;
             curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
             curve_info++ )
        {
            if( ! mbedtls_ecdsa_can_do( curve_info->grp_id ) )
                continue;

            mbedtls_ecdsa_init( &ecdsa );

            if( mbedtls_ecdsa_genkey( &ecdsa, curve_info->grp_id, myrand, NULL ) != 0 )
                mbedtls_exit( 1 );
            ecp_clear_precomputed( &ecdsa.grp );

           // mbedtls_snprintf( title, sizeof( title ), "ECDSA-%s",
                                           //   curve_info->name );
            //TIME_PUBLIC( title, "sign",
                    //ret = mbedtls_ecdsa_write_signature( &ecdsa, MBEDTLS_MD_SHA256, buf, curve_info->bit_size,
                                               // tmp, &sig_len, myrand, NULL ) );

            mbedtls_ecdsa_free( &ecdsa );
        }

        for( curve_info = curve_list;
             curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
             curve_info++ )
        {
            if( ! mbedtls_ecdsa_can_do( curve_info->grp_id ) )
                continue;

            mbedtls_ecdsa_init( &ecdsa );

            if( mbedtls_ecdsa_genkey( &ecdsa, curve_info->grp_id, myrand, NULL ) != 0 ||
                mbedtls_ecdsa_write_signature( &ecdsa, MBEDTLS_MD_SHA256, buf, curve_info->bit_size,
                                               tmp, &sig_len, myrand, NULL ) != 0 )
            {
                mbedtls_exit( 1 );
            }
            ecp_clear_precomputed( &ecdsa.grp );

            //mbedtls_snprintf( title, sizeof( title ), "ECDSA-%s",
                                            //  curve_info->name );
            //TIME_PUBLIC( title, "verify",
                    //ret = mbedtls_ecdsa_read_signature( &ecdsa, buf, curve_info->bit_size,
                                                //tmp, sig_len ) );

            mbedtls_ecdsa_free( &ecdsa );
        }
    }
#endif

#if defined(MBEDTLS_ECDH_C) && defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
    if( todo.ecdh )
    {
        mbedtls_ecdh_context ecdh;
        mbedtls_mpi z;
        const mbedtls_ecp_curve_info montgomery_curve_list[] = {
#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
            { MBEDTLS_ECP_DP_CURVE25519, 0, 0, "Curve25519" },
#endif
#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
            { MBEDTLS_ECP_DP_CURVE448, 0, 0, "Curve448" },
#endif
            { MBEDTLS_ECP_DP_NONE, 0, 0, 0 }
        };
        const mbedtls_ecp_curve_info *curve_info;
        size_t olen;
        const mbedtls_ecp_curve_info *selected_montgomery_curve_list =
            montgomery_curve_list;

        if( curve_list == (const mbedtls_ecp_curve_info*) &single_curve )
        {
            mbedtls_ecp_group grp;
            mbedtls_ecp_group_init( &grp );
            if( mbedtls_ecp_group_load( &grp, curve_list->grp_id ) != 0 )
                mbedtls_exit( 1 );
            if( mbedtls_ecp_get_type( &grp ) == MBEDTLS_ECP_TYPE_MONTGOMERY )
                selected_montgomery_curve_list = single_curve;
            else /* empty list */
                selected_montgomery_curve_list = single_curve + 1;
            mbedtls_ecp_group_free( &grp );
        }

        for( curve_info = curve_list;
             curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
             curve_info++ )
        {
            if( ! mbedtls_ecdh_can_do( curve_info->grp_id ) )
                continue;

            mbedtls_ecdh_init( &ecdh );

            CHECK_AND_CONTINUE( mbedtls_ecp_group_load( &ecdh.grp, curve_info->grp_id ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_make_public( &ecdh, &olen, buf, sizeof( buf),
                                                    myrand, NULL ) );
            CHECK_AND_CONTINUE( mbedtls_ecp_copy( &ecdh.Qp, &ecdh.Q ) );
            ecp_clear_precomputed( &ecdh.grp );

            //mbedtls_snprintf( title, sizeof( title ), "ECDHE-%s",
                                             // curve_info->name );
            TIME_PUBLIC( title, "handshake",
                    CHECK_AND_CONTINUE( mbedtls_ecdh_make_public( &ecdh, &olen, buf, sizeof( buf),
                                             myrand, NULL ) );
                    CHECK_AND_CONTINUE( mbedtls_ecdh_calc_secret( &ecdh, &olen, buf, sizeof( buf ),
                                             myrand, NULL ) ) );
            mbedtls_ecdh_free( &ecdh );
        }

        /* Montgomery curves need to be handled separately */
        for ( curve_info = selected_montgomery_curve_list;
              curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
              curve_info++ )
        {
            mbedtls_ecdh_init( &ecdh );
            mbedtls_mpi_init( &z );

            CHECK_AND_CONTINUE( mbedtls_ecp_group_load( &ecdh.grp, curve_info->grp_id ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_gen_public( &ecdh.grp, &ecdh.d, &ecdh.Qp, myrand, NULL ) );

            //mbedtls_snprintf( title, sizeof(title), "ECDHE-%s",
                             // curve_info->name );
            TIME_PUBLIC(  title, "handshake",
                    CHECK_AND_CONTINUE( mbedtls_ecdh_gen_public( &ecdh.grp, &ecdh.d, &ecdh.Q,
                                            myrand, NULL ) );
                    CHECK_AND_CONTINUE( mbedtls_ecdh_compute_shared( &ecdh.grp, &z, &ecdh.Qp, &ecdh.d,
                                                myrand, NULL ) ) );

            mbedtls_ecdh_free( &ecdh );
            mbedtls_mpi_free( &z );
        }

        for( curve_info = curve_list;
             curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
             curve_info++ )
        {
            if( ! mbedtls_ecdh_can_do( curve_info->grp_id ) )
                continue;

            mbedtls_ecdh_init( &ecdh );

            CHECK_AND_CONTINUE( mbedtls_ecp_group_load( &ecdh.grp, curve_info->grp_id ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_make_public( &ecdh, &olen, buf, sizeof( buf),
                                  myrand, NULL ) );
            CHECK_AND_CONTINUE( mbedtls_ecp_copy( &ecdh.Qp, &ecdh.Q ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_make_public( &ecdh, &olen, buf, sizeof( buf),
                                  myrand, NULL ) );
            ecp_clear_precomputed( &ecdh.grp );

            //mbedtls_snprintf( title, sizeof( title ), "ECDH-%s",
                                             // curve_info->name );
            TIME_PUBLIC( title, "handshake",
                    CHECK_AND_CONTINUE( mbedtls_ecdh_calc_secret( &ecdh, &olen, buf, sizeof( buf ),
                                             myrand, NULL ) ) );
            mbedtls_ecdh_free( &ecdh );
        }

        /* Montgomery curves need to be handled separately */
        for ( curve_info = selected_montgomery_curve_list;
              curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
              curve_info++)
        {
            mbedtls_ecdh_init( &ecdh );
            mbedtls_mpi_init( &z );

            CHECK_AND_CONTINUE( mbedtls_ecp_group_load( &ecdh.grp, curve_info->grp_id ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_gen_public( &ecdh.grp, &ecdh.d, &ecdh.Qp,
                                 myrand, NULL ) );
            CHECK_AND_CONTINUE( mbedtls_ecdh_gen_public( &ecdh.grp, &ecdh.d, &ecdh.Q, myrand, NULL ) );

           // mbedtls_snprintf( title, sizeof(title), "ECDH-%s",
                             // curve_info->name );
            TIME_PUBLIC(  title, "handshake",
                    CHECK_AND_CONTINUE( mbedtls_ecdh_compute_shared( &ecdh.grp, &z, &ecdh.Qp, &ecdh.d,
                                                myrand, NULL ) ) );

            mbedtls_ecdh_free( &ecdh );
            mbedtls_mpi_free( &z );
        }
    }
#endif

#if defined(MBEDTLS_ECDH_C) && !defined(MBEDTLS_NXP_SSSAPI)
    if( todo.ecdh )
    {
        mbedtls_ecdh_context ecdh_srv, ecdh_cli;
        unsigned char buf_srv[BUFSIZE], buf_cli[BUFSIZE];
        const mbedtls_ecp_curve_info *curve_info;
        size_t olen;

        for( curve_info = curve_list;
            curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
            curve_info++ )
        {
            if( ! mbedtls_ecdh_can_do( curve_info->grp_id ) )
                continue;

            mbedtls_ecdh_init( &ecdh_srv );
            mbedtls_ecdh_init( &ecdh_cli );

            //mbedtls_snprintf( title, sizeof( title ), "ECDHE-%s", curve_info->name );
            TIME_PUBLIC( title, "full handshake",
                const unsigned char * p_srv = buf_srv;

                CHECK_AND_CONTINUE( mbedtls_ecdh_setup( &ecdh_srv, curve_info->grp_id ) );
                CHECK_AND_CONTINUE( mbedtls_ecdh_make_params( &ecdh_srv, &olen, buf_srv, sizeof( buf_srv ), myrand, NULL ) );

                CHECK_AND_CONTINUE( mbedtls_ecdh_read_params( &ecdh_cli, &p_srv, p_srv + olen ) );
                CHECK_AND_CONTINUE( mbedtls_ecdh_make_public( &ecdh_cli, &olen, buf_cli, sizeof( buf_cli ), myrand, NULL ) );

                CHECK_AND_CONTINUE( mbedtls_ecdh_read_public( &ecdh_srv, buf_cli, olen ) );
                CHECK_AND_CONTINUE( mbedtls_ecdh_calc_secret( &ecdh_srv, &olen, buf_srv, sizeof( buf_srv ), myrand, NULL ) );

                CHECK_AND_CONTINUE( mbedtls_ecdh_calc_secret( &ecdh_cli, &olen, buf_cli, sizeof( buf_cli ), myrand, NULL ) );
                mbedtls_ecdh_free( &ecdh_cli );

                mbedtls_ecdh_free( &ecdh_srv );
            );

        }
    }
#endif

    mbedtls_printf( "\n" );

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_free();
#endif

#if defined(_WIN32)
    mbedtls_printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    while (1)
    {
        char ch = GETCHAR();
        PUTCHAR(ch);
    }
}

#endif /* MBEDTLS_TIMING_C */
