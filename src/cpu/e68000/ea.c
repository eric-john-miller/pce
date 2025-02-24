/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/cpu/e68000/ea.c                                          *
 * Created:     2006-05-17 by Hampa Hug <hampa@hampa.ch>                     *
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "e68000.h"
#include "internal.h"


/*
 * valid ea mask:
 *  0 001: Dx
 *  1 002: Ax
 *  2 004: (Ax)
 *  3 008: (Ax)+
 *  4 010: -(Ax)
 *  5 020: XXXX(Ax)
 *  6 040: XX(Ax, Rx.S)
 *  7 080: XXXX
 *  8 100: XXXXXXXX
 *  9 200: XXXX(PC)
 * 10 400: XX(PC, Rx.S)
 * 11 800: #XXXX
*/


/* Dx */
static
int e68_ea_000_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	if ((mask & 0x0001) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_REG;
	c->ea_val = ea & 7;

	return (0);
}

/* Ax */
static
int e68_ea_001_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	if ((mask & 0x0002) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_REG;
	c->ea_val = (ea & 7) + 8;

	return (0);
}

/* (Ax) */
static
int e68_ea_010_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	if ((mask & 0x0004) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_areg32 (c, ea & 7);

	return (0);
}

/* (Ax)+ */
static
int e68_ea_011_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	unsigned reg;

	if ((mask & 0x0008) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	reg = ea & 7;

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_areg32 (c, reg);

	e68_set_areg32 (c, reg, c->ea_val + (size >> 3));

	return (0);
}

/* (A7)+ */
static
int e68_ea_011_111 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	if ((mask & 0x0008) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	if (size == 8) {
		size = 16;
	}

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_areg32 (c, 7);

	e68_set_areg32 (c, 7, c->ea_val + (size >> 3));

	return (0);
}

/* -(Ax) */
static
int e68_ea_100_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	unsigned reg;

	if ((mask & 0x0010) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	reg = ea & 7;

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_areg32 (c, reg) - (size >> 3);

	e68_set_areg32 (c, reg, c->ea_val);
	e68_set_clk (c, 2);

	return (0);
}

/* -(A7) */
static
int e68_ea_100_111 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	if ((mask & 0x0010) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	if (size == 8) {
		size = 16;
	}

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_areg32 (c, 7) - (size >> 3);

	e68_set_areg32 (c, 7, c->ea_val);
	e68_set_clk (c, 2);

	return (0);
}

/* XXXX(Ax) */
static
int e68_ea_101_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;

	if ((mask & 0x0020) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);

	c->ea_val = e68_get_areg32 (c, ea & 7) + e68_exts16 (ir1[0]);

	e68_set_clk (c, 4);

	return (0);
}

/* XX(Ax, Rx.S) */
static
int e68_ea_110_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;
	uint32_t idx;

	if ((mask & 0x0040) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);

	c->ea_val = e68_get_areg32 (c, ea & 7) + e68_exts8 (ir1[0]);

	if (ir1[0] & 0x8000) {
		idx = e68_get_areg32 (c, (ir1[0] >> 12) & 7);
	}
	else {
		idx = e68_get_dreg32 (c, (ir1[0] >> 12) & 7);
	}

	if ((ir1[0] & 0x0800) == 0) {
		idx = e68_exts16 (idx);
	}

	c->ea_val += idx;

	e68_set_clk (c, 6);

	return (0);
}

/* XXXX */
static
int e68_ea_111_000 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;

	if ((mask & 0x0080) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);

	c->ea_val = e68_exts16 (ir1[0]);

	e68_set_clk (c, 4);

	return (0);
}

/* XXXXXXXX */
static
int e68_ea_111_001 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;

	if ((mask & 0x0100) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);
	e68_ifetch_next (c);

	c->ea_val = ((uint32_t) ir1[0] << 16) | ir1[1];

	e68_set_clk (c, 8);

	return (0);
}

/* XXXX(PC) */
static
int e68_ea_111_010 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;

	if ((mask & 0x0200) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;
	c->ea_val = e68_get_pc (c) + 2 * c->ircnt;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);

	c->ea_val += e68_exts16 (ir1[0]);

	e68_set_clk (c, 4);

	return (0);
}

/* XX(PC, Rx.S) */
static
int e68_ea_111_011 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;
	uint32_t idx;

	if ((mask & 0x0400) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_MEM;

	c->ea_val = e68_get_pc (c) + 2 * c->ircnt;

	ir1 = &c->ir[c->ircnt];
	e68_ifetch_next (c);

	c->ea_val += e68_exts8 (ir1[0]);

	if (ir1[0] & 0x8000) {
		idx = e68_get_areg32 (c, (ir1[0] >> 12) & 7);
	}
	else {
		idx = e68_get_dreg32 (c, (ir1[0] >> 12) & 7);
	}

	if ((ir1[0] & 0x0800) == 0) {
		idx = e68_exts16 (idx);
	}

	c->ea_val += idx;

	e68_set_clk (c, 6);

	return (0);
}

/* #XXXX */
static
int e68_ea_111_100 (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	uint16_t *ir1;

	if ((mask & 0x0800) == 0) {
		e68_exception_illegal (c);
		return (1);
	}

	c->ea_typ = E68_EA_TYPE_IMM;

	ir1 = &c->ir[c->ircnt];

	if (size == 32) {
		e68_ifetch_next (c);
		e68_ifetch_next (c);
		c->ea_val = ((uint32_t) ir1[0] << 16) | ir1[1];
		e68_set_clk (c, 8);
	}
	else if (size == 16) {
		e68_ifetch_next (c);
		c->ea_val = ir1[0];
		e68_set_clk (c, 4);
	}
	else {
		e68_ifetch_next (c);
		c->ea_val = ir1[0] & 0xff;
		e68_set_clk (c, 4);
	}

	return (0);
}

