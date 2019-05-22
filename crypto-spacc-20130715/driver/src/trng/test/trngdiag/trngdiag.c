/*-----------------------------------------------------------------------
//
// Proprietary Information of Elliptic Technologies
// Copyright (C) 2002-2010, all rights reserved
// Elliptic Technologies Inc.
//
// As part of our confidentiality  agreement, Elliptic  Technologies and
// the Company, as  a  Receiving Party, of  this  information  agrees to
// keep strictly  confidential  all Proprietary Information  so received
// from Elliptic  Technologies. Such Proprietary Information can be used
// solely for  the  purpose  of evaluating  and/or conducting a proposed
// business  relationship  or  transaction  between  the  parties.  Each
// Party  agrees  that  any  and  all  Proprietary  Information  is  and
// shall remain confidential and the property of Elliptic  Technologies.
// The  Company  may  not  use  any of  the  Proprietary  Information of
// Elliptic  Technologies for any purpose other  than  the  above-stated
// purpose  without the prior written consent of Elliptic  Technologies.
//
//-----------------------------------------------------------------------
//
// Project:
//
//   TRNG SDK 
//
// Description:
//
//    Command line diagnostic interface
//
//-----------------------------------------------------------------------*/

#include "elptrng.h"        /* for trng interface */
#include "trngdiag.h"

#define READBUFSIZE TRNG_DATA_SIZE_BYTES

void dump_bytes (uint8_t * buf, uint32_t len)
{
   uint32_t i;
   for (i = 0; i < len; i++) {
      ELPHW_PRINT ("%02X", buf[i]);
   }
}

/* Useful function to get data from the trng with optional nonce seeding */
static int32_t prng_get_data (trng_hw * trng, uint8_t * sw_buffer, uint32_t buffersize, uint8_t do_nonce, uint32_t *seed)
{
   uint8_t seed_buffer[TRNG_NONCE_SIZE_BYTES];
   int32_t i, res;

   if (do_nonce) {
      /* Set the nonce with system prng data */
      for (i = 0; i < TRNG_NONCE_SIZE_BYTES; i++) {
         seed_buffer[i] = (uint8_t) (*seed);
         (*seed) += 1;
      }
      if ((res = trng_reseed_nonce (trng, (uint32_t *) seed_buffer, 1)) != ELPTRNG_OK) {
         ELPHW_PRINT ("prng_get_data: Error: can't seed buffer '%d'..... FAILED\n", res);
         return res;
      }
   }

   memset (sw_buffer, 0, sizeof (sw_buffer));
   if ((res = trng_rand (trng, sw_buffer, buffersize, 1)) != ELPTRNG_OK) {
      ELPHW_PRINT ("prng_get_data: Error: can't generate random number ..... FAILED [%d]\n", res);
      return res;
   }

   return 0;
}

#ifdef TRNGDIAG_GENERATE_FILE
/* Write buffers of rng data to a file 'rand.bin' */
static int32_t write_rng (trng_hw * trng, uint32_t bytes_per_buf, uint32_t iterations)
{
   unsigned char buf[bytes_per_buf];
   FILE *out;
   int i, res;

   if ((res = trng_reseed_random (trng, ELPTRNG_WAIT, 1)) != ELPTRNG_OK) {
      ELPHW_PRINT ("write_rng: trng_reseed_random failed %d\n", res);
      return res;
   }

   out = fopen ("rand.bin", "wb");
   if (!out) {
      perror ("cannot open output file rand.bin");
      return 0;
   }

   for (i = 0; i < iterations; i++) {
      if ((res = trng_rand (trng, buf, sizeof (buf), 1)) < 0) {
         ELPHW_PRINT ("rng_test: Error: can't generate random number ..... FAILED [%d]\n", res);
         return res;
      }
      fwrite (buf, 1, sizeof (buf), out);
   }
   fclose (out);

   return 0;
}
#endif

#ifdef DO_PCIM
unsigned PCI_DID = PDU_PCI_DID;
#endif

/* The main test program. */
/* This includes code to initialize the TRNG as it is hosted on an FPGA based PCI card */

