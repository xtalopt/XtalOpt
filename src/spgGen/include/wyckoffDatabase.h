/**********************************************************************
  wyckoffDatabase.h - Database that contains the wyckoff letter, multiplicity,
                      and first position of each wyckoff position of every
                      spacegroup. It is stored as a static const vector of
                      vectors of tuples.

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

#include "spgGen.h"

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
    wyckPos{'a',1,"x,y,z"}
  },

  { // 2
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0,0.5,0"},
    wyckPos{'d',1,"0.5,0,0"},
    wyckPos{'e',1,"0.5,0.5,0"},
    wyckPos{'f',1,"0.5,0,0.5"},
    wyckPos{'g',1,"0,0.5,0.5"},
    wyckPos{'h',1,"0.5,0.5,0.5"},
    wyckPos{'i',2,"x,y,z"}
  },

  { // 3 - unique axis b
    wyckPos{'a',1,"0,y,0"},
    wyckPos{'b',1,"0,y,0.5"},
    wyckPos{'c',1,"0.5,y,0"},
    wyckPos{'d',1,"0.5,y,0.5"},
    wyckPos{'e',2,"x,y,z"}
  },

  { // 4 - unique axis b
    wyckPos{'a',2,"x,y,z"}
  },

  { // 5 - unique axis b
    wyckPos{'a',2,"0,y,0"},
    wyckPos{'b',2,"0,y,0.5"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 6 - unique axis b
    wyckPos{'a',1,"x,0,z"},
    wyckPos{'b',1,"x,0.5,z"},
    wyckPos{'c',2,"x,y,z"}
  },

  { // 7 - unique axis b
    wyckPos{'a',2,"x,y,z"}
  },

  { // 8 - unique axis b
    wyckPos{'a',2,"x,0,z"},
    wyckPos{'b',4,"x,y,z"}
  },

  { // 9 - unique axis b
    wyckPos{'a',4,"x,y,z"}
  },

  { // 10 - unique axis b
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0.5,0"},
    wyckPos{'c',1,"0,0,0.5"},
    wyckPos{'d',1,"0.5,0,0"},
    wyckPos{'e',1,"0.5,0.5,0"},
    wyckPos{'f',1,"0,0.5,0.5"},
    wyckPos{'g',1,"0.5,0,0.5"},
    wyckPos{'h',1,"0.5,0.5,0.5"},
    wyckPos{'i',2,"0,y,0"},
    wyckPos{'j',2,"0.5,y,0"},
    wyckPos{'k',2,"0,y,0.5"},
    wyckPos{'l',2,"0.5,y,0.5"},
    wyckPos{'m',2,"x,0,z"},
    wyckPos{'n',2,"x,0.5,z"},
    wyckPos{'o',4,"x,y,z"}
  },

  { // 11 - unique axis b
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0,0"},
    wyckPos{'c',2,"0,0,0.5"},
    wyckPos{'d',2,"0.5,0,0.5"},
    wyckPos{'e',2,"x,0.25,z"},
    wyckPos{'f',4,"x,y,z"}
  },

  { // 12 - unique axis b
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0.5,0"},
    wyckPos{'c',2,"0,0,0.5"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',4,"0.25,0.25,0"},
    wyckPos{'f',4,"0.25,0.25,0.5"},
    wyckPos{'g',4,"0,y,0"},
    wyckPos{'h',4,"0,y,0.5"},
    wyckPos{'i',4,"x,0,z"},
    wyckPos{'j',8,"x,y,z"}
  },

  { // 13 - unique axis b
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0.5,0"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0.5,0,0"},
    wyckPos{'e',2,"0,y,0.25"},
    wyckPos{'f',2,"0.5,y,0.25"},
    wyckPos{'g',4,"x,y,z"}
  },

  { // 14 - unique axis b
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0,0"},
    wyckPos{'c',2,"0,0,0.5"},
    wyckPos{'d',2,"0.5,0,0.5"},
    wyckPos{'e',4,"x,y,z"}
  },

  { // 15 - unique axis b
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0.5,0"},
    wyckPos{'c',4,"0.25,0.25,0"},
    wyckPos{'d',4,"0.25,0.25,0.5"},
    wyckPos{'e',4,"0,y,0.25"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 16
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0,0"},
    wyckPos{'c',1,"0,0.5,0"},
    wyckPos{'d',1,"0,0,0.5"},
    wyckPos{'e',1,"0.5,0.5,0"},
    wyckPos{'f',1,"0.5,0,0.5"},
    wyckPos{'g',1,"0,0.5,0.5"},
    wyckPos{'h',1,"0.5,0.5,0.5"},
    wyckPos{'i',2,"x,0,0"},
    wyckPos{'j',2,"x,0,0.5"},
    wyckPos{'k',2,"x,0.5,0"},
    wyckPos{'l',2,"x,0.5,0.5"},
    wyckPos{'m',2,"0,y,0"},
    wyckPos{'n',2,"0,y,0.5"},
    wyckPos{'o',2,"0.5,y,0"},
    wyckPos{'p',2,"0.5,y,0.5"},
    wyckPos{'q',2,"0,0,z"},
    wyckPos{'r',2,"0.5,0,z"},
    wyckPos{'s',2,"0,0.5,z"},
    wyckPos{'t',2,"0.5,0.5,z"},
    wyckPos{'u',4,"x,y,z"}
  },

  { // 17
    wyckPos{'a',2,"x,0,0"},
    wyckPos{'b',2,"x,0.5,0"},
    wyckPos{'c',2,"0,y,0.25"},
    wyckPos{'d',2,"0.5,y,0.25"},
    wyckPos{'e',4,"x,y,z"}
  },

  { // 18
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 19
    wyckPos{'a',4,"x,y,z"}
  },

  { // 20
    wyckPos{'a',4,"x,0,0"},
    wyckPos{'b',4,"0,y,0.25"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 21
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0.5,0"},
    wyckPos{'c',2,"0.5,0,0.5"},
    wyckPos{'d',2,"0,0,0.5"},
    wyckPos{'e',4,"x,0,0"},
    wyckPos{'f',4,"x,0,0.5"},
    wyckPos{'g',4,"0,y,0"},
    wyckPos{'h',4,"0,y,0.5"},
    wyckPos{'i',4,"0,0,z"},
    wyckPos{'j',4,"0,0.5,z"},
    wyckPos{'k',4,"0.25,0.25,z"},
    wyckPos{'l',8,"x,y,z"}
  },

  { // 22
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.25,0.25,0.75"},
    wyckPos{'e',8,"x,0,0"},
    wyckPos{'f',8,"0,y,0"},
    wyckPos{'g',8,"0,0,z"},
    wyckPos{'h',8,"0.25,0.25,z"},
    wyckPos{'i',8,"0.25,y,0.25"},
    wyckPos{'j',8,"x,0.25,0.25"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 23
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0,0"},
    wyckPos{'c',2,"0,0,0.5"},
    wyckPos{'d',2,"0,0.5,0"},
    wyckPos{'e',4,"x,0,0"},
    wyckPos{'f',4,"x,0,0.5"},
    wyckPos{'g',4,"0,y,0"},
    wyckPos{'h',4,"0.5,y,0"},
    wyckPos{'i',4,"0,0,z"},
    wyckPos{'j',4,"0,0.5,z"},
    wyckPos{'k',8,"x,y,z"}
  },

  { // 24
    wyckPos{'a',4,"x,0,0.25"},
    wyckPos{'b',4,"0.25,y,0"},
    wyckPos{'c',4,"0,0.25,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 25
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',1,"0,0.5,z"},
    wyckPos{'c',1,"0.5,0,z"},
    wyckPos{'d',1,"0.5,0.5,z"},
    wyckPos{'e',2,"x,0,z"},
    wyckPos{'f',2,"x,0.5,z"},
    wyckPos{'g',2,"0,y,z"},
    wyckPos{'h',2,"0.5,y,z"},
    wyckPos{'i',4,"x,y,z"}
  },

  { // 26
    wyckPos{'a',2,"0,y,z"},
    wyckPos{'b',2,"0.5,y,z"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 27
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',2,"0.5,0,z"},
    wyckPos{'d',2,"0.5,0.5,z"},
    wyckPos{'e',4,"x,y,z"}
  },

  { // 28
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',2,"0.25,y,z"},
    wyckPos{'d',4,"x,y,z"}
  },

  { // 29
    wyckPos{'a',4,"x,y,z"}
  },

  { // 30
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0,z"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 31
    wyckPos{'a',2,"0,y,z"},
    wyckPos{'b',4,"x,y,z"}
  },

  { // 32
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 33
    wyckPos{'a',4,"x,y,z"}
  },

  { // 34
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',4,"x,y,z"}
  },

  { // 35
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',4,"0.25,0.25,z"},
    wyckPos{'d',4,"x,0,z"},
    wyckPos{'e',4,"0,y,z"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 36
    wyckPos{'a',4,"0,y,z"},
    wyckPos{'b',8,"x,y,z"}
  },

  { // 37
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',4,"0.25,0.25,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 38
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0,z"},
    wyckPos{'c',4,"x,0,z"},
    wyckPos{'d',4,"0,y,z"},
    wyckPos{'e',4,"0.5,y,z"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 39
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0.5,0,z"},
    wyckPos{'c',4,"x,0.25,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 40
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0.25,y,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 41
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',8,"x,y,z"}
  },

  { // 42
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',8,"0.25,0.25,z"},
    wyckPos{'c',8,"0,y,z"},
    wyckPos{'d',8,"x,0,z"},
    wyckPos{'e',16,"x,y,z"}
  },

  { // 43
    wyckPos{'a',8,"0,0,z"},
    wyckPos{'b',16,"x,y,z"}
  },

  { // 44
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0,0.5,z"},
    wyckPos{'c',4,"x,0,z"},
    wyckPos{'d',4,"0,y,z"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 45
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 46
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0.25,y,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 47
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0,0"},
    wyckPos{'c',1,"0,0,0.5"},
    wyckPos{'d',1,"0.5,0,0.5"},
    wyckPos{'e',1,"0,0.5,0"},
    wyckPos{'f',1,"0.5,0.5,0"},
    wyckPos{'g',1,"0,0.5,0.5"},
    wyckPos{'h',1,"0.5,0.5,0.5"},
    wyckPos{'i',2,"x,0,0"},
    wyckPos{'j',2,"x,0,0.5"},
    wyckPos{'k',2,"x,0.5,0"},
    wyckPos{'l',2,"x,0.5,0.5"},
    wyckPos{'m',2,"0,y,0"},
    wyckPos{'n',2,"0,y,0.5"},
    wyckPos{'o',2,"0.5,y,0"},
    wyckPos{'p',2,"0.5,y,0.5"},
    wyckPos{'q',2,"0,0,z"},
    wyckPos{'r',2,"0,0.5,z"},
    wyckPos{'s',2,"0.5,0,z"},
    wyckPos{'t',2,"0.5,0.5,z"},
    wyckPos{'u',4,"0,y,z"},
    wyckPos{'v',4,"0.5,y,z"},
    wyckPos{'w',4,"x,0,z"},
    wyckPos{'x',4,"x,0.5,z"},
    wyckPos{'y',4,"x,y,0"},
    wyckPos{'z',4,"x,y,0.5"},
    wyckPos{'A',8,"x,y,z"}
  },

  { // 48
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',2,"0.75,0.25,0.25"},
    wyckPos{'c',2,"0.25,0.25,0.75"},
    wyckPos{'d',2,"0.25,0.75,0.25"},
    wyckPos{'e',4,"0.5,0.5,0.5"},
    wyckPos{'f',4,"0,0,0"},
    wyckPos{'g',4,"x,0.25,0.25"},
    wyckPos{'h',4,"x,0.25,0.75"},
    wyckPos{'i',4,"0.25,y,0.25"},
    wyckPos{'j',4,"0.75,y,0.25"},
    wyckPos{'k',4,"0.25,0.25,z"},
    wyckPos{'l',4,"0.25,0.75,z"},
    wyckPos{'m',8,"x,y,z"}
  },

  { // 49
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0.5,0"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0.5,0,0"},
    wyckPos{'e',2,"0,0,0.25"},
    wyckPos{'f',2,"0.5,0,0.25"},
    wyckPos{'g',2,"0,0.5,0.25"},
    wyckPos{'h',2,"0.5,0.5,0.25"},
    wyckPos{'i',4,"x,0,0.25"},
    wyckPos{'j',4,"x,0.5,0.25"},
    wyckPos{'k',4,"0,y,0.25"},
    wyckPos{'l',4,"0.5,y,0.25"},
    wyckPos{'m',4,"0,0,z"},
    wyckPos{'n',4,"0.5,0.5,z"},
    wyckPos{'o',4,"0,0.5,z"},
    wyckPos{'p',4,"0.5,0,z"},
    wyckPos{'q',4,"x,y,0"},
    wyckPos{'r',8,"x,y,z"}
  },

  { // 50
    wyckPos{'a',2,"0.25,0.25,0"},
    wyckPos{'b',2,"0.75,0.25,0"},
    wyckPos{'c',2,"0.75,0.25,0.5"},
    wyckPos{'d',2,"0.25,0.25,0.5"},
    wyckPos{'e',4,"0,0,0"},
    wyckPos{'f',4,"0,0,0.5"},
    wyckPos{'g',4,"x,0.25,0"},
    wyckPos{'h',4,"x,0.25,0.5"},
    wyckPos{'i',4,"0.25,y,0"},
    wyckPos{'j',4,"0.25,y,0.5"},
    wyckPos{'k',4,"0.25,0.25,z"},
    wyckPos{'l',4,"0.25,0.75,z"},
    wyckPos{'m',8,"x,y,z"}
  },

  { // 51
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0.5,0"},
    wyckPos{'c',2,"0,0,0.5"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',2,"0.25,0,z"},
    wyckPos{'f',2,"0.25,0.5,z"},
    wyckPos{'g',4,"0,y,0"},
    wyckPos{'h',4,"0,y,0.5"},
    wyckPos{'i',4,"x,0,z"},
    wyckPos{'j',4,"x,0.5,z"},
    wyckPos{'k',4,"0.25,y,z"},
    wyckPos{'l',8,"x,y,z"}
  },

  { // 52
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',4,"0.25,0,z"},
    wyckPos{'d',4,"x,0.25,0.25"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 53
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0,0"},
    wyckPos{'c',2,"0.5,0.5,0"},
    wyckPos{'d',2,"0,0.5,0"},
    wyckPos{'e',4,"x,0,0"},
    wyckPos{'f',4,"x,0.5,0"},
    wyckPos{'g',4,"0.25,y,0.25"},
    wyckPos{'h',4,"0,y,z"},
    wyckPos{'i',8,"x,y,z"}
  },

  { // 54
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0.5,0"},
    wyckPos{'c',4,"0,y,0.25"},
    wyckPos{'d',4,"0.25,0,z"},
    wyckPos{'e',4,"0.25,0.5,z"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 55
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',4,"x,y,0"},
    wyckPos{'h',4,"x,y,0.5"},
    wyckPos{'i',8,"x,y,z"}
  },

  { // 56
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',4,"0.25,0.25,z"},
    wyckPos{'d',4,"0.25,0.75,z"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 57
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0,0"},
    wyckPos{'c',4,"x,0.25,0"},
    wyckPos{'d',4,"x,y,0.25"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 58
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',4,"x,y,0"},
    wyckPos{'h',8,"x,y,z"}
  },

  { // 59
    wyckPos{'a',2,"0.25,0.25,z"},
    wyckPos{'b',2,"0.25,0.75,z"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0,0,0.5"},
    wyckPos{'e',4,"0.25,y,z"},
    wyckPos{'f',4,"x,0.25,z"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 60
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0.5,0"},
    wyckPos{'c',4,"0,y,0.25"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 61
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 62
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',4,"x,0.25,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 63
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0.5,0"},
    wyckPos{'c',4,"0,y,0.25"},
    wyckPos{'d',8,"0.25,0.25,0"},
    wyckPos{'e',8,"x,0,0"},
    wyckPos{'f',8,"0,y,z"},
    wyckPos{'g',8,"x,y,0.25"},
    wyckPos{'h',16,"x,y,z"}
  },

  { // 64
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0,0"},
    wyckPos{'c',8,"0.25,0.25,0"},
    wyckPos{'d',8,"x,0,0"},
    wyckPos{'e',8,"0.25,y,0.25"},
    wyckPos{'f',8,"0,y,z"},
    wyckPos{'g',16,"x,y,z"}
  },

  { // 65
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0,0"},
    wyckPos{'c',2,"0.5,0,0.5"},
    wyckPos{'d',2,"0,0,0.5"},
    wyckPos{'e',4,"0.25,0.25,0"},
    wyckPos{'f',4,"0.25,0.25,0.5"},
    wyckPos{'g',4,"x,0,0"},
    wyckPos{'h',4,"x,0,0.5"},
    wyckPos{'i',4,"0,y,0"},
    wyckPos{'j',4,"0,y,0.5"},
    wyckPos{'k',4,"0,0,z"},
    wyckPos{'l',4,"0,0.5,z"},
    wyckPos{'m',8,"0.25,0.25,z"},
    wyckPos{'n',8,"0,y,z"},
    wyckPos{'o',8,"x,0,z"},
    wyckPos{'p',8,"x,y,0"},
    wyckPos{'q',8,"x,y,0.5"},
    wyckPos{'r',16,"x,y,z"}
  },

  { // 66
    wyckPos{'a',4,"0,0,0.25"},
    wyckPos{'b',4,"0,0.5,0.25"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0,0.5,0"},
    wyckPos{'e',4,"0.25,0.25,0"},
    wyckPos{'f',4,"0.25,0.75,0"},
    wyckPos{'g',8,"x,0,0.25"},
    wyckPos{'h',8,"0,y,0.25"},
    wyckPos{'i',8,"0,0,z"},
    wyckPos{'j',8,"0,0.5,z"},
    wyckPos{'k',8,"0.25,0.25,z"},
    wyckPos{'l',8,"x,y,0"},
    wyckPos{'m',16,"x,y,z"}
  },

  { // 67
    wyckPos{'a',4,"0.25,0,0"},
    wyckPos{'b',4,"0.25,0,0.5"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0,0,0.5"},
    wyckPos{'e',4,"0.25,0.25,0"},
    wyckPos{'f',4,"0.25,0.25,0.5"},
    wyckPos{'g',4,"0,0.25,z"},
    wyckPos{'h',8,"x,0,0"},
    wyckPos{'i',8,"x,0,0.5"},
    wyckPos{'j',8,"0.25,y,0"},
    wyckPos{'k',8,"0.25,y,0.5"},
    wyckPos{'l',8,"0.25,0,z"},
    wyckPos{'m',8,"0,y,z"},
    wyckPos{'n',8,"x,0.25,z"},
    wyckPos{'o',16,"x,y,z"}
  },

  { // 68
    wyckPos{'a',4,"0,0.25,0.25"},
    wyckPos{'b',4,"0,0.25,0.75"},
    wyckPos{'c',8,"0.25,0.75,0"},
    wyckPos{'d',8,"0,0,0"},
    wyckPos{'e',8,"x,0.25,0.25"},
    wyckPos{'f',8,"0,y,0.25"},
    wyckPos{'g',8,"0,0.25,z"},
    wyckPos{'h',8,"0.25,0,z"},
    wyckPos{'i',16,"x,y,z"}
  },

  { // 69
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',8,"0,0.25,0.25"},
    wyckPos{'d',8,"0.25,0,0.25"},
    wyckPos{'e',8,"0.25,0.25,0"},
    wyckPos{'f',8,"0.25,0.25,0.25"},
    wyckPos{'g',8,"x,0,0"},
    wyckPos{'h',8,"0,y,0"},
    wyckPos{'i',8,"0,0,z"},
    wyckPos{'j',16,"0.25,0.25,z"},
    wyckPos{'k',16,"0.25,y,0.25"},
    wyckPos{'l',16,"x,0.25,0.25"},
    wyckPos{'m',16,"0,y,z"},
    wyckPos{'n',16,"x,0,z"},
    wyckPos{'o',16,"x,y,0"},
    wyckPos{'p',32,"x,y,z"}
  },

  { // 70
    wyckPos{'a',8,"0.125,0.125,0.125"},
    wyckPos{'b',8,"0.125,0.125,0.625"},
    wyckPos{'c',16,"0,0,0"},
    wyckPos{'d',16,"0.5,0.5,0.5"},
    wyckPos{'e',16,"x,0.125,0.125"},
    wyckPos{'f',16,"0.125,y,0.125"},
    wyckPos{'g',16,"0.125,0.125,z"},
    wyckPos{'h',32,"x,y,z"}
  },

  { // 71
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0.5,0.5"},
    wyckPos{'c',2,"0.5,0.5,0"},
    wyckPos{'d',2,"0.5,0,0.5"},
    wyckPos{'e',4,"x,0,0"},
    wyckPos{'f',4,"x,0.5,0"},
    wyckPos{'g',4,"0,y,0"},
    wyckPos{'h',4,"0,y,0.5"},
    wyckPos{'i',4,"0,0,z"},
    wyckPos{'j',4,"0.5,0,z"},
    wyckPos{'k',8,"0.25,0.25,0.25"},
    wyckPos{'l',8,"0,y,z"},
    wyckPos{'m',8,"x,0,z"},
    wyckPos{'n',8,"x,y,0"},
    wyckPos{'o',16,"x,y,z"}
  },

  { // 72
    wyckPos{'a',4,"0,0,0.25"},
    wyckPos{'b',4,"0.5,0,0.25"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0.5,0,0"},
    wyckPos{'e',8,"0.25,0.25,0.25"},
    wyckPos{'f',8,"x,0,0.25"},
    wyckPos{'g',8,"0,y,0.25"},
    wyckPos{'h',8,"0,0,z"},
    wyckPos{'i',8,"0,0.5,z"},
    wyckPos{'j',8,"x,y,0"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 73
    wyckPos{'a',8,"0,0,0"},
    wyckPos{'b',8,"0.25,0.25,0.25"},
    wyckPos{'c',8,"x,0,0.25"},
    wyckPos{'d',8,"0.25,y,0"},
    wyckPos{'e',8,"0,0.25,z"},
    wyckPos{'f',16,"x,y,z"}
  },

  { // 74
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.25,0.25,0.75"},
    wyckPos{'e',4,"0,0.25,z"},
    wyckPos{'f',8,"x,0,0"},
    wyckPos{'g',8,"0.25,y,0.25"},
    wyckPos{'h',8,"0,y,z"},
    wyckPos{'i',8,"x,0.25,z"},
    wyckPos{'j',16,"x,y,z"}
  },

  { // 75
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',1,"0.5,0.5,z"},
    wyckPos{'c',2,"0,0.5,z"},
    wyckPos{'d',4,"x,y,z"}
  },

  { // 76
    wyckPos{'a',4,"x,y,z"}
  },

  { // 77
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0.5,z"},
    wyckPos{'c',2,"0,0.5,z"},
    wyckPos{'d',4,"x,y,z"}
  },

  { // 78
    wyckPos{'a',4,"x,y,z"}
  },

  { // 79
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 80
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',8,"x,y,z"}
  },

  { // 81
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.5,0.5,0"},
    wyckPos{'d',1,"0.5,0.5,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',2,"0.5,0.5,z"},
    wyckPos{'g',2,"0,0.5,z"},
    wyckPos{'h',4,"x,y,z"}
  },

  { // 82
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0.25"},
    wyckPos{'d',2,"0,0.5,0.75"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 83
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.5,0.5,0"},
    wyckPos{'d',1,"0.5,0.5,0.5"},
    wyckPos{'e',2,"0,0.5,0"},
    wyckPos{'f',2,"0,0.5,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,y,0"},
    wyckPos{'k',4,"x,y,0.5"},
    wyckPos{'l',8,"x,y,z"}
  },

  { // 84
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0.5,0"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',2,"0,0,0.25"},
    wyckPos{'f',2,"0.5,0.5,0.25"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,y,0"},
    wyckPos{'k',8,"x,y,z"}
  },

  { // 85
    wyckPos{'a',2,"0.25,0.75,0"},
    wyckPos{'b',2,"0.25,0.75,0.5"},
    wyckPos{'c',2,"0.25,0.25,z"},
    wyckPos{'d',4,"0,0,0"},
    wyckPos{'e',4,"0,0,0.5"},
    wyckPos{'f',4,"0.25,0.75,z"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 86
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',2,"0.25,0.25,0.75"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0,0,0.5"},
    wyckPos{'e',4,"0.75,0.25,z"},
    wyckPos{'f',4,"0.25,0.25,z"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 87
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',8,"0.25,0.25,0.25"},
    wyckPos{'g',8,"0,0.5,z"},
    wyckPos{'h',8,"x,y,0"},
    wyckPos{'i',16,"x,y,z"}
  },

  { // 88
    wyckPos{'a',4,"0,0.25,0.125"},
    wyckPos{'b',4,"0,0.25,0.625"},
    wyckPos{'c',8,"0,0,0"},
    wyckPos{'d',8,"0,0,0.5"},
    wyckPos{'e',8,"0,0.25,z"},
    wyckPos{'f',16,"x,y,z"}
  },

  { // 89
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.5,0.5,0"},
    wyckPos{'d',1,"0.5,0.5,0.5"},
    wyckPos{'e',2,"0.5,0,0"},
    wyckPos{'f',2,"0.5,0,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,x,0"},
    wyckPos{'k',4,"x,x,0.5"},
    wyckPos{'l',4,"x,0,0"},
    wyckPos{'m',4,"x,0.5,0.5"},
    wyckPos{'n',4,"x,0,0.5"},
    wyckPos{'o',4,"x,0.5,0"},
    wyckPos{'p',8,"x,y,z"}
  },

  { // 90
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,z"},
    wyckPos{'d',4,"0,0,z"},
    wyckPos{'e',4,"x,x,0"},
    wyckPos{'f',4,"x,x,0.5"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 91
    wyckPos{'a',4,"0,y,0"},
    wyckPos{'b',4,"0.5,y,0"},
    wyckPos{'c',4,"x,x,0.375"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 92
    wyckPos{'a',4,"x,x,0"},
    wyckPos{'b',8,"x,y,z"}
  },

  { // 93
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0.5,0"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',2,"0,0,0.25"},
    wyckPos{'f',2,"0.5,0.5,0.25"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,0,0"},
    wyckPos{'k',4,"x,0.5,0.5"},
    wyckPos{'l',4,"x,0,0.5"},
    wyckPos{'m',4,"x,0.5,0"},
    wyckPos{'n',4,"x,x,0.25"},
    wyckPos{'o',4,"x,x,0.75"},
    wyckPos{'p',8,"x,y,z"}
  },

  { // 94
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0,z"},
    wyckPos{'d',4,"0,0.5,z"},
    wyckPos{'e',4,"x,x,0"},
    wyckPos{'f',4,"x,x,0.5"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 95
    wyckPos{'a',4,"0,y,0"},
    wyckPos{'b',4,"0.5,y,0"},
    wyckPos{'c',4,"x,x,0.625"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 96
    wyckPos{'a',4,"x,x,0"},
    wyckPos{'b',8,"x,y,z"}
  },

  { // 97
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',8,"0,0.5,z"},
    wyckPos{'g',8,"x,x,0"},
    wyckPos{'h',8,"x,0,0"},
    wyckPos{'i',8,"x,0,0.5"},
    wyckPos{'j',8,"x,x+0.5,0.25"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 98
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',8,"0,0,z"},
    wyckPos{'d',8,"x,x,0"},
    wyckPos{'e',8,"-x,x,0"},
    wyckPos{'f',8,"x,0.25,0.125"},
    wyckPos{'g',16,"x,y,z"}
  },

  { // 99
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',1,"0.5,0.5,z"},
    wyckPos{'c',2,"0.5,0,z"},
    wyckPos{'d',4,"x,x,z"},
    wyckPos{'e',4,"x,0,z"},
    wyckPos{'f',4,"x,0.5,z"},
    wyckPos{'g',8,"x,y,z"}
  },

  { // 100
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0,z"},
    wyckPos{'c',4,"x,x+0.5,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 101
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0.5,z"},
    wyckPos{'c',4,"0,0.5,z"},
    wyckPos{'d',4,"x,x,z"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 102
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',4,"x,x,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 103
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0.5,z"},
    wyckPos{'c',4,"0,0.5,z"},
    wyckPos{'d',8,"x,y,z"}
  },

  { // 104
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 105
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.5,0.5,z"},
    wyckPos{'c',2,"0,0.5,z"},
    wyckPos{'d',4,"x,0,z"},
    wyckPos{'e',4,"x,0.5,z"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 106
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',8,"x,y,z"}
  },

  { // 107
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0,0.5,z"},
    wyckPos{'c',8,"x,x,z"},
    wyckPos{'d',8,"x,0,z"},
    wyckPos{'e',16,"x,y,z"}
  },

  { // 108
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',4,"0.5,0,z"},
    wyckPos{'c',8,"x,x+0.5,z"},
    wyckPos{'d',16,"x,y,z"}
  },

  { // 109
    wyckPos{'a',4,"0,0,z"},
    wyckPos{'b',8,"0,y,z"},
    wyckPos{'c',16,"x,y,z"}
  },

  { // 110
    wyckPos{'a',8,"0,0,z"},
    wyckPos{'b',16,"x,y,z"}
  },

  { // 111
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',1,"0,0,0.5"},
    wyckPos{'d',1,"0.5,0.5,0"},
    wyckPos{'e',2,"0.5,0,0"},
    wyckPos{'f',2,"0.5,0,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.5,0.5,z"},
    wyckPos{'i',4,"x,0,0"},
    wyckPos{'j',4,"x,0.5,0.5"},
    wyckPos{'k',4,"x,0,0.5"},
    wyckPos{'l',4,"x,0.5,0"},
    wyckPos{'m',4,"0,0.5,z"},
    wyckPos{'n',4,"x,x,z"},
    wyckPos{'o',8,"x,y,z"}
  },

  { // 112
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0.5,0,0.25"},
    wyckPos{'c',2,"0.5,0.5,0.25"},
    wyckPos{'d',2,"0,0.5,0.25"},
    wyckPos{'e',2,"0,0,0"},
    wyckPos{'f',2,"0.5,0.5,0"},
    wyckPos{'g',4,"x,0,0.25"},
    wyckPos{'h',4,"0.5,y,0.25"},
    wyckPos{'i',4,"x,0.5,0.25"},
    wyckPos{'j',4,"0,y,0.25"},
    wyckPos{'k',4,"0,0,z"},
    wyckPos{'l',4,"0.5,0.5,z"},
    wyckPos{'m',4,"0,0.5,z"},
    wyckPos{'n',8,"x,y,z"}
  },

  { // 113
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,z"},
    wyckPos{'d',4,"0,0,z"},
    wyckPos{'e',4,"x,x+0.5,z"},
    wyckPos{'f',8,"x,y,z"}
  },

  { // 114
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0,z"},
    wyckPos{'d',4,"0,0.5,z"},
    wyckPos{'e',8,"x,y,z"}
  },

  { // 115
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0"},
    wyckPos{'c',1,"0.5,0.5,0.5"},
    wyckPos{'d',1,"0,0,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',2,"0.5,0.5,z"},
    wyckPos{'g',2,"0,0.5,z"},
    wyckPos{'h',4,"x,x,0"},
    wyckPos{'i',4,"x,x,0.5"},
    wyckPos{'j',4,"x,0,z"},
    wyckPos{'k',4,"x,0.5,z"},
    wyckPos{'l',8,"x,y,z"}
  },

  { // 116
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0.5,0.5,0.25"},
    wyckPos{'c',2,"0,0,0"},
    wyckPos{'d',2,"0.5,0.5,0"},
    wyckPos{'e',4,"x,x,0.25"},
    wyckPos{'f',4,"x,x,0.75"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',8,"x,y,z"}
  },

  { // 117
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',4,"x,x+0.5,0"},
    wyckPos{'h',4,"x,x+0.5,0.5"},
    wyckPos{'i',8,"x,y,z"}
  },

  { // 118
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0.25"},
    wyckPos{'d',2,"0,0.5,0.75"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"x,-x+0.5,0.25"},
    wyckPos{'g',4,"x,x+0.5,0.25"},
    wyckPos{'h',4,"0,0.5,z"},
    wyckPos{'i',8,"x,y,z"}
  },

  { // 119
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0.25"},
    wyckPos{'d',2,"0,0.5,0.75"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',8,"x,x,0"},
    wyckPos{'h',8,"x,x+0.5,0.25"},
    wyckPos{'i',8,"x,0,z"},
    wyckPos{'j',16,"x,y,z"}
  },

  { // 120
    wyckPos{'a',4,"0,0,0.25"},
    wyckPos{'b',4,"0,0,0"},
    wyckPos{'c',4,"0,0.5,0.25"},
    wyckPos{'d',4,"0,0.5,0"},
    wyckPos{'e',8,"x,x,0.25"},
    wyckPos{'f',8,"0,0,z"},
    wyckPos{'g',8,"0,0.5,z"},
    wyckPos{'h',8,"x,x+0.5,0"},
    wyckPos{'i',16,"x,y,z"}
  },

  { // 121
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',8,"x,0,0"},
    wyckPos{'g',8,"x,0,0.5"},
    wyckPos{'h',8,"0,0.5,z"},
    wyckPos{'i',8,"x,x,z"},
    wyckPos{'j',16,"x,y,z"}
  },

  { // 122
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.5"},
    wyckPos{'c',8,"0,0,z"},
    wyckPos{'d',8,"x,0.25,0.125"},
    wyckPos{'e',16,"x,y,z"}
  },

  { // 123
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.5,0.5,0"},
    wyckPos{'d',1,"0.5,0.5,0.5"},
    wyckPos{'e',2,"0,0.5,0.5"},
    wyckPos{'f',2,"0,0.5,0"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,x,0"},
    wyckPos{'k',4,"x,x,0.5"},
    wyckPos{'l',4,"x,0,0"},
    wyckPos{'m',4,"x,0,0.5"},
    wyckPos{'n',4,"x,0.5,0"},
    wyckPos{'o',4,"x,0.5,0.5"},
    wyckPos{'p',8,"x,y,0"},
    wyckPos{'q',8,"x,y,0.5"},
    wyckPos{'r',8,"x,x,z"},
    wyckPos{'s',8,"x,0,z"},
    wyckPos{'t',8,"x,0.5,z"},
    wyckPos{'u',16,"x,y,z"}
  },

  { // 124
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',2,"0.5,0.5,0.25"},
    wyckPos{'d',2,"0.5,0.5,0"},
    wyckPos{'e',4,"0,0.5,0"},
    wyckPos{'f',4,"0,0.5,0.25"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',8,"0,0.5,z"},
    wyckPos{'j',8,"x,x,0.25"},
    wyckPos{'k',8,"x,0,0.25"},
    wyckPos{'l',8,"x,0.5,0.25"},
    wyckPos{'m',8,"x,y,0"},
    wyckPos{'n',16,"x,y,z"}
  },

  { // 125
    wyckPos{'a',2,"0.25,0.25,0"},
    wyckPos{'b',2,"0.25,0.25,0.5"},
    wyckPos{'c',2,"0.75,0.25,0"},
    wyckPos{'d',2,"0.75,0.25,0.5"},
    wyckPos{'e',4,"0,0,0"},
    wyckPos{'f',4,"0,0,0.5"},
    wyckPos{'g',4,"0.25,0.25,z"},
    wyckPos{'h',4,"0.75,0.25,z"},
    wyckPos{'i',8,"x,x,0"},
    wyckPos{'j',8,"x,x,0.5"},
    wyckPos{'k',8,"x,0.25,0"},
    wyckPos{'l',8,"x,0.25,0.5"},
    wyckPos{'m',8,"x,-x,z"},
    wyckPos{'n',16,"x,y,z"}
  },

  { // 126
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',2,"0.25,0.25,0.75"},
    wyckPos{'c',4,"0.25,0.75,0.75"},
    wyckPos{'d',4,"0.25,0.75,0"},
    wyckPos{'e',4,"0.25,0.25,z"},
    wyckPos{'f',8,"0,0,0"},
    wyckPos{'g',8,"0.25,0.75,z"},
    wyckPos{'h',8,"x,x,0.25"},
    wyckPos{'i',8,"x,0.25,0.25"},
    wyckPos{'j',8,"x,0.75,0.25"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 127
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',2,"0,0.5,0.5"},
    wyckPos{'d',2,"0,0.5,0"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0,0.5,z"},
    wyckPos{'g',4,"x,x+0.5,0"},
    wyckPos{'h',4,"x,x+0.5,0.5"},
    wyckPos{'i',8,"x,y,0"},
    wyckPos{'j',8,"x,y,0.5"},
    wyckPos{'k',8,"x,x+0.5,z"},
    wyckPos{'l',16,"x,y,z"}
  },

  { // 128
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',8,"0,0.5,z"},
    wyckPos{'g',8,"x,x+0.5,0.25"},
    wyckPos{'h',8,"x,y,0"},
    wyckPos{'i',16,"x,y,z"}
  },

  { // 129
    wyckPos{'a',2,"0.75,0.25,0"},
    wyckPos{'b',2,"0.75,0.25,0.5"},
    wyckPos{'c',2,"0.25,0.25,z"},
    wyckPos{'d',4,"0,0,0"},
    wyckPos{'e',4,"0,0,0.5"},
    wyckPos{'f',4,"0.75,0.25,z"},
    wyckPos{'g',8,"x,-x,0"},
    wyckPos{'h',8,"x,-x,0.5"},
    wyckPos{'i',8,"0.25,y,z"},
    wyckPos{'j',8,"x,x,z"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 130
    wyckPos{'a',4,"0.75,0.25,0.25"},
    wyckPos{'b',4,"0.75,0.25,0"},
    wyckPos{'c',4,"0.25,0.25,z"},
    wyckPos{'d',8,"0,0,0"},
    wyckPos{'e',8,"0.75,0.25,z"},
    wyckPos{'f',8,"x,-x,0.25"},
    wyckPos{'g',16,"x,y,z"}
  },

  { // 131
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0.5,0.5,0"},
    wyckPos{'c',2,"0,0.5,0"},
    wyckPos{'d',2,"0,0.5,0.5"},
    wyckPos{'e',2,"0,0,0.25"},
    wyckPos{'f',2,"0.5,0.5,0.25"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',4,"0,0.5,z"},
    wyckPos{'j',4,"x,0,0"},
    wyckPos{'k',4,"x,0.5,0.5"},
    wyckPos{'l',4,"x,0,0.5"},
    wyckPos{'m',4,"x,0.5,0"},
    wyckPos{'n',8,"x,x,0.25"},
    wyckPos{'o',8,"0,y,z"},
    wyckPos{'p',8,"0.5,y,z"},
    wyckPos{'q',8,"x,y,0"},
    wyckPos{'r',16,"x,y,z"}
  },

  { // 132
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.25"},
    wyckPos{'c',2,"0.5,0.5,0"},
    wyckPos{'d',2,"0.5,0.5,0.25"},
    wyckPos{'e',4,"0,0.5,0.25"},
    wyckPos{'f',4,"0,0.5,0"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.5,0.5,z"},
    wyckPos{'i',4,"x,x,0"},
    wyckPos{'j',4,"x,x,0.5"},
    wyckPos{'k',8,"0,0.5,z"},
    wyckPos{'l',8,"x,0,0.25"},
    wyckPos{'m',8,"x,0.5,0.25"},
    wyckPos{'n',8,"x,y,0"},
    wyckPos{'o',8,"x,x,z"},
    wyckPos{'p',16,"x,y,z"}
  },

  { // 133
    wyckPos{'a',4,"0.25,0.25,0"},
    wyckPos{'b',4,"0.75,0.25,0"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.75,0.25,0.75"},
    wyckPos{'e',8,"0,0,0"},
    wyckPos{'f',8,"0.25,0.25,z"},
    wyckPos{'g',8,"0.75,0.25,z"},
    wyckPos{'h',8,"x,0.25,0"},
    wyckPos{'i',8,"x,0.25,0.5"},
    wyckPos{'j',8,"x,x,0.25"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 134
    wyckPos{'a',2,"0.25,0.75,0.25"},
    wyckPos{'b',2,"0.75,0.25,0.25"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.25,0.25,0"},
    wyckPos{'e',4,"0,0,0.5"},
    wyckPos{'f',4,"0,0,0"},
    wyckPos{'g',4,"0.75,0.25,z"},
    wyckPos{'h',8,"0.25,0.25,z"},
    wyckPos{'i',8,"x,0.25,0.75"},
    wyckPos{'j',8,"x,0.25,0.25"},
    wyckPos{'k',8,"x,x,0"},
    wyckPos{'l',8,"x,x,0.5"},
    wyckPos{'m',8,"x,-x,z"},
    wyckPos{'n',16,"x,y,z"}
  },

  { // 135
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0,0,0.25"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',8,"0,0,z"},
    wyckPos{'f',8,"0,0.5,z"},
    wyckPos{'g',8,"x,x+0.5,0.25"},
    wyckPos{'h',8,"x,y,0"},
    wyckPos{'i',16,"x,y,z"}
  },

  { // 136
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"x,x,0"},
    wyckPos{'g',4,"x,-x,0"},
    wyckPos{'h',8,"0,0.5,z"},
    wyckPos{'i',8,"x,y,0"},
    wyckPos{'j',8,"x,x,z"},
    wyckPos{'k',16,"x,y,z"}
  },

  { // 137
    wyckPos{'a',2,"0.75,0.25,0.75"},
    wyckPos{'b',2,"0.75,0.25,0.25"},
    wyckPos{'c',4,"0.75,0.25,z"},
    wyckPos{'d',4,"0.25,0.25,z"},
    wyckPos{'e',8,"0,0,0"},
    wyckPos{'f',8,"x,-x,0.25"},
    wyckPos{'g',8,"0.25,y,z"},
    wyckPos{'h',16,"x,y,z"}
  },

  { // 138
    wyckPos{'a',4,"0.75,0.25,0"},
    wyckPos{'b',4,"0.75,0.25,0.75"},
    wyckPos{'c',4,"0,0,0.5"},
    wyckPos{'d',4,"0,0,0"},
    wyckPos{'e',4,"0.25,0.25,z"},
    wyckPos{'f',8,"0.75,0.25,z"},
    wyckPos{'g',8,"x,-x,0.5"},
    wyckPos{'h',8,"x,-x,0"},
    wyckPos{'i',8,"x,x,z"},
    wyckPos{'j',16,"x,y,z"}
  },

  { // 139
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.5"},
    wyckPos{'c',4,"0,0.5,0"},
    wyckPos{'d',4,"0,0.5,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',8,"0.25,0.25,0.25"},
    wyckPos{'g',8,"0,0.5,z"},
    wyckPos{'h',8,"x,x,0"},
    wyckPos{'i',8,"x,0,0"},
    wyckPos{'j',8,"x,0.5,0"},
    wyckPos{'k',16,"x,x+0.5,0.25"},
    wyckPos{'l',16,"x,y,0"},
    wyckPos{'m',16,"x,x,z"},
    wyckPos{'n',16,"0,y,z"},
    wyckPos{'o',32,"x,y,z"}
  },

  { // 140
    wyckPos{'a',4,"0,0,0.25"},
    wyckPos{'b',4,"0,0.5,0.25"},
    wyckPos{'c',4,"0,0,0"},
    wyckPos{'d',4,"0,0.5,0"},
    wyckPos{'e',8,"0.25,0.25,0.25"},
    wyckPos{'f',8,"0,0,z"},
    wyckPos{'g',8,"0,0.5,z"},
    wyckPos{'h',8,"x,x+0.5,0"},
    wyckPos{'i',16,"x,x,0.25"},
    wyckPos{'j',16,"x,0,0.25"},
    wyckPos{'k',16,"x,y,0"},
    wyckPos{'l',16,"x,x+0.5,z"},
    wyckPos{'m',32,"x,y,z"}
  },

  { // 141
    wyckPos{'a',4,"0,0.75,0.125"},
    wyckPos{'b',4,"0,0.25,0.375"},
    wyckPos{'c',8,"0,0,0"},
    wyckPos{'d',8,"0,0,0.5"},
    wyckPos{'e',8,"0,0.25,z"},
    wyckPos{'f',16,"x,0,0"},
    wyckPos{'g',16,"x,x+0.25,0.875"},
    wyckPos{'h',16,"0,y,z"},
    wyckPos{'i',32,"x,y,z"}
  },

  { // 142
    wyckPos{'a',8,"0,0.25,0.375"},
    wyckPos{'b',8,"0,0.25,0.125"},
    wyckPos{'c',16,"0,0,0"},
    wyckPos{'d',16,"0,0.25,z"},
    wyckPos{'e',16,"x,0,0.25"},
    wyckPos{'f',16,"x,x+0.25,0.125"},
    wyckPos{'g',32,"x,y,z"}
  },

  { // 143
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',1,"0.333333,0.666667,z"},
    wyckPos{'c',1,"0.666667,0.333333,z"},
    wyckPos{'d',3,"x,y,z"}
  },

  { // 144
    wyckPos{'a',3,"x,y,z"}
  },

  { // 145
    wyckPos{'a',3,"x,y,z"}
  },

  { // 146
    wyckPos{'a',3,"0,0,z"},
    wyckPos{'b',9,"x,y,z"}
  },

  { // 147
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0,0,z"},
    wyckPos{'d',2,"0.333333,0.666667,z"},
    wyckPos{'e',3,"0.5,0,0"},
    wyckPos{'f',3,"0.5,0,0.5"},
    wyckPos{'g',6,"x,y,z"}
  },

  { // 148
    wyckPos{'a',3,"0,0,0"},
    wyckPos{'b',3,"0,0,0.5"},
    wyckPos{'c',6,"0,0,z"},
    wyckPos{'d',9,"0.5,0,0.5"},
    wyckPos{'e',9,"0.5,0,0"},
    wyckPos{'f',18,"x,y,z"}
  },

  { // 149
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.333333,0.666667,0"},
    wyckPos{'d',1,"0.333333,0.666667,0.5"},
    wyckPos{'e',1,"0.666667,0.333333,0"},
    wyckPos{'f',1,"0.666667,0.333333,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.333333,0.666667,z"},
    wyckPos{'i',2,"0.666667,0.333333,z"},
    wyckPos{'j',3,"x,-x,0"},
    wyckPos{'k',3,"x,-x,0.5"},
    wyckPos{'l',6,"x,y,z"}
  },

  { // 150
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0,0,z"},
    wyckPos{'d',2,"0.333333,0.666667,z"},
    wyckPos{'e',3,"x,0,0"},
    wyckPos{'f',3,"x,0,0.5"},
    wyckPos{'g',6,"x,y,z"}
  },

  { // 151
    wyckPos{'a',3,"x,-x,0.333333"},
    wyckPos{'b',3,"x,-x,0.833333"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 152
    wyckPos{'a',3,"x,0,0.333333"},
    wyckPos{'b',3,"x,0,0.833333"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 153
    wyckPos{'a',3,"x,-x,0.666667"},
    wyckPos{'b',3,"x,-x,0.166667"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 154
    wyckPos{'a',3,"x,0,0.666667"},
    wyckPos{'b',3,"x,0,0.166667"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 155
    wyckPos{'a',3,"0,0,0"},
    wyckPos{'b',3,"0,0,0.5"},
    wyckPos{'c',6,"0,0,z"},
    wyckPos{'d',9,"x,0,0"},
    wyckPos{'e',9,"x,0,0.5"},
    wyckPos{'f',18,"x,y,z"}
  },

  { // 156
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',1,"0.333333,0.666667,z"},
    wyckPos{'c',1,"0.666667,0.333333,z"},
    wyckPos{'d',3,"x,-x,z"},
    wyckPos{'e',6,"x,y,z"}
  },

  { // 157
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',3,"x,0,z"},
    wyckPos{'d',6,"x,y,z"}
  },

  { // 158
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',2,"0.666667,0.333333,z"},
    wyckPos{'d',6,"x,y,z"}
  },

  { // 159
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 160
    wyckPos{'a',3,"0,0,z"},
    wyckPos{'b',9,"x,-x,z"},
    wyckPos{'c',18,"x,y,z"}
  },

  { // 161
    wyckPos{'a',6,"0,0,z"},
    wyckPos{'b',18,"x,y,z"}
  },

  { // 162
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',3,"0.5,0,0"},
    wyckPos{'g',3,"0.5,0,0.5"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',6,"x,-x,0"},
    wyckPos{'j',6,"x,-x,0.5"},
    wyckPos{'k',6,"x,0,z"},
    wyckPos{'l',12,"x,y,z"}
  },

  { // 163
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',2,"0.333333,0.666667,0.25"},
    wyckPos{'d',2,"0.666667,0.333333,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0.333333,0.666667,z"},
    wyckPos{'g',6,"0.5,0,0"},
    wyckPos{'h',6,"x,-x,0.25"},
    wyckPos{'i',12,"x,y,z"}
  },

  { // 164
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0,0,z"},
    wyckPos{'d',2,"0.333333,0.666667,z"},
    wyckPos{'e',3,"0.5,0,0"},
    wyckPos{'f',3,"0.5,0,0.5"},
    wyckPos{'g',6,"x,0,0"},
    wyckPos{'h',6,"x,0,0.5"},
    wyckPos{'i',6,"x,-x,z"},
    wyckPos{'j',12,"x,y,z"}
  },

  { // 165
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',4,"0,0,z"},
    wyckPos{'d',4,"0.333333,0.666667,z"},
    wyckPos{'e',6,"0.5,0,0"},
    wyckPos{'f',6,"x,0,0.25"},
    wyckPos{'g',12,"x,y,z"}
  },

  { // 166
    wyckPos{'a',3,"0,0,0"},
    wyckPos{'b',3,"0,0,0.5"},
    wyckPos{'c',6,"0,0,z"},
    wyckPos{'d',9,"0.5,0,0.5"},
    wyckPos{'e',9,"0.5,0,0"},
    wyckPos{'f',18,"x,0,0"},
    wyckPos{'g',18,"x,0,0.5"},
    wyckPos{'h',18,"x,-x,z"},
    wyckPos{'i',36,"x,y,z"}
  },

  { // 167
    wyckPos{'a',6,"0,0,0.25"},
    wyckPos{'b',6,"0,0,0"},
    wyckPos{'c',12,"0,0,z"},
    wyckPos{'d',18,"0.5,0,0"},
    wyckPos{'e',18,"x,0,0.25"},
    wyckPos{'f',36,"x,y,z"}
  },

  { // 168
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',3,"0.5,0,z"},
    wyckPos{'d',6,"x,y,z"}
  },

  { // 169
    wyckPos{'a',6,"x,y,z"}
  },

  { // 170
    wyckPos{'a',6,"x,y,z"}
  },

  { // 171
    wyckPos{'a',3,"0,0,z"},
    wyckPos{'b',3,"0.5,0.5,z"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 172
    wyckPos{'a',3,"0,0,z"},
    wyckPos{'b',3,"0.5,0.5,z"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 173
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',6,"x,y,z"}
  },

  { // 174
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.333333,0.666667,0"},
    wyckPos{'d',1,"0.333333,0.666667,0.5"},
    wyckPos{'e',1,"0.666667,0.333333,0"},
    wyckPos{'f',1,"0.666667,0.333333,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.333333,0.666667,z"},
    wyckPos{'i',2,"0.666667,0.333333,z"},
    wyckPos{'j',3,"x,y,0"},
    wyckPos{'k',3,"x,y,0.5"},
    wyckPos{'l',6,"x,y,z"}
  },

  { // 175
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',3,"0.5,0,0"},
    wyckPos{'g',3,"0.5,0,0.5"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',6,"0.5,0,z"},
    wyckPos{'j',6,"x,y,0"},
    wyckPos{'k',6,"x,y,0.5"},
    wyckPos{'l',12,"x,y,z"}
  },

  { // 176
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',2,"0.333333,0.666667,0.25"},
    wyckPos{'d',2,"0.666667,0.333333,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0.333333,0.666667,z"},
    wyckPos{'g',6,"0.5,0,0"},
    wyckPos{'h',6,"x,y,0.25"},
    wyckPos{'i',12,"x,y,z"}
  },

  { // 177
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',3,"0.5,0,0"},
    wyckPos{'g',3,"0.5,0,0.5"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',6,"0.5,0,z"},
    wyckPos{'j',6,"x,0,0"},
    wyckPos{'k',6,"x,0,0.5"},
    wyckPos{'l',6,"x,-x,0"},
    wyckPos{'m',6,"x,-x,0.5"},
    wyckPos{'n',12,"x,y,z"}
  },

  { // 178
    wyckPos{'a',6,"x,0,0"},
    wyckPos{'b',6,"x,2x,0.25"},
    wyckPos{'c',12,"x,y,z"}
  },

  { // 179
    wyckPos{'a',6,"x,0,0"},
    wyckPos{'b',6,"x,2x,0.75"},
    wyckPos{'c',12,"x,y,z"}
  },

  { // 180
    wyckPos{'a',3,"0,0,0"},
    wyckPos{'b',3,"0,0,0.5"},
    wyckPos{'c',3,"0.5,0,0"},
    wyckPos{'d',3,"0.5,0,0.5"},
    wyckPos{'e',6,"0,0,z"},
    wyckPos{'f',6,"0.5,0,z"},
    wyckPos{'g',6,"x,0,0"},
    wyckPos{'h',6,"x,0,0.5"},
    wyckPos{'i',6,"x,2x,0"},
    wyckPos{'j',6,"x,2x,0.5"},
    wyckPos{'k',12,"x,y,z"}
  },

  { // 181
    wyckPos{'a',3,"0,0,0"},
    wyckPos{'b',3,"0,0,0.5"},
    wyckPos{'c',3,"0.5,0,0"},
    wyckPos{'d',3,"0.5,0,0.5"},
    wyckPos{'e',6,"0,0,z"},
    wyckPos{'f',6,"0.5,0,z"},
    wyckPos{'g',6,"x,0,0"},
    wyckPos{'h',6,"x,0,0.5"},
    wyckPos{'i',6,"x,2x,0"},
    wyckPos{'j',6,"x,2x,0.5"},
    wyckPos{'k',12,"x,y,z"}
  },

  { // 182
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.25"},
    wyckPos{'c',2,"0.333333,0.666667,0.25"},
    wyckPos{'d',2,"0.333333,0.666667,0.75"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0.333333,0.666667,z"},
    wyckPos{'g',6,"x,0,0"},
    wyckPos{'h',6,"x,2x,0.25"},
    wyckPos{'i',12,"x,y,z"}
  },

  { // 183
    wyckPos{'a',1,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',3,"0.5,0,z"},
    wyckPos{'d',6,"x,0,z"},
    wyckPos{'e',6,"x,-x,z"},
    wyckPos{'f',12,"x,y,z"}
  },

  { // 184
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0.333333,0.666667,z"},
    wyckPos{'c',6,"0.5,0,z"},
    wyckPos{'d',12,"x,y,z"}
  },

  { // 185
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',4,"0.333333,0.666667,z"},
    wyckPos{'c',6,"x,0,z"},
    wyckPos{'d',12,"x,y,z"}
  },

  { // 186
    wyckPos{'a',2,"0,0,z"},
    wyckPos{'b',2,"0.333333,0.666667,z"},
    wyckPos{'c',6,"x,-x,z"},
    wyckPos{'d',12,"x,y,z"}
  },

  { // 187
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',1,"0.333333,0.666667,0"},
    wyckPos{'d',1,"0.333333,0.666667,0.5"},
    wyckPos{'e',1,"0.666667,0.333333,0"},
    wyckPos{'f',1,"0.666667,0.333333,0.5"},
    wyckPos{'g',2,"0,0,z"},
    wyckPos{'h',2,"0.333333,0.666667,z"},
    wyckPos{'i',2,"0.666667,0.333333,z"},
    wyckPos{'j',3,"x,-x,0"},
    wyckPos{'k',3,"x,-x,0.5"},
    wyckPos{'l',6,"x,y,0"},
    wyckPos{'m',6,"x,y,0.5"},
    wyckPos{'n',6,"x,-x,z"},
    wyckPos{'o',12,"x,y,z"}
  },

  { // 188
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.25"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.25"},
    wyckPos{'e',2,"0.666667,0.333333,0"},
    wyckPos{'f',2,"0.666667,0.333333,0.25"},
    wyckPos{'g',4,"0,0,z"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',4,"0.666667,0.333333,z"},
    wyckPos{'j',6,"x,-x,0"},
    wyckPos{'k',6,"x,y,0.25"},
    wyckPos{'l',12,"x,y,z"}
  },

  { // 189
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',3,"x,0,0"},
    wyckPos{'g',3,"x,0,0.5"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',6,"x,0,z"},
    wyckPos{'j',6,"x,y,0"},
    wyckPos{'k',6,"x,y,0.5"},
    wyckPos{'l',12,"x,y,z"}
  },

  { // 190
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.25"},
    wyckPos{'c',2,"0.333333,0.666667,0.25"},
    wyckPos{'d',2,"0.666667,0.333333,0.25"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0.333333,0.666667,z"},
    wyckPos{'g',6,"x,0,0"},
    wyckPos{'h',6,"x,y,0.25"},
    wyckPos{'i',12,"x,y,z"}
  },

  { // 191
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0,0,0.5"},
    wyckPos{'c',2,"0.333333,0.666667,0"},
    wyckPos{'d',2,"0.333333,0.666667,0.5"},
    wyckPos{'e',2,"0,0,z"},
    wyckPos{'f',3,"0.5,0,0"},
    wyckPos{'g',3,"0.5,0,0.5"},
    wyckPos{'h',4,"0.333333,0.666667,z"},
    wyckPos{'i',6,"0.5,0,z"},
    wyckPos{'j',6,"x,0,0"},
    wyckPos{'k',6,"x,0,0.5"},
    wyckPos{'l',6,"x,2x,0"},
    wyckPos{'m',6,"x,2x,0.5"},
    wyckPos{'n',12,"x,0,z"},
    wyckPos{'o',12,"x,2x,z"},
    wyckPos{'p',12,"x,y,0"},
    wyckPos{'q',12,"x,y,0.5"},
    wyckPos{'r',24,"x,y,z"}
  },

  { // 192
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',4,"0.333333,0.666667,0.25"},
    wyckPos{'d',4,"0.333333,0.666667,0"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',6,"0.5,0,0.25"},
    wyckPos{'g',6,"0.5,0,0"},
    wyckPos{'h',8,"0.333333,0.666667,z"},
    wyckPos{'i',12,"0.5,0,z"},
    wyckPos{'j',12,"x,0,0.25"},
    wyckPos{'k',12,"x,2x,0.25"},
    wyckPos{'l',12,"x,y,0"},
    wyckPos{'m',24,"x,y,z"}
  },

  { // 193
    wyckPos{'a',2,"0,0,0.25"},
    wyckPos{'b',2,"0,0,0"},
    wyckPos{'c',4,"0.333333,0.666667,0.25"},
    wyckPos{'d',4,"0.333333,0.666667,0"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',6,"0.5,0,0"},
    wyckPos{'g',6,"x,0,0.25"},
    wyckPos{'h',8,"0.333333,0.666667,z"},
    wyckPos{'i',12,"x,2x,0"},
    wyckPos{'j',12,"x,y,0.25"},
    wyckPos{'k',12,"x,0,z"},
    wyckPos{'l',24,"x,y,z"}
  },

  { // 194
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',2,"0,0,0.25"},
    wyckPos{'c',2,"0.333333,0.666667,0.25"},
    wyckPos{'d',2,"0.333333,0.666667,0.75"},
    wyckPos{'e',4,"0,0,z"},
    wyckPos{'f',4,"0.333333,0.666667,z"},
    wyckPos{'g',6,"0.5,0,0"},
    wyckPos{'h',6,"x,2x,0.25"},
    wyckPos{'i',12,"x,0,0"},
    wyckPos{'j',12,"x,y,0.25"},
    wyckPos{'k',12,"x,2x,z"},
    wyckPos{'l',24,"x,y,z"}
  },

  { // 195
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',3,"0,0.5,0.5"},
    wyckPos{'d',3,"0.5,0,0"},
    wyckPos{'e',4,"x,x,x"},
    wyckPos{'f',6,"x,0,0"},
    wyckPos{'g',6,"x,0,0.5"},
    wyckPos{'h',6,"x,0.5,0"},
    wyckPos{'i',6,"x,0.5,0.5"},
    wyckPos{'j',12,"x,y,z"}
  },

  { // 196
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.75,0.75,0.75"},
    wyckPos{'e',16,"x,x,x"},
    wyckPos{'f',24,"x,0,0"},
    wyckPos{'g',24,"x,0.25,0.25"},
    wyckPos{'h',48,"x,y,z"}
  },

  { // 197
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',8,"x,x,x"},
    wyckPos{'d',12,"x,0,0"},
    wyckPos{'e',12,"x,0.5,0"},
    wyckPos{'f',24,"x,y,z"}
  },

  { // 198
    wyckPos{'a',4,"x,x,x"},
    wyckPos{'b',12,"x,y,z"}
  },

  { // 199
    wyckPos{'a',8,"x,x,x"},
    wyckPos{'b',12,"x,0,0.25"},
    wyckPos{'c',24,"x,y,z"}
  },

  { // 200
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',3,"0,0.5,0.5"},
    wyckPos{'d',3,"0.5,0,0"},
    wyckPos{'e',6,"x,0,0"},
    wyckPos{'f',6,"x,0,0.5"},
    wyckPos{'g',6,"x,0.5,0"},
    wyckPos{'h',6,"x,0.5,0.5"},
    wyckPos{'i',8,"x,x,x"},
    wyckPos{'j',12,"0,y,z"},
    wyckPos{'k',12,"0.5,y,z"},
    wyckPos{'l',24,"x,y,z"}
  },

  { // 201
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',4,"0,0,0"},
    wyckPos{'c',4,"0.5,0.5,0.5"},
    wyckPos{'d',6,"0.25,0.75,0.75"},
    wyckPos{'e',8,"x,x,x"},
    wyckPos{'f',12,"x,0.25,0.25"},
    wyckPos{'g',12,"x,0.75,0.25"},
    wyckPos{'h',24,"x,y,z"}
  },

  { // 202
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',24,"0,0.25,0.25"},
    wyckPos{'e',24,"x,0,0"},
    wyckPos{'f',32,"x,x,x"},
    wyckPos{'g',48,"x,0.25,0.25"},
    wyckPos{'h',48,"0,y,z"},
    wyckPos{'i',96,"x,y,z"}
  },

  { // 203
    wyckPos{'a',8,"0.125,0.125,0.125"},
    wyckPos{'b',8,"0.625,0.625,0.625"},
    wyckPos{'c',16,"0,0,0"},
    wyckPos{'d',16,"0.5,0.5,0.5"},
    wyckPos{'e',32,"x,x,x"},
    wyckPos{'f',48,"x,0.125,0.125"},
    wyckPos{'g',96,"x,y,z"}
  },

  { // 204
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',12,"x,0,0"},
    wyckPos{'e',12,"x,0,0.5"},
    wyckPos{'f',16,"x,x,x"},
    wyckPos{'g',24,"0,y,z"},
    wyckPos{'h',48,"x,y,z"}
  },

  { // 205
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',8,"x,x,x"},
    wyckPos{'d',24,"x,y,z"}
  },

  { // 206
    wyckPos{'a',8,"0,0,0"},
    wyckPos{'b',8,"0.25,0.25,0.25"},
    wyckPos{'c',16,"x,x,x"},
    wyckPos{'d',24,"x,0,0.25"},
    wyckPos{'e',48,"x,y,z"}
  },

  { // 207
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',3,"0,0.5,0.5"},
    wyckPos{'d',3,"0.5,0,0"},
    wyckPos{'e',6,"x,0,0"},
    wyckPos{'f',6,"x,0.5,0.5"},
    wyckPos{'g',8,"x,x,x"},
    wyckPos{'h',12,"x,0.5,0"},
    wyckPos{'i',12,"0,y,y"},
    wyckPos{'j',12,"0.5,y,y"},
    wyckPos{'k',24,"x,y,z"}
  },

  { // 208
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',4,"0.25,0.25,0.25"},
    wyckPos{'c',4,"0.75,0.75,0.75"},
    wyckPos{'d',6,"0,0.5,0.5"},
    wyckPos{'e',6,"0.25,0,0.5"},
    wyckPos{'f',6,"0.25,0.5,0"},
    wyckPos{'g',8,"x,x,x"},
    wyckPos{'h',12,"x,0,0"},
    wyckPos{'i',12,"x,0,0.5"},
    wyckPos{'j',12,"x,0.5,0"},
    wyckPos{'k',12,"0.25,y,-y+0.5"},
    wyckPos{'l',12,"0.25,y,y+0.5"},
    wyckPos{'m',24,"x,y,z"}
  },

  { // 209
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',24,"0,0.25,0.25"},
    wyckPos{'e',24,"x,0,0"},
    wyckPos{'f',32,"x,x,x"},
    wyckPos{'g',48,"0,y,y"},
    wyckPos{'h',48,"0.5,y,y"},
    wyckPos{'i',48,"x,0.25,0.25"},
    wyckPos{'j',96,"x,y,z"}
  },

  { // 210
    wyckPos{'a',8,"0,0,0"},
    wyckPos{'b',8,"0.5,0.5,0.5"},
    wyckPos{'c',16,"0.125,0.125,0.125"},
    wyckPos{'d',16,"0.625,0.625,0.625"},
    wyckPos{'e',32,"x,x,x"},
    wyckPos{'f',48,"x,0,0"},
    wyckPos{'g',48,"0.125,y,-y+0.25"},
    wyckPos{'h',96,"x,y,z"}
  },

  { // 211
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',12,"0.25,0.5,0"},
    wyckPos{'e',12,"x,0,0"},
    wyckPos{'f',16,"x,x,x"},
    wyckPos{'g',24,"x,0.5,0"},
    wyckPos{'h',24,"0,y,y"},
    wyckPos{'i',24,"0.25,y,-y+0.5"},
    wyckPos{'j',48,"x,y,z"}
  },

  { // 212
    wyckPos{'a',4,"0.125,0.125,0.125"},
    wyckPos{'b',4,"0.625,0.625,0.625"},
    wyckPos{'c',8,"x,x,x"},
    wyckPos{'d',12,"0.125,y,-y+0.25"},
    wyckPos{'e',24,"x,y,z"}
  },

  { // 213
    wyckPos{'a',4,"0.375,0.375,0.375"},
    wyckPos{'b',4,"0.875,0.875,0.875"},
    wyckPos{'c',8,"x,x,x"},
    wyckPos{'d',12,"0.125,y,y+0.25"},
    wyckPos{'e',24,"x,y,z"}
  },

  { // 214
    wyckPos{'a',8,"0.125,0.125,0.125"},
    wyckPos{'b',8,"0.875,0.875,0.875"},
    wyckPos{'c',12,"0.125,0,0.25"},
    wyckPos{'d',12,"0.625,0,0.25"},
    wyckPos{'e',16,"x,x,x"},
    wyckPos{'f',24,"x,0,0.25"},
    wyckPos{'g',24,"0.125,y,y+0.25"},
    wyckPos{'h',24,"0.125,y,-y+0.25"},
    wyckPos{'i',48,"x,y,z"}
  },

  { // 215
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',3,"0,0.5,0.5"},
    wyckPos{'d',3,"0.5,0,0"},
    wyckPos{'e',4,"x,x,x"},
    wyckPos{'f',6,"x,0,0"},
    wyckPos{'g',6,"x,0.5,0.5"},
    wyckPos{'h',12,"x,0.5,0"},
    wyckPos{'i',12,"x,x,z"},
    wyckPos{'j',24,"x,y,z"}
  },

  { // 216
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',4,"0.25,0.25,0.25"},
    wyckPos{'d',4,"0.75,0.75,0.75"},
    wyckPos{'e',16,"x,x,x"},
    wyckPos{'f',24,"x,0,0"},
    wyckPos{'g',24,"x,0.25,0.25"},
    wyckPos{'h',48,"x,x,z"},
    wyckPos{'i',96,"x,y,z"}
  },

  { // 217
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',8,"x,x,x"},
    wyckPos{'d',12,"0.25,0.5,0"},
    wyckPos{'e',12,"x,0,0"},
    wyckPos{'f',24,"x,0.5,0"},
    wyckPos{'g',24,"x,x,z"},
    wyckPos{'h',48,"x,y,z"}
  },

  { // 218
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',6,"0.25,0.5,0"},
    wyckPos{'d',6,"0.25,0,0.5"},
    wyckPos{'e',8,"x,x,x"},
    wyckPos{'f',12,"x,0,0"},
    wyckPos{'g',12,"x,0.5,0"},
    wyckPos{'h',12,"x,0,0.5"},
    wyckPos{'i',24,"x,y,z"}
  },

  { // 219
    wyckPos{'a',8,"0,0,0"},
    wyckPos{'b',8,"0.25,0.25,0.25"},
    wyckPos{'c',24,"0,0.25,0.25"},
    wyckPos{'d',24,"0.25,0,0"},
    wyckPos{'e',32,"x,x,x"},
    wyckPos{'f',48,"x,0,0"},
    wyckPos{'g',48,"x,0.25,0.25"},
    wyckPos{'h',96,"x,y,z"}
  },

  { // 220
    wyckPos{'a',12,"0.375,0,0.25"},
    wyckPos{'b',12,"0.875,0,0.25"},
    wyckPos{'c',16,"x,x,x"},
    wyckPos{'d',24,"x,0,0.25"},
    wyckPos{'e',48,"x,y,z"}
  },

  { // 221
    wyckPos{'a',1,"0,0,0"},
    wyckPos{'b',1,"0.5,0.5,0.5"},
    wyckPos{'c',3,"0,0.5,0.5"},
    wyckPos{'d',3,"0.5,0,0"},
    wyckPos{'e',6,"x,0,0"},
    wyckPos{'f',6,"x,0.5,0.5"},
    wyckPos{'g',8,"x,x,x"},
    wyckPos{'h',12,"x,0.5,0"},
    wyckPos{'i',12,"0,y,y"},
    wyckPos{'j',12,"0.5,y,y"},
    wyckPos{'k',24,"0,y,z"},
    wyckPos{'l',24,"0.5,y,z"},
    wyckPos{'m',24,"x,x,z"},
    wyckPos{'n',48,"x,y,z"}
  },

  { // 222
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',6,"0.75,0.25,0.25"},
    wyckPos{'c',8,"0,0,0"},
    wyckPos{'d',12,"0,0.75,0.25"},
    wyckPos{'e',12,"x,0.25,0.25"},
    wyckPos{'f',16,"x,x,x"},
    wyckPos{'g',24,"x,0.75,0.25"},
    wyckPos{'h',24,"0.25,y,y"},
    wyckPos{'i',48,"x,y,z"}
  },

  { // 223
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',6,"0.25,0,0.5"},
    wyckPos{'d',6,"0.25,0.5,0"},
    wyckPos{'e',8,"0.25,0.25,0.25"},
    wyckPos{'f',12,"x,0,0"},
    wyckPos{'g',12,"x,0,0.5"},
    wyckPos{'h',12,"x,0.5,0"},
    wyckPos{'i',16,"x,x,x"},
    wyckPos{'j',24,"0.25,y,y+0.5"},
    wyckPos{'k',24,"0,y,z"},
    wyckPos{'l',48,"x,y,z"}
  },

  { // 224
    wyckPos{'a',2,"0.25,0.25,0.25"},
    wyckPos{'b',4,"0,0,0"},
    wyckPos{'c',4,"0.5,0.5,0.5"},
    wyckPos{'d',6,"0.25,0.75,0.75"},
    wyckPos{'e',8,"x,x,x"},
    wyckPos{'f',12,"0.5,0.25,0.75"},
    wyckPos{'g',12,"x,0.25,0.25"},
    wyckPos{'h',24,"x,0.25,0.75"},
    wyckPos{'i',24,"0.5,y,y+0.5"},
    wyckPos{'j',24,"0.5,y,-y"},
    wyckPos{'k',24,"x,x,z"},
    wyckPos{'l',48,"x,y,z"}
  },

  { // 225
    wyckPos{'a',4,"0,0,0"},
    wyckPos{'b',4,"0.5,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',24,"0,0.25,0.25"},
    wyckPos{'e',24,"x,0,0"},
    wyckPos{'f',32,"x,x,x"},
    wyckPos{'g',48,"x,0.25,0.25"},
    wyckPos{'h',48,"0,y,y"},
    wyckPos{'i',48,"0.5,y,y"},
    wyckPos{'j',96,"0,y,z"},
    wyckPos{'k',96,"x,x,z"},
    wyckPos{'l',192,"x,y,z"}
  },

  { // 226
    wyckPos{'a',8,"0.25,0.25,0.25"},
    wyckPos{'b',8,"0,0,0"},
    wyckPos{'c',24,"0.25,0,0"},
    wyckPos{'d',24,"0,0.25,0.25"},
    wyckPos{'e',48,"x,0,0"},
    wyckPos{'f',48,"x,0.25,0.25"},
    wyckPos{'g',64,"x,x,x"},
    wyckPos{'h',96,"0.25,y,y"},
    wyckPos{'i',96,"0,y,z"},
    wyckPos{'j',192,"x,y,z"}
  },

  { // 227
    wyckPos{'a',8,"0.125,0.125,0.125"},
    wyckPos{'b',8,"0.375,0.375,0.375"},
    wyckPos{'c',16,"0,0,0"},
    wyckPos{'d',16,"0.5,0.5,0.5"},
    wyckPos{'e',32,"x,x,x"},
    wyckPos{'f',48,"x,0.125,0.125"},
    wyckPos{'g',96,"x,x,z"},
    wyckPos{'h',96,"0,y,-y"},
    wyckPos{'i',192,"x,y,z"}
  },

  { // 228
    wyckPos{'a',16,"0.125,0.125,0.125"},
    wyckPos{'b',32,"0.25,0.25,0.25"},
    wyckPos{'c',32,"0,0,0"},
    wyckPos{'d',48,"0.875,0.125,0.125"},
    wyckPos{'e',64,"x,x,x"},
    wyckPos{'f',96,"x,0.125,0.125"},
    wyckPos{'g',96,"0.25,y,-y"},
    wyckPos{'h',192,"x,y,z"}
  },

  { // 229
    wyckPos{'a',2,"0,0,0"},
    wyckPos{'b',6,"0,0.5,0.5"},
    wyckPos{'c',8,"0.25,0.25,0.25"},
    wyckPos{'d',12,"0.25,0,0.5"},
    wyckPos{'e',12,"x,0,0"},
    wyckPos{'f',16,"x,x,x"},
    wyckPos{'g',24,"x,0,0.5"},
    wyckPos{'h',24,"0,y,y"},
    wyckPos{'i',48,"0.25,y,-y+0.5"},
    wyckPos{'j',48,"0,y,z"},
    wyckPos{'k',48,"x,x,z"},
    wyckPos{'l',96,"x,y,z"}
  },

  { // 230
    wyckPos{'a',16,"0,0,0"},
    wyckPos{'b',16,"0.125,0.125,0.125"},
    wyckPos{'c',24,"0.125,0,0.25"},
    wyckPos{'d',24,"0.375,0,0.25"},
    wyckPos{'e',32,"x,x,x"},
    wyckPos{'f',48,"x,0,0.25"},
    wyckPos{'g',48,"0.125,y,-y+0.25"},
    wyckPos{'h',96,"x,y,z"}
  }

};

#endif
