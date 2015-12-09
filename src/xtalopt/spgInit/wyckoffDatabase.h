/**********************************************************************
  wyckoffDatabase.h - Database that contains the wyckoff letter, multiplicity,
                      and first position of each wyckoff position of every
                      spacegroup. It is stored as a static const vector of
                      vectors of tuples.

  Copyright (C) 2015 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef WYCKOFF_DATABASE_H
#define WYCKOFF_DATABASE_H

#include <xtalopt/spgInit/spgInit.h>

// Outer vector is the space group. So there are 230 of them
// Inner vector depends on the number of wyckoff elements that
// exist in each space group
// This contains the wyckoff letter, multiplicity, and x,y,z coordinates for
// the first wyckoff position of each spacegroup
// This list was obtained by parsing html files at
// http://www.cryst.ehu.es/cgi-bin/cryst/programs/nph-table?from=getwp
// on 12/04/15

// For spacegroups with an origin choice, origin choices are all 2
// For spacegroups with rhombohedral vs. hexagonal, all are hexagonal
static const std::vector<wyckoffPositions> wyckoffPositionsDatabase
{
  { // 0. Not a real space group...

  },

  { // 1
    wyckInfo{'a',1,"x,y,z"}
  },

  { // 2
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0,0.5,0"},
    wyckInfo{'d',1,"0.5,0,0"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0.5,0,0.5"},
    wyckInfo{'g',1,"0,0.5,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"x,y,z"}
  },

  { // 3 - unique axis b
    wyckInfo{'a',1,"0,y,0"},
    wyckInfo{'b',1,"0,y,0.5"},
    wyckInfo{'c',1,"0.5,y,0"},
    wyckInfo{'d',1,"0.5,y,0.5"},
    wyckInfo{'e',2,"x,y,z"}
  },

  { // 4 - unique axis b
    wyckInfo{'a',2,"x,y,z"}
  },

  { // 5 - unique axis b
    wyckInfo{'a',2,"0,y,0"},
    wyckInfo{'b',2,"0,y,0.5"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 6 - unique axis b
    wyckInfo{'a',1,"x,0,z"},
    wyckInfo{'b',1,"x,0.5,z"},
    wyckInfo{'c',2,"x,y,z"}
  },

  { // 7 - unique axis b
    wyckInfo{'a',2,"x,y,z"}
  },

  { // 8 - unique axis b
    wyckInfo{'a',2,"x,0,z"},
    wyckInfo{'b',4,"x,y,z"}
  },

  { // 9 - unique axis b
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 10 - unique axis b
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0.5,0"},
    wyckInfo{'c',1,"0,0,0.5"},
    wyckInfo{'d',1,"0.5,0,0"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0,0.5,0.5"},
    wyckInfo{'g',1,"0.5,0,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"0,y,0"},
    wyckInfo{'j',2,"0.5,y,0"},
    wyckInfo{'k',2,"0,y,0.5"},
    wyckInfo{'l',2,"0.5,y,0.5"},
    wyckInfo{'m',2,"x,0,z"},
    wyckInfo{'n',2,"x,0.5,z"},
    wyckInfo{'o',4,"x,y,z"}
  },

  { // 11 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0.5,0,0.5"},
    wyckInfo{'e',2,"x,0.25,z"},
    wyckInfo{'f',4,"x,y,z"}
  },

  { // 12 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0.5,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',4,"0.25,0.25,0"},
    wyckInfo{'f',4,"0.25,0.25,0.5"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0,y,0.5"},
    wyckInfo{'i',4,"x,0,z"},
    wyckInfo{'j',8,"x,y,z"}
  },

  { // 13 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0.5,0,0"},
    wyckInfo{'e',2,"0,y,0.25"},
    wyckInfo{'f',2,"0.5,y,0.25"},
    wyckInfo{'g',4,"x,y,z"}
  },

  { // 14 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0.5,0,0.5"},
    wyckInfo{'e',4,"x,y,z"}
  },

  { // 15 - unique axis b
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0.5,0"},
    wyckInfo{'c',4,"0.25,0.25,0"},
    wyckInfo{'d',4,"0.25,0.25,0.5"},
    wyckInfo{'e',4,"0,y,0.25"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 16
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0,0"},
    wyckInfo{'c',1,"0,0.5,0"},
    wyckInfo{'d',1,"0,0,0.5"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0.5,0,0.5"},
    wyckInfo{'g',1,"0,0.5,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"x,0,0"},
    wyckInfo{'j',2,"x,0,0.5"},
    wyckInfo{'k',2,"x,0.5,0"},
    wyckInfo{'l',2,"x,0.5,0.5"},
    wyckInfo{'m',2,"0,y,0"},
    wyckInfo{'n',2,"0,y,0.5"},
    wyckInfo{'o',2,"0.5,y,0"},
    wyckInfo{'p',2,"0.5,y,0.5"},
    wyckInfo{'q',2,"0,0,z"},
    wyckInfo{'r',2,"0.5,0,z"},
    wyckInfo{'s',2,"0,0.5,z"},
    wyckInfo{'t',2,"0.5,0.5,z"},
    wyckInfo{'u',4,"x,y,z"}
  },

  { // 17
    wyckInfo{'a',2,"x,0,0"},
    wyckInfo{'b',2,"x,0.5,0"},
    wyckInfo{'c',2,"0,y,0.25"},
    wyckInfo{'d',2,"0.5,y,0.25"},
    wyckInfo{'e',4,"x,y,z"}
  },

  { // 18
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 19
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 20
    wyckInfo{'a',4,"x,0,0"},
    wyckInfo{'b',4,"0,y,0.25"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 21
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0.5,0"},
    wyckInfo{'c',2,"0.5,0,0.5"},
    wyckInfo{'d',2,"0,0,0.5"},
    wyckInfo{'e',4,"x,0,0"},
    wyckInfo{'f',4,"x,0,0.5"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0,y,0.5"},
    wyckInfo{'i',4,"0,0,z"},
    wyckInfo{'j',4,"0,0.5,z"},
    wyckInfo{'k',4,"0.25,0.25,z"},
    wyckInfo{'l',8,"x,y,z"}
  },

  { // 22
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.25,0.25,0.75"},
    wyckInfo{'e',8,"x,0,0"},
    wyckInfo{'f',8,"0,y,0"},
    wyckInfo{'g',8,"0,0,z"},
    wyckInfo{'h',8,"0.25,0.25,z"},
    wyckInfo{'i',8,"0.25,y,0.25"},
    wyckInfo{'j',8,"x,0.25,0.25"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 23
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0,0.5,0"},
    wyckInfo{'e',4,"x,0,0"},
    wyckInfo{'f',4,"x,0,0.5"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0.5,y,0"},
    wyckInfo{'i',4,"0,0,z"},
    wyckInfo{'j',4,"0,0.5,z"},
    wyckInfo{'k',8,"x,y,z"}
  },

  { // 24
    wyckInfo{'a',4,"x,0,0.25"},
    wyckInfo{'b',4,"0.25,y,0"},
    wyckInfo{'c',4,"0,0.25,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 25
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',1,"0,0.5,z"},
    wyckInfo{'c',1,"0.5,0,z"},
    wyckInfo{'d',1,"0.5,0.5,z"},
    wyckInfo{'e',2,"x,0,z"},
    wyckInfo{'f',2,"x,0.5,z"},
    wyckInfo{'g',2,"0,y,z"},
    wyckInfo{'h',2,"0.5,y,z"},
    wyckInfo{'i',4,"x,y,z"}
  },

  { // 26
    wyckInfo{'a',2,"0,y,z"},
    wyckInfo{'b',2,"0.5,y,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 27
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',2,"0.5,0,z"},
    wyckInfo{'d',2,"0.5,0.5,z"},
    wyckInfo{'e',4,"x,y,z"}
  },

  { // 28
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',2,"0.25,y,z"},
    wyckInfo{'d',4,"x,y,z"}
  },

  { // 29
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 30
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 31
    wyckInfo{'a',2,"0,y,z"},
    wyckInfo{'b',4,"x,y,z"}
  },

  { // 32
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 33
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 34
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 35
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"0.25,0.25,z"},
    wyckInfo{'d',4,"x,0,z"},
    wyckInfo{'e',4,"0,y,z"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 36
    wyckInfo{'a',4,"0,y,z"},
    wyckInfo{'b',8,"x,y,z"}
  },

  { // 37
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',4,"0.25,0.25,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 38
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0,z"},
    wyckInfo{'c',4,"x,0,z"},
    wyckInfo{'d',4,"0,y,z"},
    wyckInfo{'e',4,"0.5,y,z"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 39
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0.5,0,z"},
    wyckInfo{'c',4,"x,0.25,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 40
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0.25,y,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 41
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',8,"x,y,z"}
  },

  { // 42
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',8,"0.25,0.25,z"},
    wyckInfo{'c',8,"0,y,z"},
    wyckInfo{'d',8,"x,0,z"},
    wyckInfo{'e',16,"x,y,z"}
  },

  { // 43
    wyckInfo{'a',8,"0,0,z"},
    wyckInfo{'b',16,"x,y,z"}
  },

  { // 44
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"x,0,z"},
    wyckInfo{'d',4,"0,y,z"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 45
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 46
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0.25,y,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 47
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0,0"},
    wyckInfo{'c',1,"0,0,0.5"},
    wyckInfo{'d',1,"0.5,0,0.5"},
    wyckInfo{'e',1,"0,0.5,0"},
    wyckInfo{'f',1,"0.5,0.5,0"},
    wyckInfo{'g',1,"0,0.5,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"x,0,0"},
    wyckInfo{'j',2,"x,0,0.5"},
    wyckInfo{'k',2,"x,0.5,0"},
    wyckInfo{'l',2,"x,0.5,0.5"},
    wyckInfo{'m',2,"0,y,0"},
    wyckInfo{'n',2,"0,y,0.5"},
    wyckInfo{'o',2,"0.5,y,0"},
    wyckInfo{'p',2,"0.5,y,0.5"},
    wyckInfo{'q',2,"0,0,z"},
    wyckInfo{'r',2,"0,0.5,z"},
    wyckInfo{'s',2,"0.5,0,z"},
    wyckInfo{'t',2,"0.5,0.5,z"},
    wyckInfo{'u',4,"0,y,z"},
    wyckInfo{'v',4,"0.5,y,z"},
    wyckInfo{'w',4,"x,0,z"},
    wyckInfo{'x',4,"x,0.5,z"},
    wyckInfo{'y',4,"x,y,0"},
    wyckInfo{'z',4,"x,y,0.5"},
    wyckInfo{'A',8,"x,y,z"}
  },

  { // 48
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',2,"0.75,0.25,0.25"},
    wyckInfo{'c',2,"0.25,0.25,0.75"},
    wyckInfo{'d',2,"0.25,0.75,0.25"},
    wyckInfo{'e',4,"0.5,0.5,0.5"},
    wyckInfo{'f',4,"0,0,0"},
    wyckInfo{'g',4,"x,0.25,0.25"},
    wyckInfo{'h',4,"x,0.25,0.75"},
    wyckInfo{'i',4,"0.25,y,0.25"},
    wyckInfo{'j',4,"0.75,y,0.25"},
    wyckInfo{'k',4,"0.25,0.25,z"},
    wyckInfo{'l',4,"0.25,0.75,z"},
    wyckInfo{'m',8,"x,y,z"}
  },

  { // 49
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0.5,0,0"},
    wyckInfo{'e',2,"0,0,0.25"},
    wyckInfo{'f',2,"0.5,0,0.25"},
    wyckInfo{'g',2,"0,0.5,0.25"},
    wyckInfo{'h',2,"0.5,0.5,0.25"},
    wyckInfo{'i',4,"x,0,0.25"},
    wyckInfo{'j',4,"x,0.5,0.25"},
    wyckInfo{'k',4,"0,y,0.25"},
    wyckInfo{'l',4,"0.5,y,0.25"},
    wyckInfo{'m',4,"0,0,z"},
    wyckInfo{'n',4,"0.5,0.5,z"},
    wyckInfo{'o',4,"0,0.5,z"},
    wyckInfo{'p',4,"0.5,0,z"},
    wyckInfo{'q',4,"x,y,0"},
    wyckInfo{'r',8,"x,y,z"}
  },

  { // 50
    wyckInfo{'a',2,"0.25,0.25,0"},
    wyckInfo{'b',2,"0.75,0.25,0"},
    wyckInfo{'c',2,"0.75,0.25,0.5"},
    wyckInfo{'d',2,"0.25,0.25,0.5"},
    wyckInfo{'e',4,"0,0,0"},
    wyckInfo{'f',4,"0,0,0.5"},
    wyckInfo{'g',4,"x,0.25,0"},
    wyckInfo{'h',4,"x,0.25,0.5"},
    wyckInfo{'i',4,"0.25,y,0"},
    wyckInfo{'j',4,"0.25,y,0.5"},
    wyckInfo{'k',4,"0.25,0.25,z"},
    wyckInfo{'l',4,"0.25,0.75,z"},
    wyckInfo{'m',8,"x,y,z"}
  },

  { // 51
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0.5,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',2,"0.25,0,z"},
    wyckInfo{'f',2,"0.25,0.5,z"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0,y,0.5"},
    wyckInfo{'i',4,"x,0,z"},
    wyckInfo{'j',4,"x,0.5,z"},
    wyckInfo{'k',4,"0.25,y,z"},
    wyckInfo{'l',8,"x,y,z"}
  },

  { // 52
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',4,"0.25,0,z"},
    wyckInfo{'d',4,"x,0.25,0.25"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 53
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0.5,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0"},
    wyckInfo{'e',4,"x,0,0"},
    wyckInfo{'f',4,"x,0.5,0"},
    wyckInfo{'g',4,"0.25,y,0.25"},
    wyckInfo{'h',4,"0,y,z"},
    wyckInfo{'i',8,"x,y,z"}
  },

  { // 54
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0.5,0"},
    wyckInfo{'c',4,"0,y,0.25"},
    wyckInfo{'d',4,"0.25,0,z"},
    wyckInfo{'e',4,"0.25,0.5,z"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 55
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',4,"x,y,0"},
    wyckInfo{'h',4,"x,y,0.5"},
    wyckInfo{'i',8,"x,y,z"}
  },

  { // 56
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',4,"0.25,0.25,z"},
    wyckInfo{'d',4,"0.25,0.75,z"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 57
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0,0"},
    wyckInfo{'c',4,"x,0.25,0"},
    wyckInfo{'d',4,"x,y,0.25"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 58
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',4,"x,y,0"},
    wyckInfo{'h',8,"x,y,z"}
  },

  { // 59
    wyckInfo{'a',2,"0.25,0.25,z"},
    wyckInfo{'b',2,"0.25,0.75,z"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0,0,0.5"},
    wyckInfo{'e',4,"0.25,y,z"},
    wyckInfo{'f',4,"x,0.25,z"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 60
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0.5,0"},
    wyckInfo{'c',4,"0,y,0.25"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 61
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 62
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',4,"x,0.25,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 63
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0.5,0"},
    wyckInfo{'c',4,"0,y,0.25"},
    wyckInfo{'d',8,"0.25,0.25,0"},
    wyckInfo{'e',8,"x,0,0"},
    wyckInfo{'f',8,"0,y,z"},
    wyckInfo{'g',8,"x,y,0.25"},
    wyckInfo{'h',16,"x,y,z"}
  },

  { // 64
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0,0"},
    wyckInfo{'c',8,"0.25,0.25,0"},
    wyckInfo{'d',8,"x,0,0"},
    wyckInfo{'e',8,"0.25,y,0.25"},
    wyckInfo{'f',8,"0,y,z"},
    wyckInfo{'g',16,"x,y,z"}
  },

  { // 65
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0.5,0,0.5"},
    wyckInfo{'d',2,"0,0,0.5"},
    wyckInfo{'e',4,"0.25,0.25,0"},
    wyckInfo{'f',4,"0.25,0.25,0.5"},
    wyckInfo{'g',4,"x,0,0"},
    wyckInfo{'h',4,"x,0,0.5"},
    wyckInfo{'i',4,"0,y,0"},
    wyckInfo{'j',4,"0,y,0.5"},
    wyckInfo{'k',4,"0,0,z"},
    wyckInfo{'l',4,"0,0.5,z"},
    wyckInfo{'m',8,"0.25,0.25,z"},
    wyckInfo{'n',8,"0,y,z"},
    wyckInfo{'o',8,"x,0,z"},
    wyckInfo{'p',8,"x,y,0"},
    wyckInfo{'q',8,"x,y,0.5"},
    wyckInfo{'r',16,"x,y,z"}
  },

  { // 66
    wyckInfo{'a',4,"0,0,0.25"},
    wyckInfo{'b',4,"0,0.5,0.25"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0,0.5,0"},
    wyckInfo{'e',4,"0.25,0.25,0"},
    wyckInfo{'f',4,"0.25,0.75,0"},
    wyckInfo{'g',8,"x,0,0.25"},
    wyckInfo{'h',8,"0,y,0.25"},
    wyckInfo{'i',8,"0,0,z"},
    wyckInfo{'j',8,"0,0.5,z"},
    wyckInfo{'k',8,"0.25,0.25,z"},
    wyckInfo{'l',8,"x,y,0"},
    wyckInfo{'m',16,"x,y,z"}
  },

  { // 67
    wyckInfo{'a',4,"0.25,0,0"},
    wyckInfo{'b',4,"0.25,0,0.5"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0,0,0.5"},
    wyckInfo{'e',4,"0.25,0.25,0"},
    wyckInfo{'f',4,"0.25,0.25,0.5"},
    wyckInfo{'g',4,"0,0.25,z"},
    wyckInfo{'h',8,"x,0,0"},
    wyckInfo{'i',8,"x,0,0.5"},
    wyckInfo{'j',8,"0.25,y,0"},
    wyckInfo{'k',8,"0.25,y,0.5"},
    wyckInfo{'l',8,"0.25,0,z"},
    wyckInfo{'m',8,"0,y,z"},
    wyckInfo{'n',8,"x,0.25,z"},
    wyckInfo{'o',16,"x,y,z"}
  },

  { // 68
    wyckInfo{'a',4,"0,0.25,0.25"},
    wyckInfo{'b',4,"0,0.25,0.75"},
    wyckInfo{'c',8,"0.25,0.75,0"},
    wyckInfo{'d',8,"0,0,0"},
    wyckInfo{'e',8,"x,0.25,0.25"},
    wyckInfo{'f',8,"0,y,0.25"},
    wyckInfo{'g',8,"0,0.25,z"},
    wyckInfo{'h',8,"0.25,0,z"},
    wyckInfo{'i',16,"x,y,z"}
  },

  { // 69
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',8,"0,0.25,0.25"},
    wyckInfo{'d',8,"0.25,0,0.25"},
    wyckInfo{'e',8,"0.25,0.25,0"},
    wyckInfo{'f',8,"0.25,0.25,0.25"},
    wyckInfo{'g',8,"x,0,0"},
    wyckInfo{'h',8,"0,y,0"},
    wyckInfo{'i',8,"0,0,z"},
    wyckInfo{'j',16,"0.25,0.25,z"},
    wyckInfo{'k',16,"0.25,y,0.25"},
    wyckInfo{'l',16,"x,0.25,0.25"},
    wyckInfo{'m',16,"0,y,z"},
    wyckInfo{'n',16,"x,0,z"},
    wyckInfo{'o',16,"x,y,0"},
    wyckInfo{'p',32,"x,y,z"}
  },

  { // 70
    wyckInfo{'a',8,"0.125,0.125,0.125"},
    wyckInfo{'b',8,"0.125,0.125,0.625"},
    wyckInfo{'c',16,"0,0,0"},
    wyckInfo{'d',16,"0.5,0.5,0.5"},
    wyckInfo{'e',16,"x,0.125,0.125"},
    wyckInfo{'f',16,"0.125,y,0.125"},
    wyckInfo{'g',16,"0.125,0.125,z"},
    wyckInfo{'h',32,"x,y,z"}
  },

  { // 71
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0.5,0.5"},
    wyckInfo{'c',2,"0.5,0.5,0"},
    wyckInfo{'d',2,"0.5,0,0.5"},
    wyckInfo{'e',4,"x,0,0"},
    wyckInfo{'f',4,"x,0.5,0"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0,y,0.5"},
    wyckInfo{'i',4,"0,0,z"},
    wyckInfo{'j',4,"0.5,0,z"},
    wyckInfo{'k',8,"0.25,0.25,0.25"},
    wyckInfo{'l',8,"0,y,z"},
    wyckInfo{'m',8,"x,0,z"},
    wyckInfo{'n',8,"x,y,0"},
    wyckInfo{'o',16,"x,y,z"}
  },

  { // 72
    wyckInfo{'a',4,"0,0,0.25"},
    wyckInfo{'b',4,"0.5,0,0.25"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0.5,0,0"},
    wyckInfo{'e',8,"0.25,0.25,0.25"},
    wyckInfo{'f',8,"x,0,0.25"},
    wyckInfo{'g',8,"0,y,0.25"},
    wyckInfo{'h',8,"0,0,z"},
    wyckInfo{'i',8,"0,0.5,z"},
    wyckInfo{'j',8,"x,y,0"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 73
    wyckInfo{'a',8,"0,0,0"},
    wyckInfo{'b',8,"0.25,0.25,0.25"},
    wyckInfo{'c',8,"x,0,0.25"},
    wyckInfo{'d',8,"0.25,y,0"},
    wyckInfo{'e',8,"0,0.25,z"},
    wyckInfo{'f',16,"x,y,z"}
  },

  { // 74
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.25,0.25,0.75"},
    wyckInfo{'e',4,"0,0.25,z"},
    wyckInfo{'f',8,"x,0,0"},
    wyckInfo{'g',8,"0.25,y,0.25"},
    wyckInfo{'h',8,"0,y,z"},
    wyckInfo{'i',8,"x,0.25,z"},
    wyckInfo{'j',16,"x,y,z"}
  },

  { // 75
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',1,"0.5,0.5,z"},
    wyckInfo{'c',2,"0,0.5,z"},
    wyckInfo{'d',4,"x,y,z"}
  },

  { // 76
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 77
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0.5,z"},
    wyckInfo{'c',2,"0,0.5,z"},
    wyckInfo{'d',4,"x,y,z"}
  },

  { // 78
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 79
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 80
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',8,"x,y,z"}
  },

  { // 81
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.5,0.5,0"},
    wyckInfo{'d',1,"0.5,0.5,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',2,"0.5,0.5,z"},
    wyckInfo{'g',2,"0,0.5,z"},
    wyckInfo{'h',4,"x,y,z"}
  },

  { // 82
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0.25"},
    wyckInfo{'d',2,"0,0.5,0.75"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 83
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.5,0.5,0"},
    wyckInfo{'d',1,"0.5,0.5,0.5"},
    wyckInfo{'e',2,"0,0.5,0"},
    wyckInfo{'f',2,"0,0.5,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,y,0"},
    wyckInfo{'k',4,"x,y,0.5"},
    wyckInfo{'l',8,"x,y,z"}
  },

  { // 84
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',2,"0,0,0.25"},
    wyckInfo{'f',2,"0.5,0.5,0.25"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,y,0"},
    wyckInfo{'k',8,"x,y,z"}
  },

  { // 85
    wyckInfo{'a',2,"0.25,0.75,0"},
    wyckInfo{'b',2,"0.25,0.75,0.5"},
    wyckInfo{'c',2,"0.25,0.25,z"},
    wyckInfo{'d',4,"0,0,0"},
    wyckInfo{'e',4,"0,0,0.5"},
    wyckInfo{'f',4,"0.25,0.75,z"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 86
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',2,"0.25,0.25,0.75"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0,0,0.5"},
    wyckInfo{'e',4,"0.75,0.25,z"},
    wyckInfo{'f',4,"0.25,0.25,z"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 87
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',8,"0.25,0.25,0.25"},
    wyckInfo{'g',8,"0,0.5,z"},
    wyckInfo{'h',8,"x,y,0"},
    wyckInfo{'i',16,"x,y,z"}
  },

  { // 88
    wyckInfo{'a',4,"0,0.25,0.125"},
    wyckInfo{'b',4,"0,0.25,0.625"},
    wyckInfo{'c',8,"0,0,0"},
    wyckInfo{'d',8,"0,0,0.5"},
    wyckInfo{'e',8,"0,0.25,z"},
    wyckInfo{'f',16,"x,y,z"}
  },

  { // 89
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.5,0.5,0"},
    wyckInfo{'d',1,"0.5,0.5,0.5"},
    wyckInfo{'e',2,"0.5,0,0"},
    wyckInfo{'f',2,"0.5,0,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,x,0"},
    wyckInfo{'k',4,"x,x,0.5"},
    wyckInfo{'l',4,"x,0,0"},
    wyckInfo{'m',4,"x,0.5,0.5"},
    wyckInfo{'n',4,"x,0,0.5"},
    wyckInfo{'o',4,"x,0.5,0"},
    wyckInfo{'p',8,"x,y,z"}
  },

  { // 90
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,z"},
    wyckInfo{'d',4,"0,0,z"},
    wyckInfo{'e',4,"x,x,0"},
    wyckInfo{'f',4,"x,x,0.5"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 91
    wyckInfo{'a',4,"0,y,0"},
    wyckInfo{'b',4,"0.5,y,0"},
    wyckInfo{'c',4,"x,x,0.375"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 92
    wyckInfo{'a',4,"x,x,0"},
    wyckInfo{'b',8,"x,y,z"}
  },

  { // 93
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',2,"0,0,0.25"},
    wyckInfo{'f',2,"0.5,0.5,0.25"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,0,0"},
    wyckInfo{'k',4,"x,0.5,0.5"},
    wyckInfo{'l',4,"x,0,0.5"},
    wyckInfo{'m',4,"x,0.5,0"},
    wyckInfo{'n',4,"x,x,0.25"},
    wyckInfo{'o',4,"x,x,0.75"},
    wyckInfo{'p',8,"x,y,z"}
  },

  { // 94
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0,z"},
    wyckInfo{'d',4,"0,0.5,z"},
    wyckInfo{'e',4,"x,x,0"},
    wyckInfo{'f',4,"x,x,0.5"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 95
    wyckInfo{'a',4,"0,y,0"},
    wyckInfo{'b',4,"0.5,y,0"},
    wyckInfo{'c',4,"x,x,0.625"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 96
    wyckInfo{'a',4,"x,x,0"},
    wyckInfo{'b',8,"x,y,z"}
  },

  { // 97
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',8,"0,0.5,z"},
    wyckInfo{'g',8,"x,x,0"},
    wyckInfo{'h',8,"x,0,0"},
    wyckInfo{'i',8,"x,0,0.5"},
    wyckInfo{'j',8,"x,x+0.5,0.25"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 98
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',8,"0,0,z"},
    wyckInfo{'d',8,"x,x,0"},
    wyckInfo{'e',8,"-x,x,0"},
    wyckInfo{'f',8,"x,0.25,0.125"},
    wyckInfo{'g',16,"x,y,z"}
  },

  { // 99
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',1,"0.5,0.5,z"},
    wyckInfo{'c',2,"0.5,0,z"},
    wyckInfo{'d',4,"x,x,z"},
    wyckInfo{'e',4,"x,0,z"},
    wyckInfo{'f',4,"x,0.5,z"},
    wyckInfo{'g',8,"x,y,z"}
  },

  { // 100
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0,z"},
    wyckInfo{'c',4,"x,x+0.5,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 101
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0.5,z"},
    wyckInfo{'c',4,"0,0.5,z"},
    wyckInfo{'d',4,"x,x,z"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 102
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',4,"x,x,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 103
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0.5,z"},
    wyckInfo{'c',4,"0,0.5,z"},
    wyckInfo{'d',8,"x,y,z"}
  },

  { // 104
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 105
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.5,0.5,z"},
    wyckInfo{'c',2,"0,0.5,z"},
    wyckInfo{'d',4,"x,0,z"},
    wyckInfo{'e',4,"x,0.5,z"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 106
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',8,"x,y,z"}
  },

  { // 107
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0,0.5,z"},
    wyckInfo{'c',8,"x,x,z"},
    wyckInfo{'d',8,"x,0,z"},
    wyckInfo{'e',16,"x,y,z"}
  },

  { // 108
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',4,"0.5,0,z"},
    wyckInfo{'c',8,"x,x+0.5,z"},
    wyckInfo{'d',16,"x,y,z"}
  },

  { // 109
    wyckInfo{'a',4,"0,0,z"},
    wyckInfo{'b',8,"0,y,z"},
    wyckInfo{'c',16,"x,y,z"}
  },

  { // 110
    wyckInfo{'a',8,"0,0,z"},
    wyckInfo{'b',16,"x,y,z"}
  },

  { // 111
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',1,"0,0,0.5"},
    wyckInfo{'d',1,"0.5,0.5,0"},
    wyckInfo{'e',2,"0.5,0,0"},
    wyckInfo{'f',2,"0.5,0,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.5,0.5,z"},
    wyckInfo{'i',4,"x,0,0"},
    wyckInfo{'j',4,"x,0.5,0.5"},
    wyckInfo{'k',4,"x,0,0.5"},
    wyckInfo{'l',4,"x,0.5,0"},
    wyckInfo{'m',4,"0,0.5,z"},
    wyckInfo{'n',4,"x,x,z"},
    wyckInfo{'o',8,"x,y,z"}
  },

  { // 112
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0.5,0,0.25"},
    wyckInfo{'c',2,"0.5,0.5,0.25"},
    wyckInfo{'d',2,"0,0.5,0.25"},
    wyckInfo{'e',2,"0,0,0"},
    wyckInfo{'f',2,"0.5,0.5,0"},
    wyckInfo{'g',4,"x,0,0.25"},
    wyckInfo{'h',4,"0.5,y,0.25"},
    wyckInfo{'i',4,"x,0.5,0.25"},
    wyckInfo{'j',4,"0,y,0.25"},
    wyckInfo{'k',4,"0,0,z"},
    wyckInfo{'l',4,"0.5,0.5,z"},
    wyckInfo{'m',4,"0,0.5,z"},
    wyckInfo{'n',8,"x,y,z"}
  },

  { // 113
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,z"},
    wyckInfo{'d',4,"0,0,z"},
    wyckInfo{'e',4,"x,x+0.5,z"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 114
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0,z"},
    wyckInfo{'d',4,"0,0.5,z"},
    wyckInfo{'e',8,"x,y,z"}
  },

  { // 115
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0"},
    wyckInfo{'c',1,"0.5,0.5,0.5"},
    wyckInfo{'d',1,"0,0,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',2,"0.5,0.5,z"},
    wyckInfo{'g',2,"0,0.5,z"},
    wyckInfo{'h',4,"x,x,0"},
    wyckInfo{'i',4,"x,x,0.5"},
    wyckInfo{'j',4,"x,0,z"},
    wyckInfo{'k',4,"x,0.5,z"},
    wyckInfo{'l',8,"x,y,z"}
  },

  { // 116
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0.5,0.5,0.25"},
    wyckInfo{'c',2,"0,0,0"},
    wyckInfo{'d',2,"0.5,0.5,0"},
    wyckInfo{'e',4,"x,x,0.25"},
    wyckInfo{'f',4,"x,x,0.75"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',8,"x,y,z"}
  },

  { // 117
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',4,"x,x+0.5,0"},
    wyckInfo{'h',4,"x,x+0.5,0.5"},
    wyckInfo{'i',8,"x,y,z"}
  },

  { // 118
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0.25"},
    wyckInfo{'d',2,"0,0.5,0.75"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"x,-x+0.5,0.25"},
    wyckInfo{'g',4,"x,x+0.5,0.25"},
    wyckInfo{'h',4,"0,0.5,z"},
    wyckInfo{'i',8,"x,y,z"}
  },

  { // 119
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0.25"},
    wyckInfo{'d',2,"0,0.5,0.75"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',8,"x,x,0"},
    wyckInfo{'h',8,"x,x+0.5,0.25"},
    wyckInfo{'i',8,"x,0,z"},
    wyckInfo{'j',16,"x,y,z"}
  },

  { // 120
    wyckInfo{'a',4,"0,0,0.25"},
    wyckInfo{'b',4,"0,0,0"},
    wyckInfo{'c',4,"0,0.5,0.25"},
    wyckInfo{'d',4,"0,0.5,0"},
    wyckInfo{'e',8,"x,x,0.25"},
    wyckInfo{'f',8,"0,0,z"},
    wyckInfo{'g',8,"0,0.5,z"},
    wyckInfo{'h',8,"x,x+0.5,0"},
    wyckInfo{'i',16,"x,y,z"}
  },

  { // 121
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',8,"x,0,0"},
    wyckInfo{'g',8,"x,0,0.5"},
    wyckInfo{'h',8,"0,0.5,z"},
    wyckInfo{'i',8,"x,x,z"},
    wyckInfo{'j',16,"x,y,z"}
  },

  { // 122
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.5"},
    wyckInfo{'c',8,"0,0,z"},
    wyckInfo{'d',8,"x,0.25,0.125"},
    wyckInfo{'e',16,"x,y,z"}
  },

  { // 123
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.5,0.5,0"},
    wyckInfo{'d',1,"0.5,0.5,0.5"},
    wyckInfo{'e',2,"0,0.5,0.5"},
    wyckInfo{'f',2,"0,0.5,0"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,x,0"},
    wyckInfo{'k',4,"x,x,0.5"},
    wyckInfo{'l',4,"x,0,0"},
    wyckInfo{'m',4,"x,0,0.5"},
    wyckInfo{'n',4,"x,0.5,0"},
    wyckInfo{'o',4,"x,0.5,0.5"},
    wyckInfo{'p',8,"x,y,0"},
    wyckInfo{'q',8,"x,y,0.5"},
    wyckInfo{'r',8,"x,x,z"},
    wyckInfo{'s',8,"x,0,z"},
    wyckInfo{'t',8,"x,0.5,z"},
    wyckInfo{'u',16,"x,y,z"}
  },

  { // 124
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',2,"0.5,0.5,0.25"},
    wyckInfo{'d',2,"0.5,0.5,0"},
    wyckInfo{'e',4,"0,0.5,0"},
    wyckInfo{'f',4,"0,0.5,0.25"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',8,"0,0.5,z"},
    wyckInfo{'j',8,"x,x,0.25"},
    wyckInfo{'k',8,"x,0,0.25"},
    wyckInfo{'l',8,"x,0.5,0.25"},
    wyckInfo{'m',8,"x,y,0"},
    wyckInfo{'n',16,"x,y,z"}
  },

  { // 125
    wyckInfo{'a',2,"0.25,0.25,0"},
    wyckInfo{'b',2,"0.25,0.25,0.5"},
    wyckInfo{'c',2,"0.75,0.25,0"},
    wyckInfo{'d',2,"0.75,0.25,0.5"},
    wyckInfo{'e',4,"0,0,0"},
    wyckInfo{'f',4,"0,0,0.5"},
    wyckInfo{'g',4,"0.25,0.25,z"},
    wyckInfo{'h',4,"0.75,0.25,z"},
    wyckInfo{'i',8,"x,x,0"},
    wyckInfo{'j',8,"x,x,0.5"},
    wyckInfo{'k',8,"x,0.25,0"},
    wyckInfo{'l',8,"x,0.25,0.5"},
    wyckInfo{'m',8,"x,-x,z"},
    wyckInfo{'n',16,"x,y,z"}
  },

  { // 126
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',2,"0.25,0.25,0.75"},
    wyckInfo{'c',4,"0.25,0.75,0.75"},
    wyckInfo{'d',4,"0.25,0.75,0"},
    wyckInfo{'e',4,"0.25,0.25,z"},
    wyckInfo{'f',8,"0,0,0"},
    wyckInfo{'g',8,"0.25,0.75,z"},
    wyckInfo{'h',8,"x,x,0.25"},
    wyckInfo{'i',8,"x,0.25,0.25"},
    wyckInfo{'j',8,"x,0.75,0.25"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 127
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',2,"0,0.5,0.5"},
    wyckInfo{'d',2,"0,0.5,0"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0,0.5,z"},
    wyckInfo{'g',4,"x,x+0.5,0"},
    wyckInfo{'h',4,"x,x+0.5,0.5"},
    wyckInfo{'i',8,"x,y,0"},
    wyckInfo{'j',8,"x,y,0.5"},
    wyckInfo{'k',8,"x,x+0.5,z"},
    wyckInfo{'l',16,"x,y,z"}
  },

  { // 128
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',8,"0,0.5,z"},
    wyckInfo{'g',8,"x,x+0.5,0.25"},
    wyckInfo{'h',8,"x,y,0"},
    wyckInfo{'i',16,"x,y,z"}
  },

  { // 129
    wyckInfo{'a',2,"0.75,0.25,0"},
    wyckInfo{'b',2,"0.75,0.25,0.5"},
    wyckInfo{'c',2,"0.25,0.25,z"},
    wyckInfo{'d',4,"0,0,0"},
    wyckInfo{'e',4,"0,0,0.5"},
    wyckInfo{'f',4,"0.75,0.25,z"},
    wyckInfo{'g',8,"x,-x,0"},
    wyckInfo{'h',8,"x,-x,0.5"},
    wyckInfo{'i',8,"0.25,y,z"},
    wyckInfo{'j',8,"x,x,z"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 130
    wyckInfo{'a',4,"0.75,0.25,0.25"},
    wyckInfo{'b',4,"0.75,0.25,0"},
    wyckInfo{'c',4,"0.25,0.25,z"},
    wyckInfo{'d',8,"0,0,0"},
    wyckInfo{'e',8,"0.75,0.25,z"},
    wyckInfo{'f',8,"x,-x,0.25"},
    wyckInfo{'g',16,"x,y,z"}
  },

  { // 131
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',2,"0,0,0.25"},
    wyckInfo{'f',2,"0.5,0.5,0.25"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',4,"0,0.5,z"},
    wyckInfo{'j',4,"x,0,0"},
    wyckInfo{'k',4,"x,0.5,0.5"},
    wyckInfo{'l',4,"x,0,0.5"},
    wyckInfo{'m',4,"x,0.5,0"},
    wyckInfo{'n',8,"x,x,0.25"},
    wyckInfo{'o',8,"0,y,z"},
    wyckInfo{'p',8,"0.5,y,z"},
    wyckInfo{'q',8,"x,y,0"},
    wyckInfo{'r',16,"x,y,z"}
  },

  { // 132
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.25"},
    wyckInfo{'c',2,"0.5,0.5,0"},
    wyckInfo{'d',2,"0.5,0.5,0.25"},
    wyckInfo{'e',4,"0,0.5,0.25"},
    wyckInfo{'f',4,"0,0.5,0"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.5,0.5,z"},
    wyckInfo{'i',4,"x,x,0"},
    wyckInfo{'j',4,"x,x,0.5"},
    wyckInfo{'k',8,"0,0.5,z"},
    wyckInfo{'l',8,"x,0,0.25"},
    wyckInfo{'m',8,"x,0.5,0.25"},
    wyckInfo{'n',8,"x,y,0"},
    wyckInfo{'o',8,"x,x,z"},
    wyckInfo{'p',16,"x,y,z"}
  },

  { // 133
    wyckInfo{'a',4,"0.25,0.25,0"},
    wyckInfo{'b',4,"0.75,0.25,0"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.75,0.25,0.75"},
    wyckInfo{'e',8,"0,0,0"},
    wyckInfo{'f',8,"0.25,0.25,z"},
    wyckInfo{'g',8,"0.75,0.25,z"},
    wyckInfo{'h',8,"x,0.25,0"},
    wyckInfo{'i',8,"x,0.25,0.5"},
    wyckInfo{'j',8,"x,x,0.25"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 134
    wyckInfo{'a',2,"0.25,0.75,0.25"},
    wyckInfo{'b',2,"0.75,0.25,0.25"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.25,0.25,0"},
    wyckInfo{'e',4,"0,0,0.5"},
    wyckInfo{'f',4,"0,0,0"},
    wyckInfo{'g',4,"0.75,0.25,z"},
    wyckInfo{'h',8,"0.25,0.25,z"},
    wyckInfo{'i',8,"x,0.25,0.75"},
    wyckInfo{'j',8,"x,0.25,0.25"},
    wyckInfo{'k',8,"x,x,0"},
    wyckInfo{'l',8,"x,x,0.5"},
    wyckInfo{'m',8,"x,-x,z"},
    wyckInfo{'n',16,"x,y,z"}
  },

  { // 135
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0,0.25"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',8,"0,0,z"},
    wyckInfo{'f',8,"0,0.5,z"},
    wyckInfo{'g',8,"x,x+0.5,0.25"},
    wyckInfo{'h',8,"x,y,0"},
    wyckInfo{'i',16,"x,y,z"}
  },

  { // 136
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"x,x,0"},
    wyckInfo{'g',4,"x,-x,0"},
    wyckInfo{'h',8,"0,0.5,z"},
    wyckInfo{'i',8,"x,y,0"},
    wyckInfo{'j',8,"x,x,z"},
    wyckInfo{'k',16,"x,y,z"}
  },

  { // 137
    wyckInfo{'a',2,"0.75,0.25,0.75"},
    wyckInfo{'b',2,"0.75,0.25,0.25"},
    wyckInfo{'c',4,"0.75,0.25,z"},
    wyckInfo{'d',4,"0.25,0.25,z"},
    wyckInfo{'e',8,"0,0,0"},
    wyckInfo{'f',8,"x,-x,0.25"},
    wyckInfo{'g',8,"0.25,y,z"},
    wyckInfo{'h',16,"x,y,z"}
  },

  { // 138
    wyckInfo{'a',4,"0.75,0.25,0"},
    wyckInfo{'b',4,"0.75,0.25,0.75"},
    wyckInfo{'c',4,"0,0,0.5"},
    wyckInfo{'d',4,"0,0,0"},
    wyckInfo{'e',4,"0.25,0.25,z"},
    wyckInfo{'f',8,"0.75,0.25,z"},
    wyckInfo{'g',8,"x,-x,0.5"},
    wyckInfo{'h',8,"x,-x,0"},
    wyckInfo{'i',8,"x,x,z"},
    wyckInfo{'j',16,"x,y,z"}
  },

  { // 139
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.5"},
    wyckInfo{'c',4,"0,0.5,0"},
    wyckInfo{'d',4,"0,0.5,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',8,"0.25,0.25,0.25"},
    wyckInfo{'g',8,"0,0.5,z"},
    wyckInfo{'h',8,"x,x,0"},
    wyckInfo{'i',8,"x,0,0"},
    wyckInfo{'j',8,"x,0.5,0"},
    wyckInfo{'k',16,"x,x+0.5,0.25"},
    wyckInfo{'l',16,"x,y,0"},
    wyckInfo{'m',16,"x,x,z"},
    wyckInfo{'n',16,"0,y,z"},
    wyckInfo{'o',32,"x,y,z"}
  },

  { // 140
    wyckInfo{'a',4,"0,0,0.25"},
    wyckInfo{'b',4,"0,0.5,0.25"},
    wyckInfo{'c',4,"0,0,0"},
    wyckInfo{'d',4,"0,0.5,0"},
    wyckInfo{'e',8,"0.25,0.25,0.25"},
    wyckInfo{'f',8,"0,0,z"},
    wyckInfo{'g',8,"0,0.5,z"},
    wyckInfo{'h',8,"x,x+0.5,0"},
    wyckInfo{'i',16,"x,x,0.25"},
    wyckInfo{'j',16,"x,0,0.25"},
    wyckInfo{'k',16,"x,y,0"},
    wyckInfo{'l',16,"x,x+0.5,z"},
    wyckInfo{'m',32,"x,y,z"}
  },

  { // 141
    wyckInfo{'a',4,"0,0.75,0.125"},
    wyckInfo{'b',4,"0,0.25,0.375"},
    wyckInfo{'c',8,"0,0,0"},
    wyckInfo{'d',8,"0,0,0.5"},
    wyckInfo{'e',8,"0,0.25,z"},
    wyckInfo{'f',16,"x,0,0"},
    wyckInfo{'g',16,"x,x+0.25,0.875"},
    wyckInfo{'h',16,"0,y,z"},
    wyckInfo{'i',32,"x,y,z"}
  },

  { // 142
    wyckInfo{'a',8,"0,0.25,0.375"},
    wyckInfo{'b',8,"0,0.25,0.125"},
    wyckInfo{'c',16,"0,0,0"},
    wyckInfo{'d',16,"0,0.25,z"},
    wyckInfo{'e',16,"x,0,0.25"},
    wyckInfo{'f',16,"x,x+0.25,0.125"},
    wyckInfo{'g',32,"x,y,z"}
  },

  { // 143
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',1,"0.333333,0.666667,z"},
    wyckInfo{'c',1,"0.666667,0.333333,z"},
    wyckInfo{'d',3,"x,y,z"}
  },

  { // 144
    wyckInfo{'a',3,"x,y,z"}
  },

  { // 145
    wyckInfo{'a',3,"x,y,z"}
  },

  { // 146
    wyckInfo{'a',3,"0,0,z"},
    wyckInfo{'b',9,"x,y,z"}
  },

  { // 147
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0,0,z"},
    wyckInfo{'d',2,"0.333333,0.666667,z"},
    wyckInfo{'e',3,"0.5,0,0"},
    wyckInfo{'f',3,"0.5,0,0.5"},
    wyckInfo{'g',6,"x,y,z"}
  },

  { // 148
    wyckInfo{'a',3,"0,0,0"},
    wyckInfo{'b',3,"0,0,0.5"},
    wyckInfo{'c',6,"0,0,z"},
    wyckInfo{'d',9,"0.5,0,0.5"},
    wyckInfo{'e',9,"0.5,0,0"},
    wyckInfo{'f',18,"x,y,z"}
  },

  { // 149
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.333333,0.666667,0"},
    wyckInfo{'d',1,"0.333333,0.666667,0.5"},
    wyckInfo{'e',1,"0.666667,0.333333,0"},
    wyckInfo{'f',1,"0.666667,0.333333,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.333333,0.666667,z"},
    wyckInfo{'i',2,"0.666667,0.333333,z"},
    wyckInfo{'j',3,"x,-x,0"},
    wyckInfo{'k',3,"x,-x,0.5"},
    wyckInfo{'l',6,"x,y,z"}
  },

  { // 150
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0,0,z"},
    wyckInfo{'d',2,"0.333333,0.666667,z"},
    wyckInfo{'e',3,"x,0,0"},
    wyckInfo{'f',3,"x,0,0.5"},
    wyckInfo{'g',6,"x,y,z"}
  },

  { // 151
    wyckInfo{'a',3,"x,-x,0.333333"},
    wyckInfo{'b',3,"x,-x,0.833333"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 152
    wyckInfo{'a',3,"x,0,0.333333"},
    wyckInfo{'b',3,"x,0,0.833333"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 153
    wyckInfo{'a',3,"x,-x,0.666667"},
    wyckInfo{'b',3,"x,-x,0.166667"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 154
    wyckInfo{'a',3,"x,0,0.666667"},
    wyckInfo{'b',3,"x,0,0.166667"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 155
    wyckInfo{'a',3,"0,0,0"},
    wyckInfo{'b',3,"0,0,0.5"},
    wyckInfo{'c',6,"0,0,z"},
    wyckInfo{'d',9,"x,0,0"},
    wyckInfo{'e',9,"x,0,0.5"},
    wyckInfo{'f',18,"x,y,z"}
  },

  { // 156
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',1,"0.333333,0.666667,z"},
    wyckInfo{'c',1,"0.666667,0.333333,z"},
    wyckInfo{'d',3,"x,-x,z"},
    wyckInfo{'e',6,"x,y,z"}
  },

  { // 157
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',3,"x,0,z"},
    wyckInfo{'d',6,"x,y,z"}
  },

  { // 158
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',2,"0.666667,0.333333,z"},
    wyckInfo{'d',6,"x,y,z"}
  },

  { // 159
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 160
    wyckInfo{'a',3,"0,0,z"},
    wyckInfo{'b',9,"x,-x,z"},
    wyckInfo{'c',18,"x,y,z"}
  },

  { // 161
    wyckInfo{'a',6,"0,0,z"},
    wyckInfo{'b',18,"x,y,z"}
  },

  { // 162
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',3,"0.5,0,0"},
    wyckInfo{'g',3,"0.5,0,0.5"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',6,"x,-x,0"},
    wyckInfo{'j',6,"x,-x,0.5"},
    wyckInfo{'k',6,"x,0,z"},
    wyckInfo{'l',12,"x,y,z"}
  },

  { // 163
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',2,"0.333333,0.666667,0.25"},
    wyckInfo{'d',2,"0.666667,0.333333,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0.333333,0.666667,z"},
    wyckInfo{'g',6,"0.5,0,0"},
    wyckInfo{'h',6,"x,-x,0.25"},
    wyckInfo{'i',12,"x,y,z"}
  },

  { // 164
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0,0,z"},
    wyckInfo{'d',2,"0.333333,0.666667,z"},
    wyckInfo{'e',3,"0.5,0,0"},
    wyckInfo{'f',3,"0.5,0,0.5"},
    wyckInfo{'g',6,"x,0,0"},
    wyckInfo{'h',6,"x,0,0.5"},
    wyckInfo{'i',6,"x,-x,z"},
    wyckInfo{'j',12,"x,y,z"}
  },

  { // 165
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',4,"0,0,z"},
    wyckInfo{'d',4,"0.333333,0.666667,z"},
    wyckInfo{'e',6,"0.5,0,0"},
    wyckInfo{'f',6,"x,0,0.25"},
    wyckInfo{'g',12,"x,y,z"}
  },

  { // 166
    wyckInfo{'a',3,"0,0,0"},
    wyckInfo{'b',3,"0,0,0.5"},
    wyckInfo{'c',6,"0,0,z"},
    wyckInfo{'d',9,"0.5,0,0.5"},
    wyckInfo{'e',9,"0.5,0,0"},
    wyckInfo{'f',18,"x,0,0"},
    wyckInfo{'g',18,"x,0,0.5"},
    wyckInfo{'h',18,"x,-x,z"},
    wyckInfo{'i',36,"x,y,z"}
  },

  { // 167
    wyckInfo{'a',6,"0,0,0.25"},
    wyckInfo{'b',6,"0,0,0"},
    wyckInfo{'c',12,"0,0,z"},
    wyckInfo{'d',18,"0.5,0,0"},
    wyckInfo{'e',18,"x,0,0.25"},
    wyckInfo{'f',36,"x,y,z"}
  },

  { // 168
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',3,"0.5,0,z"},
    wyckInfo{'d',6,"x,y,z"}
  },

  { // 169
    wyckInfo{'a',6,"x,y,z"}
  },

  { // 170
    wyckInfo{'a',6,"x,y,z"}
  },

  { // 171
    wyckInfo{'a',3,"0,0,z"},
    wyckInfo{'b',3,"0.5,0.5,z"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 172
    wyckInfo{'a',3,"0,0,z"},
    wyckInfo{'b',3,"0.5,0.5,z"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 173
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',6,"x,y,z"}
  },

  { // 174
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.333333,0.666667,0"},
    wyckInfo{'d',1,"0.333333,0.666667,0.5"},
    wyckInfo{'e',1,"0.666667,0.333333,0"},
    wyckInfo{'f',1,"0.666667,0.333333,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.333333,0.666667,z"},
    wyckInfo{'i',2,"0.666667,0.333333,z"},
    wyckInfo{'j',3,"x,y,0"},
    wyckInfo{'k',3,"x,y,0.5"},
    wyckInfo{'l',6,"x,y,z"}
  },

  { // 175
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',3,"0.5,0,0"},
    wyckInfo{'g',3,"0.5,0,0.5"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',6,"0.5,0,z"},
    wyckInfo{'j',6,"x,y,0"},
    wyckInfo{'k',6,"x,y,0.5"},
    wyckInfo{'l',12,"x,y,z"}
  },

  { // 176
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',2,"0.333333,0.666667,0.25"},
    wyckInfo{'d',2,"0.666667,0.333333,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0.333333,0.666667,z"},
    wyckInfo{'g',6,"0.5,0,0"},
    wyckInfo{'h',6,"x,y,0.25"},
    wyckInfo{'i',12,"x,y,z"}
  },

  { // 177
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',3,"0.5,0,0"},
    wyckInfo{'g',3,"0.5,0,0.5"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',6,"0.5,0,z"},
    wyckInfo{'j',6,"x,0,0"},
    wyckInfo{'k',6,"x,0,0.5"},
    wyckInfo{'l',6,"x,-x,0"},
    wyckInfo{'m',6,"x,-x,0.5"},
    wyckInfo{'n',12,"x,y,z"}
  },

  { // 178
    wyckInfo{'a',6,"x,0,0"},
    wyckInfo{'b',6,"x,2x,0.25"},
    wyckInfo{'c',12,"x,y,z"}
  },

  { // 179
    wyckInfo{'a',6,"x,0,0"},
    wyckInfo{'b',6,"x,2x,0.75"},
    wyckInfo{'c',12,"x,y,z"}
  },

  { // 180
    wyckInfo{'a',3,"0,0,0"},
    wyckInfo{'b',3,"0,0,0.5"},
    wyckInfo{'c',3,"0.5,0,0"},
    wyckInfo{'d',3,"0.5,0,0.5"},
    wyckInfo{'e',6,"0,0,z"},
    wyckInfo{'f',6,"0.5,0,z"},
    wyckInfo{'g',6,"x,0,0"},
    wyckInfo{'h',6,"x,0,0.5"},
    wyckInfo{'i',6,"x,2x,0"},
    wyckInfo{'j',6,"x,2x,0.5"},
    wyckInfo{'k',12,"x,y,z"}
  },

  { // 181
    wyckInfo{'a',3,"0,0,0"},
    wyckInfo{'b',3,"0,0,0.5"},
    wyckInfo{'c',3,"0.5,0,0"},
    wyckInfo{'d',3,"0.5,0,0.5"},
    wyckInfo{'e',6,"0,0,z"},
    wyckInfo{'f',6,"0.5,0,z"},
    wyckInfo{'g',6,"x,0,0"},
    wyckInfo{'h',6,"x,0,0.5"},
    wyckInfo{'i',6,"x,2x,0"},
    wyckInfo{'j',6,"x,2x,0.5"},
    wyckInfo{'k',12,"x,y,z"}
  },

  { // 182
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.25"},
    wyckInfo{'c',2,"0.333333,0.666667,0.25"},
    wyckInfo{'d',2,"0.333333,0.666667,0.75"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0.333333,0.666667,z"},
    wyckInfo{'g',6,"x,0,0"},
    wyckInfo{'h',6,"x,2x,0.25"},
    wyckInfo{'i',12,"x,y,z"}
  },

  { // 183
    wyckInfo{'a',1,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',3,"0.5,0,z"},
    wyckInfo{'d',6,"x,0,z"},
    wyckInfo{'e',6,"x,-x,z"},
    wyckInfo{'f',12,"x,y,z"}
  },

  { // 184
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0.333333,0.666667,z"},
    wyckInfo{'c',6,"0.5,0,z"},
    wyckInfo{'d',12,"x,y,z"}
  },

  { // 185
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',4,"0.333333,0.666667,z"},
    wyckInfo{'c',6,"x,0,z"},
    wyckInfo{'d',12,"x,y,z"}
  },

  { // 186
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0.333333,0.666667,z"},
    wyckInfo{'c',6,"x,-x,z"},
    wyckInfo{'d',12,"x,y,z"}
  },

  { // 187
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0.333333,0.666667,0"},
    wyckInfo{'d',1,"0.333333,0.666667,0.5"},
    wyckInfo{'e',1,"0.666667,0.333333,0"},
    wyckInfo{'f',1,"0.666667,0.333333,0.5"},
    wyckInfo{'g',2,"0,0,z"},
    wyckInfo{'h',2,"0.333333,0.666667,z"},
    wyckInfo{'i',2,"0.666667,0.333333,z"},
    wyckInfo{'j',3,"x,-x,0"},
    wyckInfo{'k',3,"x,-x,0.5"},
    wyckInfo{'l',6,"x,y,0"},
    wyckInfo{'m',6,"x,y,0.5"},
    wyckInfo{'n',6,"x,-x,z"},
    wyckInfo{'o',12,"x,y,z"}
  },

  { // 188
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.25"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.25"},
    wyckInfo{'e',2,"0.666667,0.333333,0"},
    wyckInfo{'f',2,"0.666667,0.333333,0.25"},
    wyckInfo{'g',4,"0,0,z"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',4,"0.666667,0.333333,z"},
    wyckInfo{'j',6,"x,-x,0"},
    wyckInfo{'k',6,"x,y,0.25"},
    wyckInfo{'l',12,"x,y,z"}
  },

  { // 189
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',3,"x,0,0"},
    wyckInfo{'g',3,"x,0,0.5"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',6,"x,0,z"},
    wyckInfo{'j',6,"x,y,0"},
    wyckInfo{'k',6,"x,y,0.5"},
    wyckInfo{'l',12,"x,y,z"}
  },

  { // 190
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.25"},
    wyckInfo{'c',2,"0.333333,0.666667,0.25"},
    wyckInfo{'d',2,"0.666667,0.333333,0.25"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0.333333,0.666667,z"},
    wyckInfo{'g',6,"x,0,0"},
    wyckInfo{'h',6,"x,y,0.25"},
    wyckInfo{'i',12,"x,y,z"}
  },

  { // 191
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',2,"0.333333,0.666667,0"},
    wyckInfo{'d',2,"0.333333,0.666667,0.5"},
    wyckInfo{'e',2,"0,0,z"},
    wyckInfo{'f',3,"0.5,0,0"},
    wyckInfo{'g',3,"0.5,0,0.5"},
    wyckInfo{'h',4,"0.333333,0.666667,z"},
    wyckInfo{'i',6,"0.5,0,z"},
    wyckInfo{'j',6,"x,0,0"},
    wyckInfo{'k',6,"x,0,0.5"},
    wyckInfo{'l',6,"x,2x,0"},
    wyckInfo{'m',6,"x,2x,0.5"},
    wyckInfo{'n',12,"x,0,z"},
    wyckInfo{'o',12,"x,2x,z"},
    wyckInfo{'p',12,"x,y,0"},
    wyckInfo{'q',12,"x,y,0.5"},
    wyckInfo{'r',24,"x,y,z"}
  },

  { // 192
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',4,"0.333333,0.666667,0.25"},
    wyckInfo{'d',4,"0.333333,0.666667,0"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',6,"0.5,0,0.25"},
    wyckInfo{'g',6,"0.5,0,0"},
    wyckInfo{'h',8,"0.333333,0.666667,z"},
    wyckInfo{'i',12,"0.5,0,z"},
    wyckInfo{'j',12,"x,0,0.25"},
    wyckInfo{'k',12,"x,2x,0.25"},
    wyckInfo{'l',12,"x,y,0"},
    wyckInfo{'m',24,"x,y,z"}
  },

  { // 193
    wyckInfo{'a',2,"0,0,0.25"},
    wyckInfo{'b',2,"0,0,0"},
    wyckInfo{'c',4,"0.333333,0.666667,0.25"},
    wyckInfo{'d',4,"0.333333,0.666667,0"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',6,"0.5,0,0"},
    wyckInfo{'g',6,"x,0,0.25"},
    wyckInfo{'h',8,"0.333333,0.666667,z"},
    wyckInfo{'i',12,"x,2x,0"},
    wyckInfo{'j',12,"x,y,0.25"},
    wyckInfo{'k',12,"x,0,z"},
    wyckInfo{'l',24,"x,y,z"}
  },

  { // 194
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0,0.25"},
    wyckInfo{'c',2,"0.333333,0.666667,0.25"},
    wyckInfo{'d',2,"0.333333,0.666667,0.75"},
    wyckInfo{'e',4,"0,0,z"},
    wyckInfo{'f',4,"0.333333,0.666667,z"},
    wyckInfo{'g',6,"0.5,0,0"},
    wyckInfo{'h',6,"x,2x,0.25"},
    wyckInfo{'i',12,"x,0,0"},
    wyckInfo{'j',12,"x,y,0.25"},
    wyckInfo{'k',12,"x,2x,z"},
    wyckInfo{'l',24,"x,y,z"}
  },

  { // 195
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',3,"0,0.5,0.5"},
    wyckInfo{'d',3,"0.5,0,0"},
    wyckInfo{'e',4,"x,x,x"},
    wyckInfo{'f',6,"x,0,0"},
    wyckInfo{'g',6,"x,0,0.5"},
    wyckInfo{'h',6,"x,0.5,0"},
    wyckInfo{'i',6,"x,0.5,0.5"},
    wyckInfo{'j',12,"x,y,z"}
  },

  { // 196
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.75,0.75,0.75"},
    wyckInfo{'e',16,"x,x,x"},
    wyckInfo{'f',24,"x,0,0"},
    wyckInfo{'g',24,"x,0.25,0.25"},
    wyckInfo{'h',48,"x,y,z"}
  },

  { // 197
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',8,"x,x,x"},
    wyckInfo{'d',12,"x,0,0"},
    wyckInfo{'e',12,"x,0.5,0"},
    wyckInfo{'f',24,"x,y,z"}
  },

  { // 198
    wyckInfo{'a',4,"x,x,x"},
    wyckInfo{'b',12,"x,y,z"}
  },

  { // 199
    wyckInfo{'a',8,"x,x,x"},
    wyckInfo{'b',12,"x,0,0.25"},
    wyckInfo{'c',24,"x,y,z"}
  },

  { // 200
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',3,"0,0.5,0.5"},
    wyckInfo{'d',3,"0.5,0,0"},
    wyckInfo{'e',6,"x,0,0"},
    wyckInfo{'f',6,"x,0,0.5"},
    wyckInfo{'g',6,"x,0.5,0"},
    wyckInfo{'h',6,"x,0.5,0.5"},
    wyckInfo{'i',8,"x,x,x"},
    wyckInfo{'j',12,"0,y,z"},
    wyckInfo{'k',12,"0.5,y,z"},
    wyckInfo{'l',24,"x,y,z"}
  },

  { // 201
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',4,"0,0,0"},
    wyckInfo{'c',4,"0.5,0.5,0.5"},
    wyckInfo{'d',6,"0.25,0.75,0.75"},
    wyckInfo{'e',8,"x,x,x"},
    wyckInfo{'f',12,"x,0.25,0.25"},
    wyckInfo{'g',12,"x,0.75,0.25"},
    wyckInfo{'h',24,"x,y,z"}
  },

  { // 202
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',24,"0,0.25,0.25"},
    wyckInfo{'e',24,"x,0,0"},
    wyckInfo{'f',32,"x,x,x"},
    wyckInfo{'g',48,"x,0.25,0.25"},
    wyckInfo{'h',48,"0,y,z"},
    wyckInfo{'i',96,"x,y,z"}
  },

  { // 203
    wyckInfo{'a',8,"0.125,0.125,0.125"},
    wyckInfo{'b',8,"0.625,0.625,0.625"},
    wyckInfo{'c',16,"0,0,0"},
    wyckInfo{'d',16,"0.5,0.5,0.5"},
    wyckInfo{'e',32,"x,x,x"},
    wyckInfo{'f',48,"x,0.125,0.125"},
    wyckInfo{'g',96,"x,y,z"}
  },

  { // 204
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',12,"x,0,0"},
    wyckInfo{'e',12,"x,0,0.5"},
    wyckInfo{'f',16,"x,x,x"},
    wyckInfo{'g',24,"0,y,z"},
    wyckInfo{'h',48,"x,y,z"}
  },

  { // 205
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',8,"x,x,x"},
    wyckInfo{'d',24,"x,y,z"}
  },

  { // 206
    wyckInfo{'a',8,"0,0,0"},
    wyckInfo{'b',8,"0.25,0.25,0.25"},
    wyckInfo{'c',16,"x,x,x"},
    wyckInfo{'d',24,"x,0,0.25"},
    wyckInfo{'e',48,"x,y,z"}
  },

  { // 207
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',3,"0,0.5,0.5"},
    wyckInfo{'d',3,"0.5,0,0"},
    wyckInfo{'e',6,"x,0,0"},
    wyckInfo{'f',6,"x,0.5,0.5"},
    wyckInfo{'g',8,"x,x,x"},
    wyckInfo{'h',12,"x,0.5,0"},
    wyckInfo{'i',12,"0,y,y"},
    wyckInfo{'j',12,"0.5,y,y"},
    wyckInfo{'k',24,"x,y,z"}
  },

  { // 208
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',4,"0.25,0.25,0.25"},
    wyckInfo{'c',4,"0.75,0.75,0.75"},
    wyckInfo{'d',6,"0,0.5,0.5"},
    wyckInfo{'e',6,"0.25,0,0.5"},
    wyckInfo{'f',6,"0.25,0.5,0"},
    wyckInfo{'g',8,"x,x,x"},
    wyckInfo{'h',12,"x,0,0"},
    wyckInfo{'i',12,"x,0,0.5"},
    wyckInfo{'j',12,"x,0.5,0"},
    wyckInfo{'k',12,"0.25,y,-y+0.5"},
    wyckInfo{'l',12,"0.25,y,y+0.5"},
    wyckInfo{'m',24,"x,y,z"}
  },

  { // 209
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',24,"0,0.25,0.25"},
    wyckInfo{'e',24,"x,0,0"},
    wyckInfo{'f',32,"x,x,x"},
    wyckInfo{'g',48,"0,y,y"},
    wyckInfo{'h',48,"0.5,y,y"},
    wyckInfo{'i',48,"x,0.25,0.25"},
    wyckInfo{'j',96,"x,y,z"}
  },

  { // 210
    wyckInfo{'a',8,"0,0,0"},
    wyckInfo{'b',8,"0.5,0.5,0.5"},
    wyckInfo{'c',16,"0.125,0.125,0.125"},
    wyckInfo{'d',16,"0.625,0.625,0.625"},
    wyckInfo{'e',32,"x,x,x"},
    wyckInfo{'f',48,"x,0,0"},
    wyckInfo{'g',48,"0.125,y,-y+0.25"},
    wyckInfo{'h',96,"x,y,z"}
  },

  { // 211
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',12,"0.25,0.5,0"},
    wyckInfo{'e',12,"x,0,0"},
    wyckInfo{'f',16,"x,x,x"},
    wyckInfo{'g',24,"x,0.5,0"},
    wyckInfo{'h',24,"0,y,y"},
    wyckInfo{'i',24,"0.25,y,-y+0.5"},
    wyckInfo{'j',48,"x,y,z"}
  },

  { // 212
    wyckInfo{'a',4,"0.125,0.125,0.125"},
    wyckInfo{'b',4,"0.625,0.625,0.625"},
    wyckInfo{'c',8,"x,x,x"},
    wyckInfo{'d',12,"0.125,y,-y+0.25"},
    wyckInfo{'e',24,"x,y,z"}
  },

  { // 213
    wyckInfo{'a',4,"0.375,0.375,0.375"},
    wyckInfo{'b',4,"0.875,0.875,0.875"},
    wyckInfo{'c',8,"x,x,x"},
    wyckInfo{'d',12,"0.125,y,y+0.25"},
    wyckInfo{'e',24,"x,y,z"}
  },

  { // 214
    wyckInfo{'a',8,"0.125,0.125,0.125"},
    wyckInfo{'b',8,"0.875,0.875,0.875"},
    wyckInfo{'c',12,"0.125,0,0.25"},
    wyckInfo{'d',12,"0.625,0,0.25"},
    wyckInfo{'e',16,"x,x,x"},
    wyckInfo{'f',24,"x,0,0.25"},
    wyckInfo{'g',24,"0.125,y,y+0.25"},
    wyckInfo{'h',24,"0.125,y,-y+0.25"},
    wyckInfo{'i',48,"x,y,z"}
  },

  { // 215
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',3,"0,0.5,0.5"},
    wyckInfo{'d',3,"0.5,0,0"},
    wyckInfo{'e',4,"x,x,x"},
    wyckInfo{'f',6,"x,0,0"},
    wyckInfo{'g',6,"x,0.5,0.5"},
    wyckInfo{'h',12,"x,0.5,0"},
    wyckInfo{'i',12,"x,x,z"},
    wyckInfo{'j',24,"x,y,z"}
  },

  { // 216
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',4,"0.25,0.25,0.25"},
    wyckInfo{'d',4,"0.75,0.75,0.75"},
    wyckInfo{'e',16,"x,x,x"},
    wyckInfo{'f',24,"x,0,0"},
    wyckInfo{'g',24,"x,0.25,0.25"},
    wyckInfo{'h',48,"x,x,z"},
    wyckInfo{'i',96,"x,y,z"}
  },

  { // 217
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',8,"x,x,x"},
    wyckInfo{'d',12,"0.25,0.5,0"},
    wyckInfo{'e',12,"x,0,0"},
    wyckInfo{'f',24,"x,0.5,0"},
    wyckInfo{'g',24,"x,x,z"},
    wyckInfo{'h',48,"x,y,z"}
  },

  { // 218
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',6,"0.25,0.5,0"},
    wyckInfo{'d',6,"0.25,0,0.5"},
    wyckInfo{'e',8,"x,x,x"},
    wyckInfo{'f',12,"x,0,0"},
    wyckInfo{'g',12,"x,0.5,0"},
    wyckInfo{'h',12,"x,0,0.5"},
    wyckInfo{'i',24,"x,y,z"}
  },

  { // 219
    wyckInfo{'a',8,"0,0,0"},
    wyckInfo{'b',8,"0.25,0.25,0.25"},
    wyckInfo{'c',24,"0,0.25,0.25"},
    wyckInfo{'d',24,"0.25,0,0"},
    wyckInfo{'e',32,"x,x,x"},
    wyckInfo{'f',48,"x,0,0"},
    wyckInfo{'g',48,"x,0.25,0.25"},
    wyckInfo{'h',96,"x,y,z"}
  },

  { // 220
    wyckInfo{'a',12,"0.375,0,0.25"},
    wyckInfo{'b',12,"0.875,0,0.25"},
    wyckInfo{'c',16,"x,x,x"},
    wyckInfo{'d',24,"x,0,0.25"},
    wyckInfo{'e',48,"x,y,z"}
  },

  { // 221
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0.5,0.5"},
    wyckInfo{'c',3,"0,0.5,0.5"},
    wyckInfo{'d',3,"0.5,0,0"},
    wyckInfo{'e',6,"x,0,0"},
    wyckInfo{'f',6,"x,0.5,0.5"},
    wyckInfo{'g',8,"x,x,x"},
    wyckInfo{'h',12,"x,0.5,0"},
    wyckInfo{'i',12,"0,y,y"},
    wyckInfo{'j',12,"0.5,y,y"},
    wyckInfo{'k',24,"0,y,z"},
    wyckInfo{'l',24,"0.5,y,z"},
    wyckInfo{'m',24,"x,x,z"},
    wyckInfo{'n',48,"x,y,z"}
  },

  { // 222
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',6,"0.75,0.25,0.25"},
    wyckInfo{'c',8,"0,0,0"},
    wyckInfo{'d',12,"0,0.75,0.25"},
    wyckInfo{'e',12,"x,0.25,0.25"},
    wyckInfo{'f',16,"x,x,x"},
    wyckInfo{'g',24,"x,0.75,0.25"},
    wyckInfo{'h',24,"0.25,y,y"},
    wyckInfo{'i',48,"x,y,z"}
  },

  { // 223
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',6,"0.25,0,0.5"},
    wyckInfo{'d',6,"0.25,0.5,0"},
    wyckInfo{'e',8,"0.25,0.25,0.25"},
    wyckInfo{'f',12,"x,0,0"},
    wyckInfo{'g',12,"x,0,0.5"},
    wyckInfo{'h',12,"x,0.5,0"},
    wyckInfo{'i',16,"x,x,x"},
    wyckInfo{'j',24,"0.25,y,y+0.5"},
    wyckInfo{'k',24,"0,y,z"},
    wyckInfo{'l',48,"x,y,z"}
  },

  { // 224
    wyckInfo{'a',2,"0.25,0.25,0.25"},
    wyckInfo{'b',4,"0,0,0"},
    wyckInfo{'c',4,"0.5,0.5,0.5"},
    wyckInfo{'d',6,"0.25,0.75,0.75"},
    wyckInfo{'e',8,"x,x,x"},
    wyckInfo{'f',12,"0.5,0.25,0.75"},
    wyckInfo{'g',12,"x,0.25,0.25"},
    wyckInfo{'h',24,"x,0.25,0.75"},
    wyckInfo{'i',24,"0.5,y,y+0.5"},
    wyckInfo{'j',24,"0.5,y,-y"},
    wyckInfo{'k',24,"x,x,z"},
    wyckInfo{'l',48,"x,y,z"}
  },

  { // 225
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0.5,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',24,"0,0.25,0.25"},
    wyckInfo{'e',24,"x,0,0"},
    wyckInfo{'f',32,"x,x,x"},
    wyckInfo{'g',48,"x,0.25,0.25"},
    wyckInfo{'h',48,"0,y,y"},
    wyckInfo{'i',48,"0.5,y,y"},
    wyckInfo{'j',96,"0,y,z"},
    wyckInfo{'k',96,"x,x,z"},
    wyckInfo{'l',192,"x,y,z"}
  },

  { // 226
    wyckInfo{'a',8,"0.25,0.25,0.25"},
    wyckInfo{'b',8,"0,0,0"},
    wyckInfo{'c',24,"0.25,0,0"},
    wyckInfo{'d',24,"0,0.25,0.25"},
    wyckInfo{'e',48,"x,0,0"},
    wyckInfo{'f',48,"x,0.25,0.25"},
    wyckInfo{'g',64,"x,x,x"},
    wyckInfo{'h',96,"0.25,y,y"},
    wyckInfo{'i',96,"0,y,z"},
    wyckInfo{'j',192,"x,y,z"}
  },

  { // 227
    wyckInfo{'a',8,"0.125,0.125,0.125"},
    wyckInfo{'b',8,"0.375,0.375,0.375"},
    wyckInfo{'c',16,"0,0,0"},
    wyckInfo{'d',16,"0.5,0.5,0.5"},
    wyckInfo{'e',32,"x,x,x"},
    wyckInfo{'f',48,"x,0.125,0.125"},
    wyckInfo{'g',96,"x,x,z"},
    wyckInfo{'h',96,"0,y,-y"},
    wyckInfo{'i',192,"x,y,z"}
  },

  { // 228
    wyckInfo{'a',16,"0.125,0.125,0.125"},
    wyckInfo{'b',32,"0.25,0.25,0.25"},
    wyckInfo{'c',32,"0,0,0"},
    wyckInfo{'d',48,"0.875,0.125,0.125"},
    wyckInfo{'e',64,"x,x,x"},
    wyckInfo{'f',96,"x,0.125,0.125"},
    wyckInfo{'g',96,"0.25,y,-y"},
    wyckInfo{'h',192,"x,y,z"}
  },

  { // 229
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',6,"0,0.5,0.5"},
    wyckInfo{'c',8,"0.25,0.25,0.25"},
    wyckInfo{'d',12,"0.25,0,0.5"},
    wyckInfo{'e',12,"x,0,0"},
    wyckInfo{'f',16,"x,x,x"},
    wyckInfo{'g',24,"x,0,0.5"},
    wyckInfo{'h',24,"0,y,y"},
    wyckInfo{'i',48,"0.25,y,-y+0.5"},
    wyckInfo{'j',48,"0,y,z"},
    wyckInfo{'k',48,"x,x,z"},
    wyckInfo{'l',96,"x,y,z"}
  },

  { // 230
    wyckInfo{'a',16,"0,0,0"},
    wyckInfo{'b',16,"0.125,0.125,0.125"},
    wyckInfo{'c',24,"0.125,0,0.25"},
    wyckInfo{'d',24,"0.375,0,0.25"},
    wyckInfo{'e',32,"x,x,x"},
    wyckInfo{'f',48,"x,0,0.25"},
    wyckInfo{'g',48,"0.125,y,-y+0.25"},
    wyckInfo{'h',96,"x,y,z"}
  }

};

#endif