int trng_diag_pci(uint32_t base_addr, uint32_t seedval)
{
   int32_t ret;
   uint8_t buff[READBUFSIZE], buff2[READBUFSIZE];
   trng_hw trng;
   uint32_t seed = seedval;
#ifdef DO_PCIM
   struct pci_dev *dev;
#endif   

   /* Initialize trng hardware */
   /* Don't enable IRQ pin and don't reseed */
   ret = trng_init (&trng, base_addr, ELPTRNG_IRQ_PIN_DISABLE, ELPTRNG_NO_RESEED);
   if (ret != ELPTRNG_OK) {
      ELPHW_PRINT ("ERR[line %d]: trng_init: %d\n", __LINE__, ret);
      return ret;
   }
   /* Dump the registers to see if they make sense */
   trng_dump_registers (&trng);
   
   trng_close (&trng);

   /* Initialize trng hardware */
   /* Enable IRQ pin and reseed */
   ret = trng_init (&trng, base_addr, ELPTRNG_IRQ_PIN_ENABLE, ELPTRNG_RESEED);
   if (ret != ELPTRNG_OK) {
      ELPHW_PRINT ("ERR[line %d]: trng_init: %d\n", __LINE__, ret);
      return ret;
   }
   /* Dump the registers to see if they make sense */
   trng_dump_registers (&trng);
   ELPHW_PRINT ("trng_init: [PASSED]\n");
   ELPHW_PRINT ("\n");

   /* Start the actual tests */

   /* Test 1: */
   /* Should be able to read random data */

   ELPHW_PRINT ("Test 1: Should be able to read random data\n");

   if ((ret = trng_rand (&trng, buff, READBUFSIZE, 1)) != ELPTRNG_OK) {
      ELPHW_PRINT ("trng_rand: Error: can't generate random number ..... FAILED [%d]\n", ret);
      return ret;
   } else {
      ELPHW_PRINT ("Rand data: ");
      dump_bytes (buff, READBUFSIZE);
      ELPHW_PRINT (" [PASSED]\n");
   }
   ELPHW_PRINT ("\n");

   /* Test 2: */
   /* Given different initial nonce seeds the rng data is different */

   ELPHW_PRINT ("Test 2: Unique Nonce Reseed test: \n");

   /* Get random data with a random nonce seed */
   seed = seedval; /* Seed the system prng with a time stamp */
   ret = prng_get_data (&trng, buff2, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Reseed 1: \t");
   dump_bytes (buff2, READBUFSIZE);
   ELPHW_PRINT ("\n");

   /* Get random data with a random nonce seed */
   /* The system prng will create nonce with it's next unique data */
   ret = prng_get_data (&trng, buff, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Reseed 2: \t");
   dump_bytes (buff, READBUFSIZE);

   /* Compare the data buffers */
   if (memcmp (buff, buff2, READBUFSIZE) != 0) {
      ELPHW_PRINT (" [PASSED]\n");
   } else {
      ELPHW_PRINT (" [FAILED]\n");
      return -1;
   }
   ELPHW_PRINT ("\n");

   /* Test 3: */
   /* Given the same initial nonce seed the rng data is the same */

   ELPHW_PRINT ("Test 3: Same Nonce Reseed test:\n");

   seed = seedval;
   ret = prng_get_data (&trng, buff, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Reseed 1: \t");
   dump_bytes (buff, READBUFSIZE);
   ELPHW_PRINT ("\n");

   seed = seedval;
   ret = prng_get_data (&trng, buff2, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Reseed 2: \t");
   dump_bytes (buff2, READBUFSIZE);

   if (memcmp (buff, buff2, READBUFSIZE) == 0) {
      ELPHW_PRINT (" [PASSED]\n");
   } else {
      ELPHW_PRINT (" [FAILED]\n");
      return -1;
   }
   ELPHW_PRINT ("\n");

#ifdef DO_PCIM
   /* Test 4: */
   /* Reset the hardware and test to see if a nonce reseed is correct */

   ELPHW_PRINT ("Test 4: Nonce Reseed while initial seed is underway:\n");

   seed = seedval;
   ret = prng_get_data (&trng, buff2, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Run 1: \t\t");
   dump_bytes (buff, READBUFSIZE);
   ELPHW_PRINT ("\n");

   /* reset hardware which should kick off hw reseed */
   dev = pci_get_device (0xE117, PCI_DID, NULL);
   pdu_pci_reset (dev);
#if (defined TRNG_HDAC)
   trng_enable(&trng);
#endif

   seed = seedval;
   ret = prng_get_data (&trng, buff, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Run 2: \t\t");
   dump_bytes (buff, READBUFSIZE);

   if (memcmp (buff, buff2, READBUFSIZE) == 0) {
      ELPHW_PRINT (" [PASSED]\n ");
   } else {
      ELPHW_PRINT (" [FAILED]\n");
      return -1;
   }
   ELPHW_PRINT ("\n");
#endif

   /* Test  5: */
   /* Force a nonce reseed while a random reseed is processing */

   ELPHW_PRINT ("Test 5: Nonce Reseed while random reseed is underway:\n");

   seed = seedval;
   ret = prng_get_data (&trng, buff2, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data_nonce: %d\n", __LINE__, ret);
   }
   ELPHW_PRINT ("Run 1: \t\t");
   dump_bytes (buff2, READBUFSIZE);
   ELPHW_PRINT ("\n");

   /* start a random reseed operation without waiting */
   if ((ret = trng_reseed_random (&trng, ELPTRNG_NO_WAIT)) != ELPTRNG_OK) {
      ELPHW_PRINT ("trng_reseed_random: failed %d\n", ret);
      return ret;
   }

   seed = seedval;
   ret = prng_get_data (&trng, buff, READBUFSIZE, 1, &seed);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: prng_get_data: %d\n", __LINE__, ret);
   }

   ELPHW_PRINT ("Run 2: \t\t");
   dump_bytes (buff, READBUFSIZE);
   /* previous buffer should be the same as this one */
   if (memcmp (buff, buff2, READBUFSIZE) == 0) {
      ELPHW_PRINT (" [PASSED]\n");
   } else {
      ELPHW_PRINT (" [FAILED]\n");
      return -1;
   }
   ELPHW_PRINT ("\n");

#ifdef TRNGDIAG_GENERATE_FILE
   /* Test 6: */
   /* Generate a large amount of random data */

   ret = write_rng (&trng, 1024, 500);
   if (ret != 0) {
      ELPHW_PRINT ("ERR[line %d]: write_rng: %d\n", __LINE__, ret);
      return ret;
   }
   ELPHW_PRINT ("Large generate test: Check file 'rand.bin': [PASSED]\n");
#endif

   trng_close (&trng);

   return ret;
}
