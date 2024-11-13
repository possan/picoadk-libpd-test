
// `pthread_cond_destroy'
// `pthread_cond_init'
// `pthread_cond_signal'
// `pthread_cond_wait'

// `pthread_create'
// `pthread_join'

// `pthread_mutex_destroy'
// `pthread_mutex_init'
// `pthread_mutex_lock'
// `pthread_mutex_unlock'

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <pico/bootrom.h>
#include "hardware/gpio.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"
#include "FreeRTOS.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

// typedef struct pthread_cond_internal
// {
//     BaseType_t xIsInitialized;            /**< Set to pdTRUE if this condition variable is initialized, pdFALSE otherwise. */
//     StaticSemaphore_t xCondWaitSemaphore; /**< Threads block on this semaphore in pthread_cond_wait. */
//     unsigned iWaitingThreads;             /**< The number of threads currently waiting on this condition variable. */
// } pthread_cond_internal_t;

/*-----------------------------------------------------------*/

// int pthread_cond_destroy( pthread_cond_t * cond )
// {
//     pthread_cond_internal_t * pxCond = ( pthread_cond_internal_t * ) ( cond );

//     /* Free all resources in use by the cond. */
//     vSemaphoreDelete( ( SemaphoreHandle_t ) &pxCond->xCondWaitSemaphore );

//     return 0;
// }

// /*-----------------------------------------------------------*/

// int pthread_cond_init( pthread_cond_t * cond,
//                        const pthread_condattr_t * attr )
// {
//     int iStatus = 0;
//     pthread_cond_internal_t * pxCond = ( pthread_cond_internal_t * ) cond;

//     /* Silence warnings about unused parameters. */
//     ( void ) attr;

//     if( pxCond == NULL )
//     {
//         iStatus = ENOMEM;
//     }

//     if( iStatus == 0 )
//     {
//         /* Set the members of the cond. The semaphore create calls will never fail
//          * when their arguments aren't NULL. */
//         pxCond->xIsInitialized = pdTRUE;

//         ( void ) xSemaphoreCreateCountingStatic( INT_MAX, 0U, &pxCond->xCondWaitSemaphore );
//         pxCond->iWaitingThreads = 0;
//     }

//     return iStatus;
// }

// /*-----------------------------------------------------------*/

// int pthread_cond_signal( pthread_cond_t * cond )
// {
//     pthread_cond_internal_t * pxCond = ( pthread_cond_internal_t * ) ( cond );

//     /* If the cond is uninitialized, perform initialization. */
//     prvInitializeStaticCond( pxCond );

//     /* Local copy of number of threads waiting. */
//     unsigned iLocalWaitingThreads = pxCond->iWaitingThreads;

//     /* Test local copy of threads waiting is larger than zero. */
//     while( iLocalWaitingThreads > 0 )
//     {
//         /* Test-and-set. Atomically check whether the copy in memory has changed.
//          * And, if not decrease the copy of threads waiting in memory. */
//         if( ATOMIC_COMPARE_AND_SWAP_SUCCESS == Atomic_CompareAndSwap_u32( ( uint32_t * ) &pxCond->iWaitingThreads, ( uint32_t ) iLocalWaitingThreads - 1, ( uint32_t ) iLocalWaitingThreads ) )
//         {
//             /* Unblock one. */
//             ( void ) xSemaphoreGive( ( SemaphoreHandle_t ) &pxCond->xCondWaitSemaphore );

//             /* Signal one succeeded. Break. */
//             break;
//         }

//         /* Local copy may be out dated. Reload, and retry. */
//         iLocalWaitingThreads = pxCond->iWaitingThreads;
//     }

//     return 0;
// }
