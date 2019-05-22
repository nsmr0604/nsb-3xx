#ifndef TRNG3_DRIVER_H_
#define TRNG3_DRIVER_H_

#include <linux/hw_random.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

#include "elptrng3_hw.h"
#include "elptrng3.h"

struct trng3_priv {
   struct elptrng3_state trng3;

   /* cfg_lock must be held when accessing TRNG MODE register. */
   spinlock_t cfg_lock;

   /* iap_lock must be held when using the indirect access port. */
   spinlock_t iap_lock;

   /*
    * seed_generation increments whenever the seed changes.  Must only be
    * accessed while holding the cfg_lock.
    */
   unsigned long long seed_generation;
   wait_queue_head_t reseed_wq;

   /*
    * alarm_generation records the active seed when an alarm was detected.
    * alarm_reason records the ISTAT alarm bit index most recently seen.
    * Must only be accessed while holding the cfg_lock.
    */
   unsigned long long alarm_generation;
   unsigned alarm_reason;
   struct work_struct alarm_work;

   unsigned char nonce_buf[32];
   bool nonce_valid;

   /* Linux hw_random interfaces */
   struct hwrng hwrng;
};

int trng3_create_debug_attrs(struct device *dev);
void trng3_remove_debug_attrs(struct device *dev);

#endif
