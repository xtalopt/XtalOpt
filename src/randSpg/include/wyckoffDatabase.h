/**********************************************************************
  wyckoffDatabase.h - Database that contains the wyckoff letter, multiplicity,
                      the first position of each wyckoff position, and a bool
                      indicating whether the position is unique or not. It
                      contains all the Wyckoff positions for every
                      spacegroup. The database is stored as a static const
                      vector of vectors of tuples.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef WYCKOFF_DATABASE_H
#define WYCKOFF_DATABASE_H

#include "randSpg.h"

// Outer vector is the space group. So there are 230 of them
// Inner vector depends on the number of wyckoff elements that
// exist in each space group
// This contains the wyckoff letter, multiplicity, (x,y,z) coordinates for
// the first wyckoff position of each spacegroup, and a bool indicating
// whether the position is unique or not (this is cached to improve speed)
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
    wyckPos{'a',1,"x,y,z",false}
  },

  { // 2
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0,0.5,0",true},
    wyckPos{'d',1,"0.5,0,0",true},
    wyckPos{'e',1,"0.5,0.5,0",true},
    wyckPos{'f',1,"0.5,0,0.5",true},
    wyckPos{'g',1,"0,0.5,0.5",true},
    wyckPos{'h',1,"0.5,0.5,0.5",true},
    wyckPos{'i',2,"x,y,z",false}
  },

  { // 3 - unique axis b
    wyckPos{'a',1,"0,y,0",false},
    wyckPos{'b',1,"0,y,0.5",false},
    wyckPos{'c',1,"0.5,y,0",false},
    wyckPos{'d',1,"0.5,y,0.5",false},
    wyckPos{'e',2,"x,y,z",false}
  },

  { // 4 - unique axis b
    wyckPos{'a',2,"x,y,z",false}
  },

  { // 5 - unique axis b
    wyckPos{'a',2,"0,y,0",false},
    wyckPos{'b',2,"0,y,0.5",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 6 - unique axis b
    wyckPos{'a',1,"x,0,z",false},
    wyckPos{'b',1,"x,0.5,z",false},
    wyckPos{'c',2,"x,y,z",false}
  },

  { // 7 - unique axis b
    wyckPos{'a',2,"x,y,z",false}
  },

  { // 8 - unique axis b
    wyckPos{'a',2,"x,0,z",false},
    wyckPos{'b',4,"x,y,z",false}
  },

  { // 9 - unique axis b
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 10 - unique axis b
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0.5,0",true},
    wyckPos{'c',1,"0,0,0.5",true},
    wyckPos{'d',1,"0.5,0,0",true},
    wyckPos{'e',1,"0.5,0.5,0",true},
    wyckPos{'f',1,"0,0.5,0.5",true},
    wyckPos{'g',1,"0.5,0,0.5",true},
    wyckPos{'h',1,"0.5,0.5,0.5",true},
    wyckPos{'i',2,"0,y,0",false},
    wyckPos{'j',2,"0.5,y,0",false},
    wyckPos{'k',2,"0,y,0.5",false},
    wyckPos{'l',2,"0.5,y,0.5",false},
    wyckPos{'m',2,"x,0,z",false},
    wyckPos{'n',2,"x,0.5,z",false},
    wyckPos{'o',4,"x,y,z",false}
  },

  { // 11 - unique axis b
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0,0",true},
    wyckPos{'c',2,"0,0,0.5",true},
    wyckPos{'d',2,"0.5,0,0.5",true},
    wyckPos{'e',2,"x,0.25,z",false},
    wyckPos{'f',4,"x,y,z",false}
  },

  { // 12 - unique axis b
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0.5,0",true},
    wyckPos{'c',2,"0,0,0.5",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',4,"0.25,0.25,0",true},
    wyckPos{'f',4,"0.25,0.25,0.5",true},
    wyckPos{'g',4,"0,y,0",false},
    wyckPos{'h',4,"0,y,0.5",false},
    wyckPos{'i',4,"x,0,z",false},
    wyckPos{'j',8,"x,y,z",false}
  },

  { // 13 - unique axis b
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0.5,0",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0.5,0,0",true},
    wyckPos{'e',2,"0,y,0.25",false},
    wyckPos{'f',2,"0.5,y,0.25",false},
    wyckPos{'g',4,"x,y,z",false}
  },

  { // 14 - unique axis b
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0,0",true},
    wyckPos{'c',2,"0,0,0.5",true},
    wyckPos{'d',2,"0.5,0,0.5",true},
    wyckPos{'e',4,"x,y,z",false}
  },

  { // 15 - unique axis b
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0.5,0",true},
    wyckPos{'c',4,"0.25,0.25,0",true},
    wyckPos{'d',4,"0.25,0.25,0.5",true},
    wyckPos{'e',4,"0,y,0.25",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 16
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0,0",true},
    wyckPos{'c',1,"0,0.5,0",true},
    wyckPos{'d',1,"0,0,0.5",true},
    wyckPos{'e',1,"0.5,0.5,0",true},
    wyckPos{'f',1,"0.5,0,0.5",true},
    wyckPos{'g',1,"0,0.5,0.5",true},
    wyckPos{'h',1,"0.5,0.5,0.5",true},
    wyckPos{'i',2,"x,0,0",false},
    wyckPos{'j',2,"x,0,0.5",false},
    wyckPos{'k',2,"x,0.5,0",false},
    wyckPos{'l',2,"x,0.5,0.5",false},
    wyckPos{'m',2,"0,y,0",false},
    wyckPos{'n',2,"0,y,0.5",false},
    wyckPos{'o',2,"0.5,y,0",false},
    wyckPos{'p',2,"0.5,y,0.5",false},
    wyckPos{'q',2,"0,0,z",false},
    wyckPos{'r',2,"0.5,0,z",false},
    wyckPos{'s',2,"0,0.5,z",false},
    wyckPos{'t',2,"0.5,0.5,z",false},
    wyckPos{'u',4,"x,y,z",false}
  },

  { // 17
    wyckPos{'a',2,"x,0,0",false},
    wyckPos{'b',2,"x,0.5,0",false},
    wyckPos{'c',2,"0,y,0.25",false},
    wyckPos{'d',2,"0.5,y,0.25",false},
    wyckPos{'e',4,"x,y,z",false}
  },

  { // 18
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 19
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 20
    wyckPos{'a',4,"x,0,0",false},
    wyckPos{'b',4,"0,y,0.25",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 21
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0.5,0",true},
    wyckPos{'c',2,"0.5,0,0.5",true},
    wyckPos{'d',2,"0,0,0.5",true},
    wyckPos{'e',4,"x,0,0",false},
    wyckPos{'f',4,"x,0,0.5",false},
    wyckPos{'g',4,"0,y,0",false},
    wyckPos{'h',4,"0,y,0.5",false},
    wyckPos{'i',4,"0,0,z",false},
    wyckPos{'j',4,"0,0.5,z",false},
    wyckPos{'k',4,"0.25,0.25,z",false},
    wyckPos{'l',8,"x,y,z",false}
  },

  { // 22
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.25,0.25,0.75",true},
    wyckPos{'e',8,"x,0,0",false},
    wyckPos{'f',8,"0,y,0",false},
    wyckPos{'g',8,"0,0,z",false},
    wyckPos{'h',8,"0.25,0.25,z",false},
    wyckPos{'i',8,"0.25,y,0.25",false},
    wyckPos{'j',8,"x,0.25,0.25",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 23
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0,0",true},
    wyckPos{'c',2,"0,0,0.5",true},
    wyckPos{'d',2,"0,0.5,0",true},
    wyckPos{'e',4,"x,0,0",false},
    wyckPos{'f',4,"x,0,0.5",false},
    wyckPos{'g',4,"0,y,0",false},
    wyckPos{'h',4,"0.5,y,0",false},
    wyckPos{'i',4,"0,0,z",false},
    wyckPos{'j',4,"0,0.5,z",false},
    wyckPos{'k',8,"x,y,z",false}
  },

  { // 24
    wyckPos{'a',4,"x,0,0.25",false},
    wyckPos{'b',4,"0.25,y,0",false},
    wyckPos{'c',4,"0,0.25,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 25
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',1,"0,0.5,z",false},
    wyckPos{'c',1,"0.5,0,z",false},
    wyckPos{'d',1,"0.5,0.5,z",false},
    wyckPos{'e',2,"x,0,z",false},
    wyckPos{'f',2,"x,0.5,z",false},
    wyckPos{'g',2,"0,y,z",false},
    wyckPos{'h',2,"0.5,y,z",false},
    wyckPos{'i',4,"x,y,z",false}
  },

  { // 26
    wyckPos{'a',2,"0,y,z",false},
    wyckPos{'b',2,"0.5,y,z",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 27
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',2,"0.5,0,z",false},
    wyckPos{'d',2,"0.5,0.5,z",false},
    wyckPos{'e',4,"x,y,z",false}
  },

  { // 28
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',2,"0.25,y,z",false},
    wyckPos{'d',4,"x,y,z",false}
  },

  { // 29
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 30
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0,z",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 31
    wyckPos{'a',2,"0,y,z",false},
    wyckPos{'b',4,"x,y,z",false}
  },

  { // 32
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 33
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 34
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',4,"x,y,z",false}
  },

  { // 35
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',4,"0.25,0.25,z",false},
    wyckPos{'d',4,"x,0,z",false},
    wyckPos{'e',4,"0,y,z",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 36
    wyckPos{'a',4,"0,y,z",false},
    wyckPos{'b',8,"x,y,z",false}
  },

  { // 37
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',4,"0.25,0.25,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 38
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0,z",false},
    wyckPos{'c',4,"x,0,z",false},
    wyckPos{'d',4,"0,y,z",false},
    wyckPos{'e',4,"0.5,y,z",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 39
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0.5,0,z",false},
    wyckPos{'c',4,"x,0.25,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 40
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0.25,y,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 41
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',8,"x,y,z",false}
  },

  { // 42
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',8,"0.25,0.25,z",false},
    wyckPos{'c',8,"0,y,z",false},
    wyckPos{'d',8,"x,0,z",false},
    wyckPos{'e',16,"x,y,z",false}
  },

  { // 43
    wyckPos{'a',8,"0,0,z",false},
    wyckPos{'b',16,"x,y,z",false}
  },

  { // 44
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0,0.5,z",false},
    wyckPos{'c',4,"x,0,z",false},
    wyckPos{'d',4,"0,y,z",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 45
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 46
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0.25,y,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 47
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0,0",true},
    wyckPos{'c',1,"0,0,0.5",true},
    wyckPos{'d',1,"0.5,0,0.5",true},
    wyckPos{'e',1,"0,0.5,0",true},
    wyckPos{'f',1,"0.5,0.5,0",true},
    wyckPos{'g',1,"0,0.5,0.5",true},
    wyckPos{'h',1,"0.5,0.5,0.5",true},
    wyckPos{'i',2,"x,0,0",false},
    wyckPos{'j',2,"x,0,0.5",false},
    wyckPos{'k',2,"x,0.5,0",false},
    wyckPos{'l',2,"x,0.5,0.5",false},
    wyckPos{'m',2,"0,y,0",false},
    wyckPos{'n',2,"0,y,0.5",false},
    wyckPos{'o',2,"0.5,y,0",false},
    wyckPos{'p',2,"0.5,y,0.5",false},
    wyckPos{'q',2,"0,0,z",false},
    wyckPos{'r',2,"0,0.5,z",false},
    wyckPos{'s',2,"0.5,0,z",false},
    wyckPos{'t',2,"0.5,0.5,z",false},
    wyckPos{'u',4,"0,y,z",false},
    wyckPos{'v',4,"0.5,y,z",false},
    wyckPos{'w',4,"x,0,z",false},
    wyckPos{'x',4,"x,0.5,z",false},
    wyckPos{'y',4,"x,y,0",false},
    wyckPos{'z',4,"x,y,0.5",false},
    wyckPos{'A',8,"x,y,z",false}
  },

  { // 48
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',2,"0.75,0.25,0.25",true},
    wyckPos{'c',2,"0.25,0.25,0.75",true},
    wyckPos{'d',2,"0.25,0.75,0.25",true},
    wyckPos{'e',4,"0.5,0.5,0.5",true},
    wyckPos{'f',4,"0,0,0",true},
    wyckPos{'g',4,"x,0.25,0.25",false},
    wyckPos{'h',4,"x,0.25,0.75",false},
    wyckPos{'i',4,"0.25,y,0.25",false},
    wyckPos{'j',4,"0.75,y,0.25",false},
    wyckPos{'k',4,"0.25,0.25,z",false},
    wyckPos{'l',4,"0.25,0.75,z",false},
    wyckPos{'m',8,"x,y,z",false}
  },

  { // 49
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0.5,0",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0.5,0,0",true},
    wyckPos{'e',2,"0,0,0.25",true},
    wyckPos{'f',2,"0.5,0,0.25",true},
    wyckPos{'g',2,"0,0.5,0.25",true},
    wyckPos{'h',2,"0.5,0.5,0.25",true},
    wyckPos{'i',4,"x,0,0.25",false},
    wyckPos{'j',4,"x,0.5,0.25",false},
    wyckPos{'k',4,"0,y,0.25",false},
    wyckPos{'l',4,"0.5,y,0.25",false},
    wyckPos{'m',4,"0,0,z",false},
    wyckPos{'n',4,"0.5,0.5,z",false},
    wyckPos{'o',4,"0,0.5,z",false},
    wyckPos{'p',4,"0.5,0,z",false},
    wyckPos{'q',4,"x,y,0",false},
    wyckPos{'r',8,"x,y,z",false}
  },

  { // 50
    wyckPos{'a',2,"0.25,0.25,0",true},
    wyckPos{'b',2,"0.75,0.25,0",true},
    wyckPos{'c',2,"0.75,0.25,0.5",true},
    wyckPos{'d',2,"0.25,0.25,0.5",true},
    wyckPos{'e',4,"0,0,0",true},
    wyckPos{'f',4,"0,0,0.5",true},
    wyckPos{'g',4,"x,0.25,0",false},
    wyckPos{'h',4,"x,0.25,0.5",false},
    wyckPos{'i',4,"0.25,y,0",false},
    wyckPos{'j',4,"0.25,y,0.5",false},
    wyckPos{'k',4,"0.25,0.25,z",false},
    wyckPos{'l',4,"0.25,0.75,z",false},
    wyckPos{'m',8,"x,y,z",false}
  },

  { // 51
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0.5,0",true},
    wyckPos{'c',2,"0,0,0.5",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',2,"0.25,0,z",false},
    wyckPos{'f',2,"0.25,0.5,z",false},
    wyckPos{'g',4,"0,y,0",false},
    wyckPos{'h',4,"0,y,0.5",false},
    wyckPos{'i',4,"x,0,z",false},
    wyckPos{'j',4,"x,0.5,z",false},
    wyckPos{'k',4,"0.25,y,z",false},
    wyckPos{'l',8,"x,y,z",false}
  },

  { // 52
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',4,"0.25,0,z",false},
    wyckPos{'d',4,"x,0.25,0.25",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 53
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0,0",true},
    wyckPos{'c',2,"0.5,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0",true},
    wyckPos{'e',4,"x,0,0",false},
    wyckPos{'f',4,"x,0.5,0",false},
    wyckPos{'g',4,"0.25,y,0.25",false},
    wyckPos{'h',4,"0,y,z",false},
    wyckPos{'i',8,"x,y,z",false}
  },

  { // 54
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0.5,0",true},
    wyckPos{'c',4,"0,y,0.25",false},
    wyckPos{'d',4,"0.25,0,z",false},
    wyckPos{'e',4,"0.25,0.5,z",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 55
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',4,"x,y,0",false},
    wyckPos{'h',4,"x,y,0.5",false},
    wyckPos{'i',8,"x,y,z",false}
  },

  { // 56
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',4,"0.25,0.25,z",false},
    wyckPos{'d',4,"0.25,0.75,z",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 57
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0,0",true},
    wyckPos{'c',4,"x,0.25,0",false},
    wyckPos{'d',4,"x,y,0.25",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 58
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',4,"x,y,0",false},
    wyckPos{'h',8,"x,y,z",false}
  },

  { // 59
    wyckPos{'a',2,"0.25,0.25,z",false},
    wyckPos{'b',2,"0.25,0.75,z",false},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0,0,0.5",true},
    wyckPos{'e',4,"0.25,y,z",false},
    wyckPos{'f',4,"x,0.25,z",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 60
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0.5,0",true},
    wyckPos{'c',4,"0,y,0.25",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 61
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 62
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',4,"x,0.25,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 63
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0.5,0",true},
    wyckPos{'c',4,"0,y,0.25",false},
    wyckPos{'d',8,"0.25,0.25,0",true},
    wyckPos{'e',8,"x,0,0",false},
    wyckPos{'f',8,"0,y,z",false},
    wyckPos{'g',8,"x,y,0.25",false},
    wyckPos{'h',16,"x,y,z",false}
  },

  { // 64
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0,0",true},
    wyckPos{'c',8,"0.25,0.25,0",true},
    wyckPos{'d',8,"x,0,0",false},
    wyckPos{'e',8,"0.25,y,0.25",false},
    wyckPos{'f',8,"0,y,z",false},
    wyckPos{'g',16,"x,y,z",false}
  },

  { // 65
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0,0",true},
    wyckPos{'c',2,"0.5,0,0.5",true},
    wyckPos{'d',2,"0,0,0.5",true},
    wyckPos{'e',4,"0.25,0.25,0",true},
    wyckPos{'f',4,"0.25,0.25,0.5",true},
    wyckPos{'g',4,"x,0,0",false},
    wyckPos{'h',4,"x,0,0.5",false},
    wyckPos{'i',4,"0,y,0",false},
    wyckPos{'j',4,"0,y,0.5",false},
    wyckPos{'k',4,"0,0,z",false},
    wyckPos{'l',4,"0,0.5,z",false},
    wyckPos{'m',8,"0.25,0.25,z",false},
    wyckPos{'n',8,"0,y,z",false},
    wyckPos{'o',8,"x,0,z",false},
    wyckPos{'p',8,"x,y,0",false},
    wyckPos{'q',8,"x,y,0.5",false},
    wyckPos{'r',16,"x,y,z",false}
  },

  { // 66
    wyckPos{'a',4,"0,0,0.25",true},
    wyckPos{'b',4,"0,0.5,0.25",true},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0,0.5,0",true},
    wyckPos{'e',4,"0.25,0.25,0",true},
    wyckPos{'f',4,"0.25,0.75,0",true},
    wyckPos{'g',8,"x,0,0.25",false},
    wyckPos{'h',8,"0,y,0.25",false},
    wyckPos{'i',8,"0,0,z",false},
    wyckPos{'j',8,"0,0.5,z",false},
    wyckPos{'k',8,"0.25,0.25,z",false},
    wyckPos{'l',8,"x,y,0",false},
    wyckPos{'m',16,"x,y,z",false}
  },

  { // 67
    wyckPos{'a',4,"0.25,0,0",true},
    wyckPos{'b',4,"0.25,0,0.5",true},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0,0,0.5",true},
    wyckPos{'e',4,"0.25,0.25,0",true},
    wyckPos{'f',4,"0.25,0.25,0.5",true},
    wyckPos{'g',4,"0,0.25,z",false},
    wyckPos{'h',8,"x,0,0",false},
    wyckPos{'i',8,"x,0,0.5",false},
    wyckPos{'j',8,"0.25,y,0",false},
    wyckPos{'k',8,"0.25,y,0.5",false},
    wyckPos{'l',8,"0.25,0,z",false},
    wyckPos{'m',8,"0,y,z",false},
    wyckPos{'n',8,"x,0.25,z",false},
    wyckPos{'o',16,"x,y,z",false}
  },

  { // 68
    wyckPos{'a',4,"0,0.25,0.25",true},
    wyckPos{'b',4,"0,0.25,0.75",true},
    wyckPos{'c',8,"0.25,0.75,0",true},
    wyckPos{'d',8,"0,0,0",true},
    wyckPos{'e',8,"x,0.25,0.25",false},
    wyckPos{'f',8,"0,y,0.25",false},
    wyckPos{'g',8,"0,0.25,z",false},
    wyckPos{'h',8,"0.25,0,z",false},
    wyckPos{'i',16,"x,y,z",false}
  },

  { // 69
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',8,"0,0.25,0.25",true},
    wyckPos{'d',8,"0.25,0,0.25",true},
    wyckPos{'e',8,"0.25,0.25,0",true},
    wyckPos{'f',8,"0.25,0.25,0.25",true},
    wyckPos{'g',8,"x,0,0",false},
    wyckPos{'h',8,"0,y,0",false},
    wyckPos{'i',8,"0,0,z",false},
    wyckPos{'j',16,"0.25,0.25,z",false},
    wyckPos{'k',16,"0.25,y,0.25",false},
    wyckPos{'l',16,"x,0.25,0.25",false},
    wyckPos{'m',16,"0,y,z",false},
    wyckPos{'n',16,"x,0,z",false},
    wyckPos{'o',16,"x,y,0",false},
    wyckPos{'p',32,"x,y,z",false}
  },

  { // 70
    wyckPos{'a',8,"0.125,0.125,0.125",true},
    wyckPos{'b',8,"0.125,0.125,0.625",true},
    wyckPos{'c',16,"0,0,0",true},
    wyckPos{'d',16,"0.5,0.5,0.5",true},
    wyckPos{'e',16,"x,0.125,0.125",false},
    wyckPos{'f',16,"0.125,y,0.125",false},
    wyckPos{'g',16,"0.125,0.125,z",false},
    wyckPos{'h',32,"x,y,z",false}
  },

  { // 71
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0.5,0.5",true},
    wyckPos{'c',2,"0.5,0.5,0",true},
    wyckPos{'d',2,"0.5,0,0.5",true},
    wyckPos{'e',4,"x,0,0",false},
    wyckPos{'f',4,"x,0.5,0",false},
    wyckPos{'g',4,"0,y,0",false},
    wyckPos{'h',4,"0,y,0.5",false},
    wyckPos{'i',4,"0,0,z",false},
    wyckPos{'j',4,"0.5,0,z",false},
    wyckPos{'k',8,"0.25,0.25,0.25",true},
    wyckPos{'l',8,"0,y,z",false},
    wyckPos{'m',8,"x,0,z",false},
    wyckPos{'n',8,"x,y,0",false},
    wyckPos{'o',16,"x,y,z",false}
  },

  { // 72
    wyckPos{'a',4,"0,0,0.25",true},
    wyckPos{'b',4,"0.5,0,0.25",true},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0.5,0,0",true},
    wyckPos{'e',8,"0.25,0.25,0.25",true},
    wyckPos{'f',8,"x,0,0.25",false},
    wyckPos{'g',8,"0,y,0.25",false},
    wyckPos{'h',8,"0,0,z",false},
    wyckPos{'i',8,"0,0.5,z",false},
    wyckPos{'j',8,"x,y,0",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 73
    wyckPos{'a',8,"0,0,0",true},
    wyckPos{'b',8,"0.25,0.25,0.25",true},
    wyckPos{'c',8,"x,0,0.25",false},
    wyckPos{'d',8,"0.25,y,0",false},
    wyckPos{'e',8,"0,0.25,z",false},
    wyckPos{'f',16,"x,y,z",false}
  },

  { // 74
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.25,0.25,0.75",true},
    wyckPos{'e',4,"0,0.25,z",false},
    wyckPos{'f',8,"x,0,0",false},
    wyckPos{'g',8,"0.25,y,0.25",false},
    wyckPos{'h',8,"0,y,z",false},
    wyckPos{'i',8,"x,0.25,z",false},
    wyckPos{'j',16,"x,y,z",false}
  },

  { // 75
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',1,"0.5,0.5,z",false},
    wyckPos{'c',2,"0,0.5,z",false},
    wyckPos{'d',4,"x,y,z",false}
  },

  { // 76
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 77
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0.5,z",false},
    wyckPos{'c',2,"0,0.5,z",false},
    wyckPos{'d',4,"x,y,z",false}
  },

  { // 78
    wyckPos{'a',4,"x,y,z",false}
  },

  { // 79
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 80
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',8,"x,y,z",false}
  },

  { // 81
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.5,0.5,0",true},
    wyckPos{'d',1,"0.5,0.5,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',2,"0.5,0.5,z",false},
    wyckPos{'g',2,"0,0.5,z",false},
    wyckPos{'h',4,"x,y,z",false}
  },

  { // 82
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0.25",true},
    wyckPos{'d',2,"0,0.5,0.75",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 83
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.5,0.5,0",true},
    wyckPos{'d',1,"0.5,0.5,0.5",true},
    wyckPos{'e',2,"0,0.5,0",true},
    wyckPos{'f',2,"0,0.5,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,y,0",false},
    wyckPos{'k',4,"x,y,0.5",false},
    wyckPos{'l',8,"x,y,z",false}
  },

  { // 84
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0.5,0",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',2,"0,0,0.25",true},
    wyckPos{'f',2,"0.5,0.5,0.25",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,y,0",false},
    wyckPos{'k',8,"x,y,z",false}
  },

  { // 85
    wyckPos{'a',2,"0.25,0.75,0",true},
    wyckPos{'b',2,"0.25,0.75,0.5",true},
    wyckPos{'c',2,"0.25,0.25,z",false},
    wyckPos{'d',4,"0,0,0",true},
    wyckPos{'e',4,"0,0,0.5",true},
    wyckPos{'f',4,"0.25,0.75,z",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 86
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',2,"0.25,0.25,0.75",true},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0,0,0.5",true},
    wyckPos{'e',4,"0.75,0.25,z",false},
    wyckPos{'f',4,"0.25,0.25,z",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 87
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',8,"0.25,0.25,0.25",true},
    wyckPos{'g',8,"0,0.5,z",false},
    wyckPos{'h',8,"x,y,0",false},
    wyckPos{'i',16,"x,y,z",false}
  },

  { // 88
    wyckPos{'a',4,"0,0.25,0.125",true},
    wyckPos{'b',4,"0,0.25,0.625",true},
    wyckPos{'c',8,"0,0,0",true},
    wyckPos{'d',8,"0,0,0.5",true},
    wyckPos{'e',8,"0,0.25,z",false},
    wyckPos{'f',16,"x,y,z",false}
  },

  { // 89
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.5,0.5,0",true},
    wyckPos{'d',1,"0.5,0.5,0.5",true},
    wyckPos{'e',2,"0.5,0,0",true},
    wyckPos{'f',2,"0.5,0,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,x,0",false},
    wyckPos{'k',4,"x,x,0.5",false},
    wyckPos{'l',4,"x,0,0",false},
    wyckPos{'m',4,"x,0.5,0.5",false},
    wyckPos{'n',4,"x,0,0.5",false},
    wyckPos{'o',4,"x,0.5,0",false},
    wyckPos{'p',8,"x,y,z",false}
  },

  { // 90
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,z",false},
    wyckPos{'d',4,"0,0,z",false},
    wyckPos{'e',4,"x,x,0",false},
    wyckPos{'f',4,"x,x,0.5",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 91
    wyckPos{'a',4,"0,y,0",false},
    wyckPos{'b',4,"0.5,y,0",false},
    wyckPos{'c',4,"x,x,0.375",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 92
    wyckPos{'a',4,"x,x,0",false},
    wyckPos{'b',8,"x,y,z",false}
  },

  { // 93
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0.5,0",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',2,"0,0,0.25",true},
    wyckPos{'f',2,"0.5,0.5,0.25",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,0,0",false},
    wyckPos{'k',4,"x,0.5,0.5",false},
    wyckPos{'l',4,"x,0,0.5",false},
    wyckPos{'m',4,"x,0.5,0",false},
    wyckPos{'n',4,"x,x,0.25",false},
    wyckPos{'o',4,"x,x,0.75",false},
    wyckPos{'p',8,"x,y,z",false}
  },

  { // 94
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0,z",false},
    wyckPos{'d',4,"0,0.5,z",false},
    wyckPos{'e',4,"x,x,0",false},
    wyckPos{'f',4,"x,x,0.5",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 95
    wyckPos{'a',4,"0,y,0",false},
    wyckPos{'b',4,"0.5,y,0",false},
    wyckPos{'c',4,"x,x,0.625",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 96
    wyckPos{'a',4,"x,x,0",false},
    wyckPos{'b',8,"x,y,z",false}
  },

  { // 97
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',8,"0,0.5,z",false},
    wyckPos{'g',8,"x,x,0",false},
    wyckPos{'h',8,"x,0,0",false},
    wyckPos{'i',8,"x,0,0.5",false},
    wyckPos{'j',8,"x,x+0.5,0.25",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 98
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',8,"0,0,z",false},
    wyckPos{'d',8,"x,x,0",false},
    wyckPos{'e',8,"-x,x,0",false},
    wyckPos{'f',8,"x,0.25,0.125",false},
    wyckPos{'g',16,"x,y,z",false}
  },

  { // 99
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',1,"0.5,0.5,z",false},
    wyckPos{'c',2,"0.5,0,z",false},
    wyckPos{'d',4,"x,x,z",false},
    wyckPos{'e',4,"x,0,z",false},
    wyckPos{'f',4,"x,0.5,z",false},
    wyckPos{'g',8,"x,y,z",false}
  },

  { // 100
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0,z",false},
    wyckPos{'c',4,"x,x+0.5,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 101
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0.5,z",false},
    wyckPos{'c',4,"0,0.5,z",false},
    wyckPos{'d',4,"x,x,z",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 102
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',4,"x,x,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 103
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0.5,z",false},
    wyckPos{'c',4,"0,0.5,z",false},
    wyckPos{'d',8,"x,y,z",false}
  },

  { // 104
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 105
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.5,0.5,z",false},
    wyckPos{'c',2,"0,0.5,z",false},
    wyckPos{'d',4,"x,0,z",false},
    wyckPos{'e',4,"x,0.5,z",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 106
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',8,"x,y,z",false}
  },

  { // 107
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0,0.5,z",false},
    wyckPos{'c',8,"x,x,z",false},
    wyckPos{'d',8,"x,0,z",false},
    wyckPos{'e',16,"x,y,z",false}
  },

  { // 108
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',4,"0.5,0,z",false},
    wyckPos{'c',8,"x,x+0.5,z",false},
    wyckPos{'d',16,"x,y,z",false}
  },

  { // 109
    wyckPos{'a',4,"0,0,z",false},
    wyckPos{'b',8,"0,y,z",false},
    wyckPos{'c',16,"x,y,z",false}
  },

  { // 110
    wyckPos{'a',8,"0,0,z",false},
    wyckPos{'b',16,"x,y,z",false}
  },

  { // 111
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',1,"0,0,0.5",true},
    wyckPos{'d',1,"0.5,0.5,0",true},
    wyckPos{'e',2,"0.5,0,0",true},
    wyckPos{'f',2,"0.5,0,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.5,0.5,z",false},
    wyckPos{'i',4,"x,0,0",false},
    wyckPos{'j',4,"x,0.5,0.5",false},
    wyckPos{'k',4,"x,0,0.5",false},
    wyckPos{'l',4,"x,0.5,0",false},
    wyckPos{'m',4,"0,0.5,z",false},
    wyckPos{'n',4,"x,x,z",false},
    wyckPos{'o',8,"x,y,z",false}
  },

  { // 112
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0.5,0,0.25",true},
    wyckPos{'c',2,"0.5,0.5,0.25",true},
    wyckPos{'d',2,"0,0.5,0.25",true},
    wyckPos{'e',2,"0,0,0",true},
    wyckPos{'f',2,"0.5,0.5,0",true},
    wyckPos{'g',4,"x,0,0.25",false},
    wyckPos{'h',4,"0.5,y,0.25",false},
    wyckPos{'i',4,"x,0.5,0.25",false},
    wyckPos{'j',4,"0,y,0.25",false},
    wyckPos{'k',4,"0,0,z",false},
    wyckPos{'l',4,"0.5,0.5,z",false},
    wyckPos{'m',4,"0,0.5,z",false},
    wyckPos{'n',8,"x,y,z",false}
  },

  { // 113
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,z",false},
    wyckPos{'d',4,"0,0,z",false},
    wyckPos{'e',4,"x,x+0.5,z",false},
    wyckPos{'f',8,"x,y,z",false}
  },

  { // 114
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0,z",false},
    wyckPos{'d',4,"0,0.5,z",false},
    wyckPos{'e',8,"x,y,z",false}
  },

  { // 115
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0",true},
    wyckPos{'c',1,"0.5,0.5,0.5",true},
    wyckPos{'d',1,"0,0,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',2,"0.5,0.5,z",false},
    wyckPos{'g',2,"0,0.5,z",false},
    wyckPos{'h',4,"x,x,0",false},
    wyckPos{'i',4,"x,x,0.5",false},
    wyckPos{'j',4,"x,0,z",false},
    wyckPos{'k',4,"x,0.5,z",false},
    wyckPos{'l',8,"x,y,z",false}
  },

  { // 116
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0.5,0.5,0.25",true},
    wyckPos{'c',2,"0,0,0",true},
    wyckPos{'d',2,"0.5,0.5,0",true},
    wyckPos{'e',4,"x,x,0.25",false},
    wyckPos{'f',4,"x,x,0.75",false},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',8,"x,y,z",false}
  },

  { // 117
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',4,"x,x+0.5,0",false},
    wyckPos{'h',4,"x,x+0.5,0.5",false},
    wyckPos{'i',8,"x,y,z",false}
  },

  { // 118
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0.25",true},
    wyckPos{'d',2,"0,0.5,0.75",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"x,-x+0.5,0.25",false},
    wyckPos{'g',4,"x,x+0.5,0.25",false},
    wyckPos{'h',4,"0,0.5,z",false},
    wyckPos{'i',8,"x,y,z",false}
  },

  { // 119
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0.25",true},
    wyckPos{'d',2,"0,0.5,0.75",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',8,"x,x,0",false},
    wyckPos{'h',8,"x,x+0.5,0.25",false},
    wyckPos{'i',8,"x,0,z",false},
    wyckPos{'j',16,"x,y,z",false}
  },

  { // 120
    wyckPos{'a',4,"0,0,0.25",true},
    wyckPos{'b',4,"0,0,0",true},
    wyckPos{'c',4,"0,0.5,0.25",true},
    wyckPos{'d',4,"0,0.5,0",true},
    wyckPos{'e',8,"x,x,0.25",false},
    wyckPos{'f',8,"0,0,z",false},
    wyckPos{'g',8,"0,0.5,z",false},
    wyckPos{'h',8,"x,x+0.5,0",false},
    wyckPos{'i',16,"x,y,z",false}
  },

  { // 121
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',8,"x,0,0",false},
    wyckPos{'g',8,"x,0,0.5",false},
    wyckPos{'h',8,"0,0.5,z",false},
    wyckPos{'i',8,"x,x,z",false},
    wyckPos{'j',16,"x,y,z",false}
  },

  { // 122
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.5",true},
    wyckPos{'c',8,"0,0,z",false},
    wyckPos{'d',8,"x,0.25,0.125",false},
    wyckPos{'e',16,"x,y,z",false}
  },

  { // 123
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.5,0.5,0",true},
    wyckPos{'d',1,"0.5,0.5,0.5",true},
    wyckPos{'e',2,"0,0.5,0.5",true},
    wyckPos{'f',2,"0,0.5,0",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,x,0",false},
    wyckPos{'k',4,"x,x,0.5",false},
    wyckPos{'l',4,"x,0,0",false},
    wyckPos{'m',4,"x,0,0.5",false},
    wyckPos{'n',4,"x,0.5,0",false},
    wyckPos{'o',4,"x,0.5,0.5",false},
    wyckPos{'p',8,"x,y,0",false},
    wyckPos{'q',8,"x,y,0.5",false},
    wyckPos{'r',8,"x,x,z",false},
    wyckPos{'s',8,"x,0,z",false},
    wyckPos{'t',8,"x,0.5,z",false},
    wyckPos{'u',16,"x,y,z",false}
  },

  { // 124
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',2,"0.5,0.5,0.25",true},
    wyckPos{'d',2,"0.5,0.5,0",true},
    wyckPos{'e',4,"0,0.5,0",true},
    wyckPos{'f',4,"0,0.5,0.25",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',8,"0,0.5,z",false},
    wyckPos{'j',8,"x,x,0.25",false},
    wyckPos{'k',8,"x,0,0.25",false},
    wyckPos{'l',8,"x,0.5,0.25",false},
    wyckPos{'m',8,"x,y,0",false},
    wyckPos{'n',16,"x,y,z",false}
  },

  { // 125
    wyckPos{'a',2,"0.25,0.25,0",true},
    wyckPos{'b',2,"0.25,0.25,0.5",true},
    wyckPos{'c',2,"0.75,0.25,0",true},
    wyckPos{'d',2,"0.75,0.25,0.5",true},
    wyckPos{'e',4,"0,0,0",true},
    wyckPos{'f',4,"0,0,0.5",true},
    wyckPos{'g',4,"0.25,0.25,z",false},
    wyckPos{'h',4,"0.75,0.25,z",false},
    wyckPos{'i',8,"x,x,0",false},
    wyckPos{'j',8,"x,x,0.5",false},
    wyckPos{'k',8,"x,0.25,0",false},
    wyckPos{'l',8,"x,0.25,0.5",false},
    wyckPos{'m',8,"x,-x,z",false},
    wyckPos{'n',16,"x,y,z",false}
  },

  { // 126
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',2,"0.25,0.25,0.75",true},
    wyckPos{'c',4,"0.25,0.75,0.75",true},
    wyckPos{'d',4,"0.25,0.75,0",true},
    wyckPos{'e',4,"0.25,0.25,z",false},
    wyckPos{'f',8,"0,0,0",true},
    wyckPos{'g',8,"0.25,0.75,z",false},
    wyckPos{'h',8,"x,x,0.25",false},
    wyckPos{'i',8,"x,0.25,0.25",false},
    wyckPos{'j',8,"x,0.75,0.25",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 127
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',2,"0,0.5,0.5",true},
    wyckPos{'d',2,"0,0.5,0",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0,0.5,z",false},
    wyckPos{'g',4,"x,x+0.5,0",false},
    wyckPos{'h',4,"x,x+0.5,0.5",false},
    wyckPos{'i',8,"x,y,0",false},
    wyckPos{'j',8,"x,y,0.5",false},
    wyckPos{'k',8,"x,x+0.5,z",false},
    wyckPos{'l',16,"x,y,z",false}
  },

  { // 128
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',8,"0,0.5,z",false},
    wyckPos{'g',8,"x,x+0.5,0.25",false},
    wyckPos{'h',8,"x,y,0",false},
    wyckPos{'i',16,"x,y,z",false}
  },

  { // 129
    wyckPos{'a',2,"0.75,0.25,0",true},
    wyckPos{'b',2,"0.75,0.25,0.5",true},
    wyckPos{'c',2,"0.25,0.25,z",false},
    wyckPos{'d',4,"0,0,0",true},
    wyckPos{'e',4,"0,0,0.5",true},
    wyckPos{'f',4,"0.75,0.25,z",false},
    wyckPos{'g',8,"x,-x,0",false},
    wyckPos{'h',8,"x,-x,0.5",false},
    wyckPos{'i',8,"0.25,y,z",false},
    wyckPos{'j',8,"x,x,z",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 130
    wyckPos{'a',4,"0.75,0.25,0.25",true},
    wyckPos{'b',4,"0.75,0.25,0",true},
    wyckPos{'c',4,"0.25,0.25,z",false},
    wyckPos{'d',8,"0,0,0",true},
    wyckPos{'e',8,"0.75,0.25,z",false},
    wyckPos{'f',8,"x,-x,0.25",false},
    wyckPos{'g',16,"x,y,z",false}
  },

  { // 131
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0.5,0.5,0",true},
    wyckPos{'c',2,"0,0.5,0",true},
    wyckPos{'d',2,"0,0.5,0.5",true},
    wyckPos{'e',2,"0,0,0.25",true},
    wyckPos{'f',2,"0.5,0.5,0.25",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',4,"0,0.5,z",false},
    wyckPos{'j',4,"x,0,0",false},
    wyckPos{'k',4,"x,0.5,0.5",false},
    wyckPos{'l',4,"x,0,0.5",false},
    wyckPos{'m',4,"x,0.5,0",false},
    wyckPos{'n',8,"x,x,0.25",false},
    wyckPos{'o',8,"0,y,z",false},
    wyckPos{'p',8,"0.5,y,z",false},
    wyckPos{'q',8,"x,y,0",false},
    wyckPos{'r',16,"x,y,z",false}
  },

  { // 132
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.25",true},
    wyckPos{'c',2,"0.5,0.5,0",true},
    wyckPos{'d',2,"0.5,0.5,0.25",true},
    wyckPos{'e',4,"0,0.5,0.25",true},
    wyckPos{'f',4,"0,0.5,0",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.5,0.5,z",false},
    wyckPos{'i',4,"x,x,0",false},
    wyckPos{'j',4,"x,x,0.5",false},
    wyckPos{'k',8,"0,0.5,z",false},
    wyckPos{'l',8,"x,0,0.25",false},
    wyckPos{'m',8,"x,0.5,0.25",false},
    wyckPos{'n',8,"x,y,0",false},
    wyckPos{'o',8,"x,x,z",false},
    wyckPos{'p',16,"x,y,z",false}
  },

  { // 133
    wyckPos{'a',4,"0.25,0.25,0",true},
    wyckPos{'b',4,"0.75,0.25,0",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.75,0.25,0.75",true},
    wyckPos{'e',8,"0,0,0",true},
    wyckPos{'f',8,"0.25,0.25,z",false},
    wyckPos{'g',8,"0.75,0.25,z",false},
    wyckPos{'h',8,"x,0.25,0",false},
    wyckPos{'i',8,"x,0.25,0.5",false},
    wyckPos{'j',8,"x,x,0.25",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 134
    wyckPos{'a',2,"0.25,0.75,0.25",true},
    wyckPos{'b',2,"0.75,0.25,0.25",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.25,0.25,0",true},
    wyckPos{'e',4,"0,0,0.5",true},
    wyckPos{'f',4,"0,0,0",true},
    wyckPos{'g',4,"0.75,0.25,z",false},
    wyckPos{'h',8,"0.25,0.25,z",false},
    wyckPos{'i',8,"x,0.25,0.75",false},
    wyckPos{'j',8,"x,0.25,0.25",false},
    wyckPos{'k',8,"x,x,0",false},
    wyckPos{'l',8,"x,x,0.5",false},
    wyckPos{'m',8,"x,-x,z",false},
    wyckPos{'n',16,"x,y,z",false}
  },

  { // 135
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0,0,0.25",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',8,"0,0,z",false},
    wyckPos{'f',8,"0,0.5,z",false},
    wyckPos{'g',8,"x,x+0.5,0.25",false},
    wyckPos{'h',8,"x,y,0",false},
    wyckPos{'i',16,"x,y,z",false}
  },

  { // 136
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"x,x,0",false},
    wyckPos{'g',4,"x,-x,0",false},
    wyckPos{'h',8,"0,0.5,z",false},
    wyckPos{'i',8,"x,y,0",false},
    wyckPos{'j',8,"x,x,z",false},
    wyckPos{'k',16,"x,y,z",false}
  },

  { // 137
    wyckPos{'a',2,"0.75,0.25,0.75",true},
    wyckPos{'b',2,"0.75,0.25,0.25",true},
    wyckPos{'c',4,"0.75,0.25,z",false},
    wyckPos{'d',4,"0.25,0.25,z",false},
    wyckPos{'e',8,"0,0,0",true},
    wyckPos{'f',8,"x,-x,0.25",false},
    wyckPos{'g',8,"0.25,y,z",false},
    wyckPos{'h',16,"x,y,z",false}
  },

  { // 138
    wyckPos{'a',4,"0.75,0.25,0",true},
    wyckPos{'b',4,"0.75,0.25,0.75",true},
    wyckPos{'c',4,"0,0,0.5",true},
    wyckPos{'d',4,"0,0,0",true},
    wyckPos{'e',4,"0.25,0.25,z",false},
    wyckPos{'f',8,"0.75,0.25,z",false},
    wyckPos{'g',8,"x,-x,0.5",false},
    wyckPos{'h',8,"x,-x,0",false},
    wyckPos{'i',8,"x,x,z",false},
    wyckPos{'j',16,"x,y,z",false}
  },

  { // 139
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.5",true},
    wyckPos{'c',4,"0,0.5,0",true},
    wyckPos{'d',4,"0,0.5,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',8,"0.25,0.25,0.25",true},
    wyckPos{'g',8,"0,0.5,z",false},
    wyckPos{'h',8,"x,x,0",false},
    wyckPos{'i',8,"x,0,0",false},
    wyckPos{'j',8,"x,0.5,0",false},
    wyckPos{'k',16,"x,x+0.5,0.25",false},
    wyckPos{'l',16,"x,y,0",false},
    wyckPos{'m',16,"x,x,z",false},
    wyckPos{'n',16,"0,y,z",false},
    wyckPos{'o',32,"x,y,z",false}
  },

  { // 140
    wyckPos{'a',4,"0,0,0.25",true},
    wyckPos{'b',4,"0,0.5,0.25",true},
    wyckPos{'c',4,"0,0,0",true},
    wyckPos{'d',4,"0,0.5,0",true},
    wyckPos{'e',8,"0.25,0.25,0.25",true},
    wyckPos{'f',8,"0,0,z",false},
    wyckPos{'g',8,"0,0.5,z",false},
    wyckPos{'h',8,"x,x+0.5,0",false},
    wyckPos{'i',16,"x,x,0.25",false},
    wyckPos{'j',16,"x,0,0.25",false},
    wyckPos{'k',16,"x,y,0",false},
    wyckPos{'l',16,"x,x+0.5,z",false},
    wyckPos{'m',32,"x,y,z",false}
  },

  { // 141
    wyckPos{'a',4,"0,0.75,0.125",true},
    wyckPos{'b',4,"0,0.25,0.375",true},
    wyckPos{'c',8,"0,0,0",true},
    wyckPos{'d',8,"0,0,0.5",true},
    wyckPos{'e',8,"0,0.25,z",false},
    wyckPos{'f',16,"x,0,0",false},
    wyckPos{'g',16,"x,x+0.25,0.875",false},
    wyckPos{'h',16,"0,y,z",false},
    wyckPos{'i',32,"x,y,z",false}
  },

  { // 142
    wyckPos{'a',8,"0,0.25,0.375",true},
    wyckPos{'b',8,"0,0.25,0.125",true},
    wyckPos{'c',16,"0,0,0",true},
    wyckPos{'d',16,"0,0.25,z",false},
    wyckPos{'e',16,"x,0,0.25",false},
    wyckPos{'f',16,"x,x+0.25,0.125",false},
    wyckPos{'g',32,"x,y,z",false}
  },

  { // 143
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',1,"0.333333,0.666667,z",false},
    wyckPos{'c',1,"0.666667,0.333333,z",false},
    wyckPos{'d',3,"x,y,z",false}
  },

  { // 144
    wyckPos{'a',3,"x,y,z",false}
  },

  { // 145
    wyckPos{'a',3,"x,y,z",false}
  },

  { // 146
    wyckPos{'a',3,"0,0,z",false},
    wyckPos{'b',9,"x,y,z",false}
  },

  { // 147
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0,0,z",false},
    wyckPos{'d',2,"0.333333,0.666667,z",false},
    wyckPos{'e',3,"0.5,0,0",true},
    wyckPos{'f',3,"0.5,0,0.5",true},
    wyckPos{'g',6,"x,y,z",false}
  },

  { // 148
    wyckPos{'a',3,"0,0,0",true},
    wyckPos{'b',3,"0,0,0.5",true},
    wyckPos{'c',6,"0,0,z",false},
    wyckPos{'d',9,"0.5,0,0.5",true},
    wyckPos{'e',9,"0.5,0,0",true},
    wyckPos{'f',18,"x,y,z",false}
  },

  { // 149
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.333333,0.666667,0",true},
    wyckPos{'d',1,"0.333333,0.666667,0.5",true},
    wyckPos{'e',1,"0.666667,0.333333,0",true},
    wyckPos{'f',1,"0.666667,0.333333,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.333333,0.666667,z",false},
    wyckPos{'i',2,"0.666667,0.333333,z",false},
    wyckPos{'j',3,"x,-x,0",false},
    wyckPos{'k',3,"x,-x,0.5",false},
    wyckPos{'l',6,"x,y,z",false}
  },

  { // 150
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0,0,z",false},
    wyckPos{'d',2,"0.333333,0.666667,z",false},
    wyckPos{'e',3,"x,0,0",false},
    wyckPos{'f',3,"x,0,0.5",false},
    wyckPos{'g',6,"x,y,z",false}
  },

  { // 151
    wyckPos{'a',3,"x,-x,0.333333",false},
    wyckPos{'b',3,"x,-x,0.833333",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 152
    wyckPos{'a',3,"x,0,0.333333",false},
    wyckPos{'b',3,"x,0,0.833333",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 153
    wyckPos{'a',3,"x,-x,0.666667",false},
    wyckPos{'b',3,"x,-x,0.166667",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 154
    wyckPos{'a',3,"x,0,0.666667",false},
    wyckPos{'b',3,"x,0,0.166667",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 155
    wyckPos{'a',3,"0,0,0",true},
    wyckPos{'b',3,"0,0,0.5",true},
    wyckPos{'c',6,"0,0,z",false},
    wyckPos{'d',9,"x,0,0",false},
    wyckPos{'e',9,"x,0,0.5",false},
    wyckPos{'f',18,"x,y,z",false}
  },

  { // 156
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',1,"0.333333,0.666667,z",false},
    wyckPos{'c',1,"0.666667,0.333333,z",false},
    wyckPos{'d',3,"x,-x,z",false},
    wyckPos{'e',6,"x,y,z",false}
  },

  { // 157
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',3,"x,0,z",false},
    wyckPos{'d',6,"x,y,z",false}
  },

  { // 158
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',2,"0.666667,0.333333,z",false},
    wyckPos{'d',6,"x,y,z",false}
  },

  { // 159
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 160
    wyckPos{'a',3,"0,0,z",false},
    wyckPos{'b',9,"x,-x,z",false},
    wyckPos{'c',18,"x,y,z",false}
  },

  { // 161
    wyckPos{'a',6,"0,0,z",false},
    wyckPos{'b',18,"x,y,z",false}
  },

  { // 162
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',3,"0.5,0,0",true},
    wyckPos{'g',3,"0.5,0,0.5",true},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',6,"x,-x,0",false},
    wyckPos{'j',6,"x,-x,0.5",false},
    wyckPos{'k',6,"x,0,z",false},
    wyckPos{'l',12,"x,y,z",false}
  },

  { // 163
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',2,"0.333333,0.666667,0.25",true},
    wyckPos{'d',2,"0.666667,0.333333,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0.333333,0.666667,z",false},
    wyckPos{'g',6,"0.5,0,0",true},
    wyckPos{'h',6,"x,-x,0.25",false},
    wyckPos{'i',12,"x,y,z",false}
  },

  { // 164
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0,0,z",false},
    wyckPos{'d',2,"0.333333,0.666667,z",false},
    wyckPos{'e',3,"0.5,0,0",true},
    wyckPos{'f',3,"0.5,0,0.5",true},
    wyckPos{'g',6,"x,0,0",false},
    wyckPos{'h',6,"x,0,0.5",false},
    wyckPos{'i',6,"x,-x,z",false},
    wyckPos{'j',12,"x,y,z",false}
  },

  { // 165
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',4,"0,0,z",false},
    wyckPos{'d',4,"0.333333,0.666667,z",false},
    wyckPos{'e',6,"0.5,0,0",true},
    wyckPos{'f',6,"x,0,0.25",false},
    wyckPos{'g',12,"x,y,z",false}
  },

  { // 166
    wyckPos{'a',3,"0,0,0",true},
    wyckPos{'b',3,"0,0,0.5",true},
    wyckPos{'c',6,"0,0,z",false},
    wyckPos{'d',9,"0.5,0,0.5",true},
    wyckPos{'e',9,"0.5,0,0",true},
    wyckPos{'f',18,"x,0,0",false},
    wyckPos{'g',18,"x,0,0.5",false},
    wyckPos{'h',18,"x,-x,z",false},
    wyckPos{'i',36,"x,y,z",false}
  },

  { // 167
    wyckPos{'a',6,"0,0,0.25",true},
    wyckPos{'b',6,"0,0,0",true},
    wyckPos{'c',12,"0,0,z",false},
    wyckPos{'d',18,"0.5,0,0",true},
    wyckPos{'e',18,"x,0,0.25",false},
    wyckPos{'f',36,"x,y,z",false}
  },

  { // 168
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',3,"0.5,0,z",false},
    wyckPos{'d',6,"x,y,z",false}
  },

  { // 169
    wyckPos{'a',6,"x,y,z",false}
  },

  { // 170
    wyckPos{'a',6,"x,y,z",false}
  },

  { // 171
    wyckPos{'a',3,"0,0,z",false},
    wyckPos{'b',3,"0.5,0.5,z",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 172
    wyckPos{'a',3,"0,0,z",false},
    wyckPos{'b',3,"0.5,0.5,z",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 173
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',6,"x,y,z",false}
  },

  { // 174
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.333333,0.666667,0",true},
    wyckPos{'d',1,"0.333333,0.666667,0.5",true},
    wyckPos{'e',1,"0.666667,0.333333,0",true},
    wyckPos{'f',1,"0.666667,0.333333,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.333333,0.666667,z",false},
    wyckPos{'i',2,"0.666667,0.333333,z",false},
    wyckPos{'j',3,"x,y,0",false},
    wyckPos{'k',3,"x,y,0.5",false},
    wyckPos{'l',6,"x,y,z",false}
  },

  { // 175
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',3,"0.5,0,0",true},
    wyckPos{'g',3,"0.5,0,0.5",true},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',6,"0.5,0,z",false},
    wyckPos{'j',6,"x,y,0",false},
    wyckPos{'k',6,"x,y,0.5",false},
    wyckPos{'l',12,"x,y,z",false}
  },

  { // 176
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',2,"0.333333,0.666667,0.25",true},
    wyckPos{'d',2,"0.666667,0.333333,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0.333333,0.666667,z",false},
    wyckPos{'g',6,"0.5,0,0",true},
    wyckPos{'h',6,"x,y,0.25",false},
    wyckPos{'i',12,"x,y,z",false}
  },

  { // 177
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',3,"0.5,0,0",true},
    wyckPos{'g',3,"0.5,0,0.5",true},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',6,"0.5,0,z",false},
    wyckPos{'j',6,"x,0,0",false},
    wyckPos{'k',6,"x,0,0.5",false},
    wyckPos{'l',6,"x,-x,0",false},
    wyckPos{'m',6,"x,-x,0.5",false},
    wyckPos{'n',12,"x,y,z",false}
  },

  { // 178
    wyckPos{'a',6,"x,0,0",false},
    wyckPos{'b',6,"x,2x,0.25",false},
    wyckPos{'c',12,"x,y,z",false}
  },

  { // 179
    wyckPos{'a',6,"x,0,0",false},
    wyckPos{'b',6,"x,2x,0.75",false},
    wyckPos{'c',12,"x,y,z",false}
  },

  { // 180
    wyckPos{'a',3,"0,0,0",true},
    wyckPos{'b',3,"0,0,0.5",true},
    wyckPos{'c',3,"0.5,0,0",true},
    wyckPos{'d',3,"0.5,0,0.5",true},
    wyckPos{'e',6,"0,0,z",false},
    wyckPos{'f',6,"0.5,0,z",false},
    wyckPos{'g',6,"x,0,0",false},
    wyckPos{'h',6,"x,0,0.5",false},
    wyckPos{'i',6,"x,2x,0",false},
    wyckPos{'j',6,"x,2x,0.5",false},
    wyckPos{'k',12,"x,y,z",false}
  },

  { // 181
    wyckPos{'a',3,"0,0,0",true},
    wyckPos{'b',3,"0,0,0.5",true},
    wyckPos{'c',3,"0.5,0,0",true},
    wyckPos{'d',3,"0.5,0,0.5",true},
    wyckPos{'e',6,"0,0,z",false},
    wyckPos{'f',6,"0.5,0,z",false},
    wyckPos{'g',6,"x,0,0",false},
    wyckPos{'h',6,"x,0,0.5",false},
    wyckPos{'i',6,"x,2x,0",false},
    wyckPos{'j',6,"x,2x,0.5",false},
    wyckPos{'k',12,"x,y,z",false}
  },

  { // 182
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.25",true},
    wyckPos{'c',2,"0.333333,0.666667,0.25",true},
    wyckPos{'d',2,"0.333333,0.666667,0.75",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0.333333,0.666667,z",false},
    wyckPos{'g',6,"x,0,0",false},
    wyckPos{'h',6,"x,2x,0.25",false},
    wyckPos{'i',12,"x,y,z",false}
  },

  { // 183
    wyckPos{'a',1,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',3,"0.5,0,z",false},
    wyckPos{'d',6,"x,0,z",false},
    wyckPos{'e',6,"x,-x,z",false},
    wyckPos{'f',12,"x,y,z",false}
  },

  { // 184
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0.333333,0.666667,z",false},
    wyckPos{'c',6,"0.5,0,z",false},
    wyckPos{'d',12,"x,y,z",false}
  },

  { // 185
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',4,"0.333333,0.666667,z",false},
    wyckPos{'c',6,"x,0,z",false},
    wyckPos{'d',12,"x,y,z",false}
  },

  { // 186
    wyckPos{'a',2,"0,0,z",false},
    wyckPos{'b',2,"0.333333,0.666667,z",false},
    wyckPos{'c',6,"x,-x,z",false},
    wyckPos{'d',12,"x,y,z",false}
  },

  { // 187
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',1,"0.333333,0.666667,0",true},
    wyckPos{'d',1,"0.333333,0.666667,0.5",true},
    wyckPos{'e',1,"0.666667,0.333333,0",true},
    wyckPos{'f',1,"0.666667,0.333333,0.5",true},
    wyckPos{'g',2,"0,0,z",false},
    wyckPos{'h',2,"0.333333,0.666667,z",false},
    wyckPos{'i',2,"0.666667,0.333333,z",false},
    wyckPos{'j',3,"x,-x,0",false},
    wyckPos{'k',3,"x,-x,0.5",false},
    wyckPos{'l',6,"x,y,0",false},
    wyckPos{'m',6,"x,y,0.5",false},
    wyckPos{'n',6,"x,-x,z",false},
    wyckPos{'o',12,"x,y,z",false}
  },

  { // 188
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.25",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.25",true},
    wyckPos{'e',2,"0.666667,0.333333,0",true},
    wyckPos{'f',2,"0.666667,0.333333,0.25",true},
    wyckPos{'g',4,"0,0,z",false},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',4,"0.666667,0.333333,z",false},
    wyckPos{'j',6,"x,-x,0",false},
    wyckPos{'k',6,"x,y,0.25",false},
    wyckPos{'l',12,"x,y,z",false}
  },

  { // 189
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',3,"x,0,0",false},
    wyckPos{'g',3,"x,0,0.5",false},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',6,"x,0,z",false},
    wyckPos{'j',6,"x,y,0",false},
    wyckPos{'k',6,"x,y,0.5",false},
    wyckPos{'l',12,"x,y,z",false}
  },

  { // 190
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.25",true},
    wyckPos{'c',2,"0.333333,0.666667,0.25",true},
    wyckPos{'d',2,"0.666667,0.333333,0.25",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0.333333,0.666667,z",false},
    wyckPos{'g',6,"x,0,0",false},
    wyckPos{'h',6,"x,y,0.25",false},
    wyckPos{'i',12,"x,y,z",false}
  },

  { // 191
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0,0,0.5",true},
    wyckPos{'c',2,"0.333333,0.666667,0",true},
    wyckPos{'d',2,"0.333333,0.666667,0.5",true},
    wyckPos{'e',2,"0,0,z",false},
    wyckPos{'f',3,"0.5,0,0",true},
    wyckPos{'g',3,"0.5,0,0.5",true},
    wyckPos{'h',4,"0.333333,0.666667,z",false},
    wyckPos{'i',6,"0.5,0,z",false},
    wyckPos{'j',6,"x,0,0",false},
    wyckPos{'k',6,"x,0,0.5",false},
    wyckPos{'l',6,"x,2x,0",false},
    wyckPos{'m',6,"x,2x,0.5",false},
    wyckPos{'n',12,"x,0,z",false},
    wyckPos{'o',12,"x,2x,z",false},
    wyckPos{'p',12,"x,y,0",false},
    wyckPos{'q',12,"x,y,0.5",false},
    wyckPos{'r',24,"x,y,z",false}
  },

  { // 192
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',4,"0.333333,0.666667,0.25",true},
    wyckPos{'d',4,"0.333333,0.666667,0",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',6,"0.5,0,0.25",true},
    wyckPos{'g',6,"0.5,0,0",true},
    wyckPos{'h',8,"0.333333,0.666667,z",false},
    wyckPos{'i',12,"0.5,0,z",false},
    wyckPos{'j',12,"x,0,0.25",false},
    wyckPos{'k',12,"x,2x,0.25",false},
    wyckPos{'l',12,"x,y,0",false},
    wyckPos{'m',24,"x,y,z",false}
  },

  { // 193
    wyckPos{'a',2,"0,0,0.25",true},
    wyckPos{'b',2,"0,0,0",true},
    wyckPos{'c',4,"0.333333,0.666667,0.25",true},
    wyckPos{'d',4,"0.333333,0.666667,0",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',6,"0.5,0,0",true},
    wyckPos{'g',6,"x,0,0.25",false},
    wyckPos{'h',8,"0.333333,0.666667,z",false},
    wyckPos{'i',12,"x,2x,0",false},
    wyckPos{'j',12,"x,y,0.25",false},
    wyckPos{'k',12,"x,0,z",false},
    wyckPos{'l',24,"x,y,z",false}
  },

  { // 194
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',2,"0,0,0.25",true},
    wyckPos{'c',2,"0.333333,0.666667,0.25",true},
    wyckPos{'d',2,"0.333333,0.666667,0.75",true},
    wyckPos{'e',4,"0,0,z",false},
    wyckPos{'f',4,"0.333333,0.666667,z",false},
    wyckPos{'g',6,"0.5,0,0",true},
    wyckPos{'h',6,"x,2x,0.25",false},
    wyckPos{'i',12,"x,0,0",false},
    wyckPos{'j',12,"x,y,0.25",false},
    wyckPos{'k',12,"x,2x,z",false},
    wyckPos{'l',24,"x,y,z",false}
  },

  { // 195
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',3,"0,0.5,0.5",true},
    wyckPos{'d',3,"0.5,0,0",true},
    wyckPos{'e',4,"x,x,x",false},
    wyckPos{'f',6,"x,0,0",false},
    wyckPos{'g',6,"x,0,0.5",false},
    wyckPos{'h',6,"x,0.5,0",false},
    wyckPos{'i',6,"x,0.5,0.5",false},
    wyckPos{'j',12,"x,y,z",false}
  },

  { // 196
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.75,0.75,0.75",true},
    wyckPos{'e',16,"x,x,x",false},
    wyckPos{'f',24,"x,0,0",false},
    wyckPos{'g',24,"x,0.25,0.25",false},
    wyckPos{'h',48,"x,y,z",false}
  },

  { // 197
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',8,"x,x,x",false},
    wyckPos{'d',12,"x,0,0",false},
    wyckPos{'e',12,"x,0.5,0",false},
    wyckPos{'f',24,"x,y,z",false}
  },

  { // 198
    wyckPos{'a',4,"x,x,x",false},
    wyckPos{'b',12,"x,y,z",false}
  },

  { // 199
    wyckPos{'a',8,"x,x,x",false},
    wyckPos{'b',12,"x,0,0.25",false},
    wyckPos{'c',24,"x,y,z",false}
  },

  { // 200
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',3,"0,0.5,0.5",true},
    wyckPos{'d',3,"0.5,0,0",true},
    wyckPos{'e',6,"x,0,0",false},
    wyckPos{'f',6,"x,0,0.5",false},
    wyckPos{'g',6,"x,0.5,0",false},
    wyckPos{'h',6,"x,0.5,0.5",false},
    wyckPos{'i',8,"x,x,x",false},
    wyckPos{'j',12,"0,y,z",false},
    wyckPos{'k',12,"0.5,y,z",false},
    wyckPos{'l',24,"x,y,z",false}
  },

  { // 201
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',4,"0,0,0",true},
    wyckPos{'c',4,"0.5,0.5,0.5",true},
    wyckPos{'d',6,"0.25,0.75,0.75",true},
    wyckPos{'e',8,"x,x,x",false},
    wyckPos{'f',12,"x,0.25,0.25",false},
    wyckPos{'g',12,"x,0.75,0.25",false},
    wyckPos{'h',24,"x,y,z",false}
  },

  { // 202
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',24,"0,0.25,0.25",true},
    wyckPos{'e',24,"x,0,0",false},
    wyckPos{'f',32,"x,x,x",false},
    wyckPos{'g',48,"x,0.25,0.25",false},
    wyckPos{'h',48,"0,y,z",false},
    wyckPos{'i',96,"x,y,z",false}
  },

  { // 203
    wyckPos{'a',8,"0.125,0.125,0.125",true},
    wyckPos{'b',8,"0.625,0.625,0.625",true},
    wyckPos{'c',16,"0,0,0",true},
    wyckPos{'d',16,"0.5,0.5,0.5",true},
    wyckPos{'e',32,"x,x,x",false},
    wyckPos{'f',48,"x,0.125,0.125",false},
    wyckPos{'g',96,"x,y,z",false}
  },

  { // 204
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',12,"x,0,0",false},
    wyckPos{'e',12,"x,0,0.5",false},
    wyckPos{'f',16,"x,x,x",false},
    wyckPos{'g',24,"0,y,z",false},
    wyckPos{'h',48,"x,y,z",false}
  },

  { // 205
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',8,"x,x,x",false},
    wyckPos{'d',24,"x,y,z",false}
  },

  { // 206
    wyckPos{'a',8,"0,0,0",true},
    wyckPos{'b',8,"0.25,0.25,0.25",true},
    wyckPos{'c',16,"x,x,x",false},
    wyckPos{'d',24,"x,0,0.25",false},
    wyckPos{'e',48,"x,y,z",false}
  },

  { // 207
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',3,"0,0.5,0.5",true},
    wyckPos{'d',3,"0.5,0,0",true},
    wyckPos{'e',6,"x,0,0",false},
    wyckPos{'f',6,"x,0.5,0.5",false},
    wyckPos{'g',8,"x,x,x",false},
    wyckPos{'h',12,"x,0.5,0",false},
    wyckPos{'i',12,"0,y,y",false},
    wyckPos{'j',12,"0.5,y,y",false},
    wyckPos{'k',24,"x,y,z",false}
  },

  { // 208
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',4,"0.25,0.25,0.25",true},
    wyckPos{'c',4,"0.75,0.75,0.75",true},
    wyckPos{'d',6,"0,0.5,0.5",true},
    wyckPos{'e',6,"0.25,0,0.5",true},
    wyckPos{'f',6,"0.25,0.5,0",true},
    wyckPos{'g',8,"x,x,x",false},
    wyckPos{'h',12,"x,0,0",false},
    wyckPos{'i',12,"x,0,0.5",false},
    wyckPos{'j',12,"x,0.5,0",false},
    wyckPos{'k',12,"0.25,y,-y+0.5",false},
    wyckPos{'l',12,"0.25,y,y+0.5",false},
    wyckPos{'m',24,"x,y,z",false}
  },

  { // 209
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',24,"0,0.25,0.25",true},
    wyckPos{'e',24,"x,0,0",false},
    wyckPos{'f',32,"x,x,x",false},
    wyckPos{'g',48,"0,y,y",false},
    wyckPos{'h',48,"0.5,y,y",false},
    wyckPos{'i',48,"x,0.25,0.25",false},
    wyckPos{'j',96,"x,y,z",false}
  },

  { // 210
    wyckPos{'a',8,"0,0,0",true},
    wyckPos{'b',8,"0.5,0.5,0.5",true},
    wyckPos{'c',16,"0.125,0.125,0.125",true},
    wyckPos{'d',16,"0.625,0.625,0.625",true},
    wyckPos{'e',32,"x,x,x",false},
    wyckPos{'f',48,"x,0,0",false},
    wyckPos{'g',48,"0.125,y,-y+0.25",false},
    wyckPos{'h',96,"x,y,z",false}
  },

  { // 211
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',12,"0.25,0.5,0",true},
    wyckPos{'e',12,"x,0,0",false},
    wyckPos{'f',16,"x,x,x",false},
    wyckPos{'g',24,"x,0.5,0",false},
    wyckPos{'h',24,"0,y,y",false},
    wyckPos{'i',24,"0.25,y,-y+0.5",false},
    wyckPos{'j',48,"x,y,z",false}
  },

  { // 212
    wyckPos{'a',4,"0.125,0.125,0.125",true},
    wyckPos{'b',4,"0.625,0.625,0.625",true},
    wyckPos{'c',8,"x,x,x",false},
    wyckPos{'d',12,"0.125,y,-y+0.25",false},
    wyckPos{'e',24,"x,y,z",false}
  },

  { // 213
    wyckPos{'a',4,"0.375,0.375,0.375",true},
    wyckPos{'b',4,"0.875,0.875,0.875",true},
    wyckPos{'c',8,"x,x,x",false},
    wyckPos{'d',12,"0.125,y,y+0.25",false},
    wyckPos{'e',24,"x,y,z",false}
  },

  { // 214
    wyckPos{'a',8,"0.125,0.125,0.125",true},
    wyckPos{'b',8,"0.875,0.875,0.875",true},
    wyckPos{'c',12,"0.125,0,0.25",true},
    wyckPos{'d',12,"0.625,0,0.25",true},
    wyckPos{'e',16,"x,x,x",false},
    wyckPos{'f',24,"x,0,0.25",false},
    wyckPos{'g',24,"0.125,y,y+0.25",false},
    wyckPos{'h',24,"0.125,y,-y+0.25",false},
    wyckPos{'i',48,"x,y,z",false}
  },

  { // 215
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',3,"0,0.5,0.5",true},
    wyckPos{'d',3,"0.5,0,0",true},
    wyckPos{'e',4,"x,x,x",false},
    wyckPos{'f',6,"x,0,0",false},
    wyckPos{'g',6,"x,0.5,0.5",false},
    wyckPos{'h',12,"x,0.5,0",false},
    wyckPos{'i',12,"x,x,z",false},
    wyckPos{'j',24,"x,y,z",false}
  },

  { // 216
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',4,"0.25,0.25,0.25",true},
    wyckPos{'d',4,"0.75,0.75,0.75",true},
    wyckPos{'e',16,"x,x,x",false},
    wyckPos{'f',24,"x,0,0",false},
    wyckPos{'g',24,"x,0.25,0.25",false},
    wyckPos{'h',48,"x,x,z",false},
    wyckPos{'i',96,"x,y,z",false}
  },

  { // 217
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',8,"x,x,x",false},
    wyckPos{'d',12,"0.25,0.5,0",true},
    wyckPos{'e',12,"x,0,0",false},
    wyckPos{'f',24,"x,0.5,0",false},
    wyckPos{'g',24,"x,x,z",false},
    wyckPos{'h',48,"x,y,z",false}
  },

  { // 218
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',6,"0.25,0.5,0",true},
    wyckPos{'d',6,"0.25,0,0.5",true},
    wyckPos{'e',8,"x,x,x",false},
    wyckPos{'f',12,"x,0,0",false},
    wyckPos{'g',12,"x,0.5,0",false},
    wyckPos{'h',12,"x,0,0.5",false},
    wyckPos{'i',24,"x,y,z",false}
  },

  { // 219
    wyckPos{'a',8,"0,0,0",true},
    wyckPos{'b',8,"0.25,0.25,0.25",true},
    wyckPos{'c',24,"0,0.25,0.25",true},
    wyckPos{'d',24,"0.25,0,0",true},
    wyckPos{'e',32,"x,x,x",false},
    wyckPos{'f',48,"x,0,0",false},
    wyckPos{'g',48,"x,0.25,0.25",false},
    wyckPos{'h',96,"x,y,z",false}
  },

  { // 220
    wyckPos{'a',12,"0.375,0,0.25",true},
    wyckPos{'b',12,"0.875,0,0.25",true},
    wyckPos{'c',16,"x,x,x",false},
    wyckPos{'d',24,"x,0,0.25",false},
    wyckPos{'e',48,"x,y,z",false}
  },

  { // 221
    wyckPos{'a',1,"0,0,0",true},
    wyckPos{'b',1,"0.5,0.5,0.5",true},
    wyckPos{'c',3,"0,0.5,0.5",true},
    wyckPos{'d',3,"0.5,0,0",true},
    wyckPos{'e',6,"x,0,0",false},
    wyckPos{'f',6,"x,0.5,0.5",false},
    wyckPos{'g',8,"x,x,x",false},
    wyckPos{'h',12,"x,0.5,0",false},
    wyckPos{'i',12,"0,y,y",false},
    wyckPos{'j',12,"0.5,y,y",false},
    wyckPos{'k',24,"0,y,z",false},
    wyckPos{'l',24,"0.5,y,z",false},
    wyckPos{'m',24,"x,x,z",false},
    wyckPos{'n',48,"x,y,z",false}
  },

  { // 222
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',6,"0.75,0.25,0.25",true},
    wyckPos{'c',8,"0,0,0",true},
    wyckPos{'d',12,"0,0.75,0.25",true},
    wyckPos{'e',12,"x,0.25,0.25",false},
    wyckPos{'f',16,"x,x,x",false},
    wyckPos{'g',24,"x,0.75,0.25",false},
    wyckPos{'h',24,"0.25,y,y",false},
    wyckPos{'i',48,"x,y,z",false}
  },

  { // 223
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',6,"0.25,0,0.5",true},
    wyckPos{'d',6,"0.25,0.5,0",true},
    wyckPos{'e',8,"0.25,0.25,0.25",true},
    wyckPos{'f',12,"x,0,0",false},
    wyckPos{'g',12,"x,0,0.5",false},
    wyckPos{'h',12,"x,0.5,0",false},
    wyckPos{'i',16,"x,x,x",false},
    wyckPos{'j',24,"0.25,y,y+0.5",false},
    wyckPos{'k',24,"0,y,z",false},
    wyckPos{'l',48,"x,y,z",false}
  },

  { // 224
    wyckPos{'a',2,"0.25,0.25,0.25",true},
    wyckPos{'b',4,"0,0,0",true},
    wyckPos{'c',4,"0.5,0.5,0.5",true},
    wyckPos{'d',6,"0.25,0.75,0.75",true},
    wyckPos{'e',8,"x,x,x",false},
    wyckPos{'f',12,"0.5,0.25,0.75",true},
    wyckPos{'g',12,"x,0.25,0.25",false},
    wyckPos{'h',24,"x,0.25,0.75",false},
    wyckPos{'i',24,"0.5,y,y+0.5",false},
    wyckPos{'j',24,"0.5,y,-y",false},
    wyckPos{'k',24,"x,x,z",false},
    wyckPos{'l',48,"x,y,z",false}
  },

  { // 225
    wyckPos{'a',4,"0,0,0",true},
    wyckPos{'b',4,"0.5,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',24,"0,0.25,0.25",true},
    wyckPos{'e',24,"x,0,0",false},
    wyckPos{'f',32,"x,x,x",false},
    wyckPos{'g',48,"x,0.25,0.25",false},
    wyckPos{'h',48,"0,y,y",false},
    wyckPos{'i',48,"0.5,y,y",false},
    wyckPos{'j',96,"0,y,z",false},
    wyckPos{'k',96,"x,x,z",false},
    wyckPos{'l',192,"x,y,z",false}
  },

  { // 226
    wyckPos{'a',8,"0.25,0.25,0.25",true},
    wyckPos{'b',8,"0,0,0",true},
    wyckPos{'c',24,"0.25,0,0",true},
    wyckPos{'d',24,"0,0.25,0.25",true},
    wyckPos{'e',48,"x,0,0",false},
    wyckPos{'f',48,"x,0.25,0.25",false},
    wyckPos{'g',64,"x,x,x",false},
    wyckPos{'h',96,"0.25,y,y",false},
    wyckPos{'i',96,"0,y,z",false},
    wyckPos{'j',192,"x,y,z",false}
  },

  { // 227
    wyckPos{'a',8,"0.125,0.125,0.125",true},
    wyckPos{'b',8,"0.375,0.375,0.375",true},
    wyckPos{'c',16,"0,0,0",true},
    wyckPos{'d',16,"0.5,0.5,0.5",true},
    wyckPos{'e',32,"x,x,x",false},
    wyckPos{'f',48,"x,0.125,0.125",false},
    wyckPos{'g',96,"x,x,z",false},
    wyckPos{'h',96,"0,y,-y",false},
    wyckPos{'i',192,"x,y,z",false}
  },

  { // 228
    wyckPos{'a',16,"0.125,0.125,0.125",true},
    wyckPos{'b',32,"0.25,0.25,0.25",true},
    wyckPos{'c',32,"0,0,0",true},
    wyckPos{'d',48,"0.875,0.125,0.125",true},
    wyckPos{'e',64,"x,x,x",false},
    wyckPos{'f',96,"x,0.125,0.125",false},
    wyckPos{'g',96,"0.25,y,-y",false},
    wyckPos{'h',192,"x,y,z",false}
  },

  { // 229
    wyckPos{'a',2,"0,0,0",true},
    wyckPos{'b',6,"0,0.5,0.5",true},
    wyckPos{'c',8,"0.25,0.25,0.25",true},
    wyckPos{'d',12,"0.25,0,0.5",true},
    wyckPos{'e',12,"x,0,0",false},
    wyckPos{'f',16,"x,x,x",false},
    wyckPos{'g',24,"x,0,0.5",false},
    wyckPos{'h',24,"0,y,y",false},
    wyckPos{'i',48,"0.25,y,-y+0.5",false},
    wyckPos{'j',48,"0,y,z",false},
    wyckPos{'k',48,"x,x,z",false},
    wyckPos{'l',96,"x,y,z",false}
  },

  { // 230
    wyckPos{'a',16,"0,0,0",true},
    wyckPos{'b',16,"0.125,0.125,0.125",true},
    wyckPos{'c',24,"0.125,0,0.25",true},
    wyckPos{'d',24,"0.375,0,0.25",true},
    wyckPos{'e',32,"x,x,x",false},
    wyckPos{'f',48,"x,0,0.25",false},
    wyckPos{'g',48,"0.125,y,-y+0.25",false},
    wyckPos{'h',96,"x,y,z",false}
  }

};

#endif