static
int e68_ea_111_xxx (e68000_t *c, unsigned ea, unsigned mask, unsigned size)
{
	e68_exception_illegal (c);

	return (1);
}

e68_get_ea_ptr_f e68_ea_tab[64] = {
	e68_ea_000_xxx, e68_ea_000_xxx, e68_ea_000_xxx, e68_ea_000_xxx,
	e68_ea_000_xxx, e68_ea_000_xxx, e68_ea_000_xxx, e68_ea_000_xxx,

	e68_ea_001_xxx, e68_ea_001_xxx, e68_ea_001_xxx, e68_ea_001_xxx,
	e68_ea_001_xxx, e68_ea_001_xxx, e68_ea_001_xxx, e68_ea_001_xxx,

	e68_ea_010_xxx, e68_ea_010_xxx, e68_ea_010_xxx, e68_ea_010_xxx,
	e68_ea_010_xxx, e68_ea_010_xxx, e68_ea_010_xxx, e68_ea_010_xxx,

	e68_ea_011_xxx, e68_ea_011_xxx, e68_ea_011_xxx, e68_ea_011_xxx,
	e68_ea_011_xxx, e68_ea_011_xxx, e68_ea_011_xxx, e68_ea_011_111,

	e68_ea_100_xxx, e68_ea_100_xxx, e68_ea_100_xxx, e68_ea_100_xxx,
	e68_ea_100_xxx, e68_ea_100_xxx, e68_ea_100_xxx, e68_ea_100_111,

	e68_ea_101_xxx, e68_ea_101_xxx, e68_ea_101_xxx, e68_ea_101_xxx,
	e68_ea_101_xxx, e68_ea_101_xxx, e68_ea_101_xxx, e68_ea_101_xxx,

	e68_ea_110_xxx, e68_ea_110_xxx, e68_ea_110_xxx, e68_ea_110_xxx,
	e68_ea_110_xxx, e68_ea_110_xxx, e68_ea_110_xxx, e68_ea_110_xxx,

	e68_ea_111_000, e68_ea_111_001, e68_ea_111_010, e68_ea_111_011,
	e68_ea_111_100, e68_ea_111_xxx, e68_ea_111_xxx, e68_ea_111_xxx,
};


int e68_ea_get_val8 (e68000_t *c, uint8_t *val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		*val = c->ea_val & 0xff;
		return (0);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			*val = e68_get_dreg8 (c, c->ea_val);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		*val = e68_get_mem8 (c, c->ea_val);
		e68_set_clk (c, 4);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}

int e68_ea_get_val16 (e68000_t *c, uint16_t *val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		*val = c->ea_val & 0xffff;
		return (0);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			*val = e68_get_dreg16 (c, c->ea_val);
		}
		else if (c->ea_val < 16) {
			*val = e68_get_areg16 (c, c->ea_val - 8);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		if (c->ea_val & 1) {
			if ((c->flags & E68_FLAG_NOADDR) == 0) {
				e68_exception_address (c, c->ea_val, 1, 0);
				return (1);
			}
		}

		*val = e68_get_mem16 (c, c->ea_val);
		e68_set_clk (c, 4);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}

int e68_ea_get_val32 (e68000_t *c, uint32_t *val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		*val = c->ea_val & 0xffffffff;
		return (0);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			*val = e68_get_dreg32 (c, c->ea_val);
		}
		else if (c->ea_val < 16) {
			*val = e68_get_areg32 (c, c->ea_val - 8);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		if (c->ea_val & 1) {
			if ((c->flags & E68_FLAG_NOADDR) == 0) {
				e68_exception_address (c, c->ea_val, 1, 0);
				return (1);
			}
		}

		*val = e68_get_mem32 (c, c->ea_val);
		e68_set_clk (c, 8);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}

int e68_ea_set_val8 (e68000_t *c, uint8_t val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		e68_exception_illegal (c);
		return (1);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			e68_set_dreg8 (c, c->ea_val, val);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		e68_set_mem8 (c, c->ea_val, val);
		e68_set_clk (c, 4);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}

int e68_ea_set_val16 (e68000_t *c, uint16_t val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		e68_exception_illegal (c);
		return (1);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			e68_set_dreg16 (c, c->ea_val, val);
		}
		else if (c->ea_val < 16) {
			e68_set_areg16 (c, c->ea_val - 8, val);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		if (c->ea_val & 1) {
			if ((c->flags & E68_FLAG_NOADDR) == 0) {
				e68_exception_address (c, c->ea_val, 1, 1);
				return (1);
			}
		}

		e68_set_mem16 (c, c->ea_val, val);
		e68_set_clk (c, 4);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}

int e68_ea_set_val32 (e68000_t *c, uint32_t val)
{
	switch (c->ea_typ) {
	case E68_EA_TYPE_IMM:
		e68_exception_illegal (c);
		return (1);

	case E68_EA_TYPE_REG:
		if (c->ea_val < 8) {
			e68_set_dreg32 (c, c->ea_val, val);
		}
		else if (c->ea_val < 16) {
			e68_set_areg32 (c, c->ea_val - 8, val);
		}
		else {
			e68_exception_illegal (c);
			return (1);
		}
		return (0);

	case E68_EA_TYPE_MEM:
		if (c->ea_val & 1) {
			if ((c->flags & E68_FLAG_NOADDR) == 0) {
				e68_exception_address (c, c->ea_val, 1, 1);
				return (1);
			}
		}

		e68_set_mem32 (c, c->ea_val, val);
		e68_set_clk (c, 8);
		return (0);
	}

	e68_exception_illegal (c);

	return (1);
}
