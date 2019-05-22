/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <elptrnghw.h>
#include <elptrng.h>


void trng_dump_registers (trng_hw * hw)
{
   if (ELPTRNG_INIT == hw->initialized) {
#if (defined TRNG_HDAC)
      ELPHW_PRINT ("HDAC CTRL     @%p = %08lx\n", hw->trng_hdac_ctrl, pdu_io_read32 (hw->trng_hdac_ctrl));
      ELPHW_PRINT ("HDAC STAT     @%p = %08lx\n", hw->trng_hdac_stat, pdu_io_read32 (hw->trng_hdac_stat));
#endif
      ELPHW_PRINT ("TRNG CTRL     @%p = %08lx\n", hw->trng_ctrl, pdu_io_read32 (hw->trng_ctrl));
      ELPHW_PRINT ("TRNG IRQ_STAT @%p = %08lx\n", hw->trng_irq_stat, pdu_io_read32 (hw->trng_irq_stat));
      ELPHW_PRINT ("TRNG IRQ_EN   @%p = %08lx\n", hw->trng_irq_en, pdu_io_read32 (hw->trng_irq_en));
      ELPHW_PRINT ("TRNG DATA     @%p : ", hw->trng_data);
      ELPHW_PRINT ("%08lx-%08lx-%08lx-%08lx\n", pdu_io_read32 (hw->trng_data), pdu_io_read32 (hw->trng_data +1),
                                           pdu_io_read32 (hw->trng_data +2), pdu_io_read32 (hw->trng_data +3));
   }
}

