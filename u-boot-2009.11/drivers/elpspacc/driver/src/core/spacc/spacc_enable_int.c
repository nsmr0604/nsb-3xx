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

#include "elpspacc.h"

/* cmdx and cmdx_cnt depend on HW config */
/* cmdx can be 0, 1 or 2 */
/* cmdx_cnt must be 2^6 or less */
void spacc_irq_cmdx_enable (spacc_device *spacc, int cmdx, int cmdx_cnt)
{
   uint32_t temp;

   //printk("CMDX enable\n");
   /* read the reg, clear the bit range and set the new value */
   temp = pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_CTRL) & (~SPACC_IRQ_CTRL_CMDX_CNT_MASK(cmdx));
   temp |= SPACC_IRQ_CTRL_CMDX_CNT_SET(cmdx, cmdx_cnt);
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_CTRL, temp | SPACC_IRQ_CTRL_CMDX_CNT_SET(cmdx, cmdx_cnt));

   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN,  pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) | SPACC_IRQ_EN_CMD(cmdx));
}

void spacc_irq_cmdx_disable (spacc_device *spacc, int cmdx)
{
   //printk("CMDX disable\n");
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN,  pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) & (~SPACC_IRQ_EN_CMD(cmdx)));
   //printk("IRQ_EN == 0x%08zx\n", pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN));
}


void spacc_irq_stat_enable (spacc_device *spacc, int stat_cnt)
{
   uint32_t temp;

   temp = pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_CTRL);
   if (spacc->config.is_qos) {
      temp &= (~SPACC_IRQ_CTRL_STAT_CNT_MASK_QOS);
      temp |= SPACC_IRQ_CTRL_STAT_CNT_SET_QOS(stat_cnt);
   } else {
      temp &= (~SPACC_IRQ_CTRL_STAT_CNT_MASK);
      temp |= SPACC_IRQ_CTRL_STAT_CNT_SET(stat_cnt);
   }
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_CTRL, temp);
   //printk("stat_enable\nIRQ_CTRL == 0x%08zx\n", temp);
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) | SPACC_IRQ_EN_STAT);
   //printk("IRQ_EN   == 0x%08zx\n", pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN));
}

void spacc_irq_stat_disable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) & (~SPACC_IRQ_EN_STAT));
}

void spacc_irq_stat_wd_enable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) | SPACC_IRQ_EN_STAT_WD);
}

void spacc_irq_stat_wd_disable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) & (~SPACC_IRQ_EN_STAT_WD));
}


void spacc_irq_rc4_dma_enable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) | SPACC_IRQ_EN_RC4_DMA);
}

void spacc_irq_rc4_dma_disable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) & (~SPACC_IRQ_EN_RC4_DMA));
}

void spacc_irq_glbl_enable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) | SPACC_IRQ_EN_GLBL);
}

void spacc_irq_glbl_disable (spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN, pdu_io_read32(spacc->regmap + SPACC_REG_IRQ_EN) & (~SPACC_IRQ_EN_GLBL));
}

