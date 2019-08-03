/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/drivers/block/blkfdc.h                                   *
 * Created:     2010-08-11 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2010-2012 Hampa Hug <hampa@hampa.ch>                     *
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


#ifndef PCE_DEVICES_BLOCK_BLKFDC_H
#define PCE_DEVICES_BLOCK_BLKFDC_H 1


#include <config.h>

#include <drivers/block/block.h>

#include <drivers/pfdc/pfdc.h>
#include <drivers/pfdc/pfdc-img.h>

#include <stdio.h>
#include <stdint.h>


#define PCE_BLK_FDC_OK        0x00
#define PCE_BLK_FDC_NO_ID     0x01	/* sector id not found */
#define PCE_BLK_FDC_NO_DATA   0x02	/* sector data not found */
#define PCE_BLK_FDC_CRC_ID    0x04
#define PCE_BLK_FDC_CRC_DATA  0x08	/* data crc error */
#define PCE_BLK_FDC_DEL_DAM   0x10	/* deleted data adress mark */
#define PCE_BLK_FDC_DATALEN   0x20	/* sector is smaller than requested */
#define PCE_BLK_FDC_WPROT     0x40	/* write protected */


/*!***************************************************************************
 * @short The fdc disk structure
 *****************************************************************************/
typedef struct {
	disk_t     dsk;

	pfdc_img_t *img;

	char       dirty;

	unsigned   encoding;

	unsigned   type;
	char       *fname;
} disk_fdc_t;


unsigned dsk_fdc_read_chs (disk_fdc_t *fdc, void *buf, unsigned *cnt,
	unsigned c, unsigned h, unsigned s, int phy
);

unsigned dsk_fdc_read_tags (disk_fdc_t *fdc, void *buf, unsigned cnt,
	unsigned c, unsigned h, unsigned s, int phy
);

unsigned dsk_fdc_write_chs (disk_fdc_t *fdc, const void *buf, unsigned *cnt,
	unsigned c, unsigned h, unsigned s, int phy
);

unsigned dsk_fdc_write_tags (disk_fdc_t *fdc, const void *buf, unsigned cnt,
	unsigned c, unsigned h, unsigned s, int phy
);

int dsk_fdc_erase_track (disk_fdc_t *fdc, unsigned c, unsigned h);

int dsk_fdc_erase_disk (disk_fdc_t *fdc);

void dsk_fdc_set_encoding (disk_fdc_t *fdc, unsigned enc);

int dsk_fdc_format_sector (disk_fdc_t *fdc,
	unsigned pc, unsigned ph, unsigned c, unsigned h, unsigned s,
	unsigned cnt, unsigned fill
);

int dsk_fdc_read_id (disk_fdc_t *fdc,
	unsigned pc, unsigned ph, unsigned ps,
	unsigned *c, unsigned *h, unsigned *s, unsigned *cnt, unsigned *cnt_id
);


disk_t *dsk_fdc_open_fp (FILE *fp, unsigned type, int ro);
disk_t *dsk_fdc_open (const char *fname, unsigned type, int ro);


int dsk_fdc_create_fp (FILE *fp, unsigned type, uint32_t c, uint32_t h, uint32_t s);
int dsk_fdc_create (const char *fname, unsigned type, uint32_t c, uint32_t h, uint32_t s);


unsigned dsk_fdc_probe_fp (FILE *fp);
unsigned dsk_fdc_probe (const char *fname);


#endif