/* wait for operation complete */
/* input: number of cylces to wait */
/* output: engine status */
int32_t trng_wait_for_done (trng_hw * hw, uint32_t cycles)
{
   if (hw == 0) {
      return ELPTRNG_INVALID_ARGUMENT;
   }

   if (ELPTRNG_NOT_INIT == hw->initialized) {
      return ELPTRNG_NOT_INITIALIZED;
   } else {

//     trng_user_init_wait(hw);
//     trng_user_wait(hw);

#if (defined TRNG_HDAC)
      while ((cycles > 0) && (0 == (TRNG_HDAC_STAT_IRQ_DONE & pdu_io_read32 (hw->trng_hdac_stat)))) {
#else
      while ((cycles > 0) && (0 == (TRNG_IRQ_DONE & pdu_io_read32 (hw->trng_irq_stat)))) {
#endif
         --cycles;
      }
      if (!cycles) {
         trng_dump_registers (hw);
         return ELPTRNG_TIMEOUT;
      }
      /* clear the status */
      pdu_io_write32 (hw->trng_irq_stat, TRNG_IRQ_DONE);
      return ELPTRNG_OK;
   }
}

/* start the rand reseed */
/* input: wait at end for irq status */
/* output: engine status */
int32_t trng_reseed_random (trng_hw * hw, uint32_t wait, int lock)
{
   unsigned long flags;

   if (hw == 0) {
      return ELPTRNG_INVALID_ARGUMENT;
   }

   if (ELPTRNG_NOT_INIT == hw->initialized) {
      return ELPTRNG_NOT_INITIALIZED;
   }

   if (lock) { PDU_LOCK(&hw->lock, flags); }
   pdu_io_write32 (hw->trng_ctrl, TRNG_RAND_RESEED);
   if (lock) { PDU_UNLOCK(&hw->lock, flags); }

   if (ELPTRNG_WAIT == wait) {
      return trng_wait_for_done (hw, LOOP_WAIT);
   }

   return ELPTRNG_OK;
}

/* set nonce seed */
/* input: *seed, the size has to be 32 bytes which is 8 32-bit words)
          The hw uses 128+127=255 bit) */
/* output: engine status */
int32_t trng_reseed_nonce (trng_hw * hw, uint32_t seed[TRNG_NONCE_SIZE_WORDS], int lock)
{
   int32_t ret;
   uint32_t n;
   unsigned long flags;

   if (hw == 0) {
      return ELPTRNG_INVALID_ARGUMENT;
   }

   ret = ELPTRNG_OK;

   if (lock) {
      PDU_LOCK(&hw->lock, flags);
   }
   if (ELPTRNG_NOT_INIT == hw->initialized) {
      ret = ELPTRNG_NOT_INITIALIZED;
   } else if (seed == NULL) {
      ret = ELPTRNG_INVALID_ARGUMENT;
   } else {
      /* write the lower words */
      pdu_io_write32 (hw->trng_ctrl, TRNG_NONCE_RESEED);
      for (n = 0; n < TRNG_DATA_SIZE_WORDS; n++) {
         pdu_io_write32 ((hw->trng_data + n), seed[n]);
      }
      pdu_io_write32 (hw->trng_ctrl, (TRNG_NONCE_RESEED | TRNG_NONCE_RESEED_LD));

      /* write the upper words */
      for (n = 0; n < TRNG_DATA_SIZE_WORDS; n++) {
         pdu_io_write32 ((hw->trng_data + n), seed[n + TRNG_DATA_SIZE_WORDS]);
      }
      pdu_io_write32 (hw->trng_ctrl, (TRNG_NONCE_RESEED | TRNG_NONCE_RESEED_LD | TRNG_NONCE_RESEED_SELECT));

      /* finish the operation */
      pdu_io_write32 (hw->trng_ctrl, 0);

      ret = trng_wait_for_done (hw, LOOP_WAIT);
   }
   if (lock) {
      PDU_UNLOCK(&hw->lock, flags);
   }
   return ret;
}

/* Initialize the trng. Optionally enable the IRQ and reseed if desired */
int32_t trng_init (trng_hw * hw, uint32_t reg_base, uint32_t enable_irq, uint32_t reseed)
{
   int32_t ret;

   if (hw == 0) {
      return ELPTRNG_INVALID_ARGUMENT;
   }

   ret = ELPTRNG_OK;
  
   PDU_INIT_LOCK(&hw->lock);

   /* set the register addresses */
#if (defined TRNG_HDAC)
   hw->trng_hdac_ctrl = (uint32_t *) (reg_base + TRNG_HDAC_CTRL);
   hw->trng_hdac_stat = (uint32_t *) (reg_base + TRNG_HDAC_STAT);
#endif
   hw->trng_ctrl     = (uint32_t *) (reg_base + TRNG_CTRL);
   hw->trng_irq_stat = (uint32_t *) (reg_base + TRNG_IRQ_STAT);
   hw->trng_irq_en   = (uint32_t *) (reg_base + TRNG_IRQ_EN);
   hw->trng_data     = (uint32_t *) (reg_base + TRNG_DATA);
   hw->trng_cfg      = (uint32_t *) (reg_base + TRNG_CFG);
   hw->initialized = ELPTRNG_INIT;

#if (defined TRNG_HDAC)
  trng_enable(hw);
#endif

   if (ELPTRNG_IRQ_PIN_ENABLE == enable_irq) {
      /* enable the interrupt pin and clear the status */
#if (defined TRNG_HDAC)
      pdu_io_write32 (hw->trng_hdac_ctrl, TRNG_HDAC_CTRL_IRQ_ENABLE | pdu_io_read32 (hw->trng_hdac_ctrl));   
#else
      pdu_io_write32 (hw->trng_irq_en, TRNG_IRQ_ENABLE);
#endif
      pdu_io_write32 (hw->trng_irq_stat, TRNG_IRQ_DONE);
   }

   if (ELPTRNG_RESEED == reseed) {
      uint32_t cfg = pdu_io_read32(hw->trng_cfg);

      if (((cfg >> TRNG_CFG_RINGS) & ((1ul << TRNG_CFG_RINGS_BITS)-1)) == 1) {
         /* reseed with the rings */
         ret = trng_reseed_random (hw, ELPTRNG_WAIT, 1);
         if (ret != ELPTRNG_OK) {
            hw->initialized = ELPTRNG_NOT_INIT;
         }
      } else {
         uint32_t seed[8];
         memset(seed, 0xFF, sizeof seed);
         ret = trng_reseed_nonce(hw, seed, 1);
      }
      
   }
   return ret;
}

#if (defined TRNG_HDAC)
void trng_enable(trng_hw* hw)
{
   /* for HDAC, this now acts as an ENABLE bit */
   pdu_io_write32 (hw->trng_irq_en, TRNG_IRQ_ENABLE);
}
#endif

/* Close the trng */
void trng_close (trng_hw * hw)
{
   /* clear the initialization flag and any settings */
   hw->initialized = ELPTRNG_NOT_INIT;
   pdu_io_write32 (hw->trng_ctrl, 0);
#if (defined TRNG_HDAC)
   pdu_io_write32 (hw->trng_hdac_ctrl, (~TRNG_HDAC_CTRL_IRQ_ENABLE) & pdu_io_read32 (hw->trng_hdac_ctrl));
#else
   pdu_io_write32 (hw->trng_irq_en, 0);
#endif
}

/* get a stream of random bytes */
/* input: *randbuf and it size, the size should be > 0, 
         s/w supports byte access even if h/w word (4 bytes) access */
/* output: data into randbuf and returns engine status */
int32_t trng_rand (trng_hw * hw, uint8_t * randbuf, uint32_t size, int lock)
{
   int32_t ret;
   uint32_t buf[TRNG_DATA_SIZE_WORDS];
   uint32_t i;
   uint32_t n;
   unsigned long flags;

   if ((hw == 0) || (randbuf == 0) || (size == 0)) {
      return ELPTRNG_INVALID_ARGUMENT;
   }

   ret = ELPTRNG_OK;

   if (lock) {
      PDU_LOCK(&hw->lock, flags);   
   }

   if (ELPTRNG_NOT_INIT == hw->initialized) {
      ret = ELPTRNG_NOT_INITIALIZED;
   } else if ((!randbuf) || (size == 0)) {
      ret = ELPTRNG_INVALID_ARGUMENT;
   } else {
      for (; size;) {
         /* This tests for a reseeding operation or a new value generation */
         if (pdu_io_read32 (hw->trng_ctrl) > 0) {
            if ((ret = trng_wait_for_done (hw, LOOP_WAIT)) != ELPTRNG_OK) {
               break;
            }
         }

         /* read out in maximum TRNG_DATA_SIZE_BYTES byte chunks */
         i = size > TRNG_DATA_SIZE_BYTES ? TRNG_DATA_SIZE_BYTES : size;

         for (n = 0; n < TRNG_DATA_SIZE_WORDS; n++) {
            buf[n] = pdu_io_read32 (hw->trng_data + n);
         }

         memcpy(randbuf, (uint8_t *) buf, i);
         randbuf += i;
         size -= i;

         /* request next data */
         pdu_io_write32 (hw->trng_ctrl, TRNG_GET_NEW);
         if ((ret = trng_wait_for_done (hw, LOOP_WAIT)) != ELPTRNG_OK) {
            break;
         }
      }
   }

   if (lock) {
      PDU_UNLOCK(&hw->lock, flags);
   }

   return ret;
}

