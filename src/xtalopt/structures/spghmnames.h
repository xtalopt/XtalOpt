/**********************************************************************
  HMNames - Hermann-Mauguin name for a space group number

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_HMNAMES_H
#define XTALOPT_HMNAMES_H

#include <string>
#include <vector>

namespace XtalOpt {

static const std::vector<std::string> _HMNames = {
  "",             // 0 - not a real space group
  "P 1",          // 1
  "P -1",         // 2
  "P 1 2 1",      // 3
  "P 1 21 1",     // 4
  "C 1 2 1",      // 5
  "P 1 m 1",      // 6
  "P 1 c 1",      // 7
  "C 1 m 1",      // 8
  "C 1 c 1",      // 9
  "P 1 2/m 1",    // 10
  "P 1 21/m 1",   // 11
  "C 1 2/m 1",    // 12
  "P 1 2/c 1",    // 13
  "P 1 21/c 1",   // 14
  "C 1 2/c 1",    // 15
  "P 2 2 2",      // 16
  "P 2 2 21",     // 17
  "P 21 21 2",    // 18
  "P 21 21 21",   // 19
  "C 2 2 21",     // 20
  "C 2 2 2",      // 21
  "F 2 2 2",      // 22
  "I 2 2 2",      // 23
  "I 21 21 21",   // 24
  "P m m 2",      // 25
  "P m c 21",     // 26
  "P c c 2",      // 27
  "P m a 2",      // 28
  "P c a 21",     // 29
  "P n c 2",      // 30
  "P m n 21",     // 31
  "P b a 2",      // 32
  "P n a 21",     // 33
  "P n n 2",      // 34
  "C m m 2",      // 35
  "C m c 21",     // 36
  "C c c 2",      // 37
  "A m m 2",      // 38
  "A b m 2",      // 39
  "A m a 2",      // 40
  "A b a 2",      // 41
  "F m m 2",      // 42
  "F d d 2",      // 43
  "I m m 2",      // 44
  "I b a 2",      // 45
  "I m a 2",      // 46
  "P m m m",      // 47
  "P n n n:1",    // 48
  "P c c m",      // 49
  "P b a n",      // 50
  "P m m a",      // 51
  "P n n a",      // 52
  "P m n a",      // 53
  "P c c a",      // 54
  "P b a m",      // 55
  "P c c n",      // 56
  "P b c m",      // 57
  "P n n m",      // 58
  "P m m n",      // 59
  "P b c n",      // 60
  "P b c a",      // 61
  "P n m a",      // 62
  "C m c m",      // 63
  "C m c a",      // 64
  "C m m m",      // 65
  "C c c m",      // 66
  "C m m a",      // 67
  "C c c a",      // 68
  "F m m m",      // 69
  "F d d d:1",    // 70
  "I m m m",      // 71
  "I b a m",      // 72
  "I b c a",      // 73
  "I m m a",      // 74
  "P 4",          // 75
  "P 41",         // 76
  "P 42",         // 77
  "P 43",         // 78
  "I 4",          // 79
  "I 41",         // 80
  "P -4",         // 81
  "I -4",         // 82
  "P 4/m",        // 83
  "P 42/m",       // 84
  "P 4/n",        // 85
  "P 42/n",       // 86
  "I 4/m",        // 87
  "I 41/a",       // 88
  "P 4 2 2",      // 89
  "P 42 1 2",     // 90
  "P 41 2 2",     // 91
  "P 41 21 2",    // 92
  "P 42 2 2",     // 93
  "P 42 21 2",    // 94
  "P 43 2 2",     // 95
  "P 43 21 2",    // 96
  "I 4 2 2",      // 97
  "I 41 2 2",     // 98
  "P 4 m m",      // 99
  "P 4 b m",      // 100
  "P 42 c m",     // 101
  "P 42 n m",     // 102
  "P 4 c c",      // 103
  "P 4 n c",      // 104
  "P 42 m c",     // 105
  "P 42 b c",     // 106
  "I 4 m m",      // 107
  "I 4 c m",      // 108
  "I 41 m d",     // 109
  "I 41 c d",     // 110
  "P -4 2 m",     // 111
  "P -4 2 c",     // 112
  "P -4 21 m",    // 113
  "P -4 21 c",    // 114
  "P -4 m 2",     // 115
  "P -4 c 2",     // 116
  "P -4 b 2",     // 117
  "P -4 n 2",     // 118
  "I -4 m 2",     // 119
  "I -4 c 2",     // 120
  "I -4 2 m",     // 121
  "I -4 2 d",     // 122
  "P 4/m m m",    // 123
  "P 4/m c c",    // 124
  "P 4/n b m:1",  // 125
  "P 4/n n c:1",  // 126
  "P 4/m b m",    // 127
  "P 4/m n c",    // 128
  "P 4/n m m:1",  // 129
  "P 4/n c c:1",  // 130
  "P 42/m m c",   // 131
  "P 42/m c m",   // 132
  "P 42/n b c:1", // 133
  "P 42/n n m:1", // 134
  "P 42/m b c",   // 135
  "P 42/m n m",   // 136
  "P 42/n m c:1", // 137
  "P 42/n c m:1", // 138
  "I 4/m m m",    // 139
  "I 4/m c m",    // 140
  "I 41/a m d",   // 141
  "I 41/a c d",   // 142
  "P 3",          // 143
  "P 31",         // 144
  "P 32",         // 145
  "R 3",          // 146
  "P -3",         // 147
  "R -3",         // 148
  "P 3 1 2",      // 149
  "P 3 2 1",      // 150
  "P 31 1 2",     // 151
  "P 31 2 1",     // 152
  "P 32 1 2",     // 153
  "P 32 2 1",     // 154
  "R 32",         // 155
  "P 3 m 1",      // 156
  "P 3 1 m",      // 157
  "P 3 c 1",      // 158
  "P 3 1 c",      // 159
  "R 3 m",        // 160
  "R 3 c",        // 161
  "P -3 1 m",     // 162
  "P -3 1 c",     // 163
  "P -3 m 1",     // 164
  "P -3 c 1",     // 165
  "R -3 m:H",     // 166
  "R -3 c:H",     // 167
  "P 6",          // 168
  "P 61",         // 169
  "P 65",         // 170
  "P 62",         // 171
  "P 64",         // 172
  "P 63",         // 173
  "P -6",         // 174
  "P 6/m",        // 175
  "P 63/m",       // 176
  "P 6 2 2",      // 177
  "P 61 2 2",     // 178
  "P 65 2 2",     // 179
  "P 62 2 2",     // 180
  "P 64 2 2",     // 181
  "P 63 2 2",     // 182
  "P 6 m m",      // 183
  "P 6 c c",      // 184
  "P 63 c m",     // 185
  "P 63 m c",     // 186
  "P -6 m 2",     // 187
  "P -6 c 2",     // 188
  "P -6 2 m",     // 189
  "P -6 2 c",     // 190
  "P 6/m m m",    // 191
  "P 6/m c c",    // 192
  "P 63/m c m",   // 193
  "P 63/m m c",   // 194
  "P 2 3",        // 195
  "F 2 3",        // 196
  "I 2 3",        // 197
  "P 21 3",       // 198
  "I 21 3",       // 199
  "P m 3",        // 200
  "P n 3",        // 201
  "F m 3",        // 202
  "F d 3",        // 203
  "I m 3",        // 204
  "P a 3",        // 205
  "I a 3",        // 206
  "P 4 3 2",      // 207
  "P 42 3 2",     // 208
  "F 4 3 2",      // 209
  "F 41 3 2",     // 210
  "I 4 3 2",      // 211
  "P 43 3 2",     // 212
  "P 41 3 2",     // 213
  "I 41 3 2",     // 214
  "P -4 3 m",     // 215
  "F -4 3 m",     // 216
  "I -4 3 m",     // 217
  "P -4 3 n",     // 218
  "F -4 3 c",     // 219
  "I -4 3 d",     // 220
  "P m 3 m",      // 221
  "P n 3 n",      // 222
  "P m 3 n",      // 223
  "P n 3 m",      // 224
  "F m 3 m",      // 225
  "F m 3 c",      // 226
  "F d 3 m",      // 227
  "F d 3 c",      // 228
  "I m 3 m",      // 229
  "I a 3 d"       // 230
};

} // namespace XtalOpt

#endif
