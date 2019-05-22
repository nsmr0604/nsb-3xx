#ifndef ELPTRNG3_HW_H_
#define ELPTRNG3_HW_H_

/* Control/status registers */
enum {
   TRNG3_CTRL,
   TRNG3_STAT,
   TRNG3_MODE,
   TRNG3_SMODE,
   TRNG3_IE,
   TRNG3_ISTAT,
   TRNG3_BUILD_ID,
   TRNG3_FEATURES,

   TRNG3_RAND_BASE = 0x20>>2,
   TRNG3_SEED_BASE = 0x40>>2,

   TRNG3_AUTO_RQST = 0x60>>2,
   TRNG3_AUTO_AGE,

   TRNG3_IA_RDATA = 0x70>>2,
   TRNG3_IA_WDATA,
   TRNG3_IA_ADDR,
   TRNG3_IA_CMD,
};

/* IRQ bitfields (used for both IE and ISTAT registers) */
#define TRNG3_IRQ_GLBL_EN    31
#define TRNG3_IRQ_RQST_ALARM  3
#define TRNG3_IRQ_AGE_ALARM   2
#define TRNG3_IRQ_SEED_DONE   1
#define TRNG3_IRQ_RAND_RDY    0

#define TRNG3_IRQ_GLBL_EN_MASK    (1ul<<TRNG3_IRQ_GLBL_EN)
#define TRNG3_IRQ_RQST_ALARM_MASK (1ul<<TRNG3_IRQ_RQST_ALARM)
#define TRNG3_IRQ_AGE_ALARM_MASK  (1ul<<TRNG3_IRQ_AGE_ALARM)
#define TRNG3_IRQ_SEED_DONE_MASK  (1ul<<TRNG3_IRQ_SEED_DONE)
#define TRNG3_IRQ_RAND_RDY_MASK   (1ul<<TRNG3_IRQ_RAND_RDY)

#define TRNG3_IRQ_ALL_MASK ( TRNG3_IRQ_GLBL_EN_MASK \
                           | TRNG3_IRQ_RQST_ALARM_MASK \
                           | TRNG3_IRQ_AGE_ALARM_MASK \
                           | TRNG3_IRQ_SEED_DONE_MASK \
                           | TRNG3_IRQ_RAND_RDY_MASK )

/* CTRL register commands */
enum {
   TRNG3_CMD_NOP,
   TRNG3_CMD_GEN_RAND,
   TRNG3_CMD_RAND_RESEED,
   TRNG3_CMD_NONCE_RESEED,
};

/* STAT register bitfields */
#define TRNG3_STAT_RESEEDING          31
#define TRNG3_STAT_GENERATING         30
#define TRNG3_STAT_SRVC_RQST          27
#define TRNG3_STAT_RESEED_REASON      16
#define TRNG3_STAT_RESEED_REASON_BITS  3
#define TRNG3_STAT_SEEDED              9
#define TRNG3_STAT_SECURE              8
#define TRNG3_STAT_R256                3
#define TRNG3_STAT_NONCE_MODE          2

/* STAT.RESEED_REASON values */
enum {
   TRNG3_RESEED_HOST     = 0,
   TRNG3_RESEED_NONCE    = 3,
   TRNG3_RESEED_PIN      = 4,
   TRNG3_RESEED_UNSEEDED = 7,
};

/* MODE register bitfields */
#define TRNG3_MODE_R256               3

/* SMODE register bitfields */
#define TRNG3_SMODE_MAX_REJECTS      16
#define TRNG3_SMODE_MAX_REJECTS_BITS  8
#define TRNG3_SMODE_SECURE            8
#define TRNG3_SMODE_NONCE             2

/* FEATURES register bitfields */
#define TRNG3_FEATURES_DIAG_LEVEL      4
#define TRNG3_FEATURES_DIAG_LEVEL_BITS 3
#define TRNG3_FEATURES_SECURE_RST      3
#define TRNG3_FEATURES_RAND_SEED       2
#define TRNG3_FEATURES_RAND_LEN        0
#define TRNG3_FEATURES_RAND_LEN_BITS   2

/* BUILD_ID register bitfields */
#define TRNG3_BUILD_ID_EPN           0
#define TRNG3_BUILD_ID_EPN_BITS     16
#define TRNG3_BUILD_ID_STEPPING     28
#define TRNG3_BUILD_ID_STEPPING_BITS 4

/* AUTO_RQST register bitfields */
#define TRNG3_AUTO_RQST_RQSTS        0
#define TRNG3_AUTO_RQST_RQSTS_BITS  16
#define TRNG3_AUTO_RQST_RESOLUTION  16ul

/* AUTO_AGE register bitfields */
#define TRNG3_AUTO_AGE_AGE           0
#define TRNG3_AUTO_AGE_AGE_BITS     16
#define TRNG3_AUTO_AGE_RESOLUTION   (1ul << 26)

/* IA_CMD register bitfields */
#define TRNG3_IA_CMD_GO   31
#define TRNG3_IA_CMD_WRITE 0

#endif
