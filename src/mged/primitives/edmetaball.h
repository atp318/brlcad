/*                      E D M E T A B A L L . H
 * BRL-CAD
 *
 * Copyright (c) 1985-2024 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file mged/edmetaball.h
 */

#ifndef EDMETABALL_H
#define EDMETABALL_H

#include "common.h"
#include "vmath.h"
#include "raytrace.h"
#include "mged.h"

extern struct wdb_metaball_pnt *es_metaball_pnt;

#define ECMD_METABALL_SET_THRESHOLD	83	/* overall metaball threshold value */
#define ECMD_METABALL_SET_METHOD	84	/* set the rendering method */
#define ECMD_METABALL_PT_PICK	85	/* pick a metaball control point */
#define ECMD_METABALL_PT_MOV	86	/* move a metaball control point */
#define ECMD_METABALL_PT_FLDSTR	87	/* set a metaball control point field strength */
#define ECMD_METABALL_PT_DEL	88	/* delete a metaball control point */
#define ECMD_METABALL_PT_ADD	89	/* add a metaball control point */
#define ECMD_METABALL_RMET	90	/* set the metaball render method */

#define MENU_METABALL_SET_THRESHOLD	117
#define MENU_METABALL_SET_METHOD	118
#define MENU_METABALL_PT_SET_GOO	119
#define MENU_METABALL_SELECT	120
#define MENU_METABALL_NEXT_PT	121
#define MENU_METABALL_PREV_PT	122
#define MENU_METABALL_MOV_PT	123
#define MENU_METABALL_PT_FLDSTR	124
#define MENU_METABALL_DEL_PT	125
#define MENU_METABALL_ADD_PT	126

#endif  /* EDMETABALL_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */