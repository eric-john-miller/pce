/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/cpu/e68000/internal.h                                    *
 * Created:     2005-07-17 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2005-2009 Hampa Hug <hampa@hampa.ch>                     *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/


#ifndef PCE_E68000_INTERNAL_H
#define PCE_E68000_INTERNAL_H 1


#include <stdlib.h>
#include <stdio.h>


#define e68_set_clk(c, clk) do { (c)->delay += (clk); } while (0)


#define E68_SR_XC (E68_SR_X | E68_SR_C)
#define E68_SR_NZVC (E68_SR_N | E68_SR_Z | E68_SR_V | E68_SR_C)
#define E68_SR_XNVC (E68_SR_X | E68_SR_N | E68_SR_V | E68_SR_C)
#define E68_SR_XNZVC (E68_SR_X | E68_SR_N | E68_SR_Z | E68_SR_V | E68_SR_C)

#define E68_CCR_MASK (E68_SR_X | E68_SR_N | E68_SR_Z | E68_SR_V | E68_SR_C)
#define E68_SR_MASK (E68_CCR_MASK | E68_SR_S | E68_SR_T | E68_SR_I)


#define E68_FLAG_NOADDR 1
#define E68_FLAG_68010  2


void e68_set_sr (e68000_t *c, unsigned short val);


static inline
uint32_t e68_exts8 (uint32_t val)
{
	return ((val & 0x80) ? (val | 0xffffff00) : (val & 0x000000ff));
}

static inline
uint32_t e68_exts16 (uint32_t val)
{
	return ((val & 0x8000) ? (val | 0xffff0000) : (val & 0x0000ffff));
}

static inline
void e68_set_ccr (e68000_t *c, uint8_t val)
{
	c->sr = (c->sr & 0xff00) | (val & 0x00ff);
}

static inline
void e68_set_iml (e68000_t *c, unsigned val)
{
	c->sr &= 0xf8ff;
	c->sr |= (val & 7) << 8;
}

static inline
void e68_set_usp (e68000_t *c, uint32_t val)
{
	if (c->supervisor) {
		c->usp = val;
	}
	else {
		c->areg[7] = val;
	}
}

static inline
void e68_set_ssp (e68000_t *c, uint32_t val)
{
	if (c->supervisor) {
		c->areg[7] = val;
	}
	else {
		c->ssp = val;
	}
}

static inline
void e68_push16 (e68000_t *c, uint16_t val)
{
	uint32_t sp;

	sp = (e68_get_areg32 (c, 7) - 2) & 0xffffffff;

	e68_set_mem16 (c, sp, val);
	e68_set_areg32 (c, 7, sp);
}

static inline
void e68_push32 (e68000_t *c, uint32_t val)
{
	uint32_t sp;

	sp = (e68_get_areg32 (c, 7) - 4) & 0xffffffff;

	e68_set_mem32 (c, sp, val);
	e68_set_areg32 (c, 7, sp);
}

static inline
uint16_t e68_get_uint16 (const void *buf, unsigned i)
{
	uint16_t            r;
	const unsigned char *tmp = (const unsigned char *) buf + i;

	r = tmp[0] & 0xff;
	r = (r << 8) | (tmp[1] & 0xff);

	return (r);
}

static inline
uint32_t e68_get_uint32 (const void *buf, unsigned i)
{
	uint32_t            r;
	const unsigned char *tmp = (const unsigned char *) buf + i;

	r = tmp[0] & 0xff;
	r = (r << 8) | (tmp[1] & 0xff);
	r = (r << 8) | (tmp[2] & 0xff);
	r = (r << 8) | (tmp[3] & 0xff);

	return (r);
}

static inline
void e68_ifetch (e68000_t *c, unsigned i)
{
	c->ir[i] = e68_get_mem16 (c, c->pc + 2 * i);
	c->ircnt = i + 1;
}

static inline
void e68_ifetch_next (e68000_t *c)
{
	c->ir[c->ircnt] = e68_get_mem16 (c, c->pc + 2 * c->ircnt);
	c->ircnt += 1;
}


void e68_cc_set_nz_8 (e68000_t *c, uint8_t msk, uint8_t val);
void e68_cc_set_nz_16 (e68000_t *c, uint8_t msk, uint16_t val);
void e68_cc_set_nz_32 (e68000_t *c, uint8_t msk, uint32_t val);

void e68_cc_set_add_8 (e68000_t *c, uint8_t d, uint8_t s1, uint8_t s2);
void e68_cc_set_add_16 (e68000_t *c, uint16_t d, uint16_t s1, uint16_t s2);
void e68_cc_set_add_32 (e68000_t *c, uint32_t d, uint32_t s1, uint32_t s2);

void e68_cc_set_addx_8 (e68000_t *c, uint8_t d, uint8_t s1, uint8_t s2);
void e68_cc_set_addx_16 (e68000_t *c, uint16_t d, uint16_t s1, uint16_t s2);
void e68_cc_set_addx_32 (e68000_t *c, uint32_t d, uint32_t s1, uint32_t s2);

void e68_cc_set_cmp_8 (e68000_t *c, uint8_t d, uint8_t s1, uint8_t s2);
void e68_cc_set_cmp_16 (e68000_t *c, uint16_t d, uint16_t s1, uint16_t s2);
void e68_cc_set_cmp_32 (e68000_t *c, uint32_t d, uint32_t s1, uint32_t s2);

void e68_cc_set_sub_8 (e68000_t *c, uint8_t d, uint8_t s1, uint8_t s2);
void e68_cc_set_sub_16 (e68000_t *c, uint16_t d, uint16_t s1, uint16_t s2);
void e68_cc_set_sub_32 (e68000_t *c, uint32_t d, uint32_t s1, uint32_t s2);

void e68_cc_set_subx_8 (e68000_t *c, uint8_t d, uint8_t s1, uint8_t s2);
void e68_cc_set_subx_16 (e68000_t *c, uint16_t d, uint16_t s1, uint16_t s2);
void e68_cc_set_subx_32 (e68000_t *c, uint32_t d, uint32_t s1, uint32_t s2);


#define E68_EA_TYPE_IMM 0
#define E68_EA_TYPE_REG 1
#define E68_EA_TYPE_MEM 2

typedef int (*e68_get_ea_ptr_f) (e68000_t *c, unsigned ea, unsigned mask, unsigned size);

e68_get_ea_ptr_f e68_ea_tab[64];

static inline
int e68_ea_get_ptr (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	return (e68_ea_tab[ea] (c, ea, mask, size));
}

int e68_ea_get_val8 (e68000_t *c, uint8_t *val);
int e68_ea_get_val16 (e68000_t *c, uint16_t *val);
int e68_ea_get_val32 (e68000_t *c, uint32_t *val);
int e68_ea_set_val8 (e68000_t *c, uint8_t val);
int e68_ea_set_val16 (e68000_t *c, uint16_t val);
int e68_ea_set_val32 (e68000_t *c, uint32_t val);


void e68_set_opcodes (e68000_t *c);


#endif
