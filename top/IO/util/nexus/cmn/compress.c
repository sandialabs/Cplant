/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

#include "cmn.h"
#include "queue.h"

IDENTIFY("$Id: compress.c,v 0.1 2001/07/18 18:57:26 rklundt Exp $");

/*
 * Implementation of BSTW compression using Fibonnaci codes.
 *
 * Lee Ward; Wed Nov 10 16:48:00 MST 1999
 */

#if NBBY != 8
#error Sorry, only 8-bit bytes are supported.
#endif

/*
 * The compression stream ends when the defined position is encountered.
 */
#define STREAM_END	257

struct _code_stream {
	int	_flags;					/* see below */
	char	*_buf;					/* work buffer */
	size_t	_resid;					/* remaining in _buf */
	unsigned char _cbuf;				/* current char */
	size_t	_cbuflen;				/* bits in _cbuf */
	int	_err;					/* last error */
};
typedef struct _code_stream CODESTREAM;			/* overkill */

#define CODESTREAM_F_READ	0x01			/* read stream */
#define CODESTREAM_F_WRITE	0x02			/* write stream */
#define CODESTREAM_F_ERROR	0x04			/* stream err'd? */

/*
 * Put a code stream into the error state.
 */
#define CODESTREAM_SET_ERROR(cstream, err) \
	do { \
		(cstream)->_err = (err); \
		(cstream)->_flags |= CODESTREAM_F_ERROR; \
	} while (0)

/*
 * Init CODESTREAM stream descriptor.
 */
#define csopen(cstream, flags, buf, bufsiz) \
	do { \
		(cstream)->_flags = \
		   (flags) & (CODESTREAM_F_READ|CODESTREAM_F_WRITE); \
		(cstream)->_buf = (buf); \
		(cstream)->_resid = (bufsiz); \
		(cstream)->_cbuf = 0; \
		(cstream)->_cbuflen = 0; \
		(cstream)->_err = 0; \
	} while (0)

/*
 * Return error or 0, if none, from given code stream.
 */
#define cserror(cstream) \
	((cstream)->_flags & CODESTREAM_F_ERROR ? (cstream)->_err : 0)

/*
 * A list message_list_entry.
 */
struct message_list_entry {
	unsigned char c;				/* message */
	LIST_ENTRY(message_list_entry) next;				/* -> next in list */
};

/*
 * Table of Fibonnaci codes from 1 to 257.
 */
static unsigned codes[] = {
	0x001,						/*   1 */
	0x002,						/*   2 */
	0x004,						/*   3 */
	0x005,						/*   4 */
	0x008,						/*   5 */
	0x009,						/*   6 */
	0x00a,						/*   7 */
	0x010,						/*   8 */
	0x011,						/*   9 */
	0x012,						/*  10 */
	0x014,						/*  11 */
	0x015,						/*  12 */
	0x020,						/*  13 */
	0x021,						/*  14 */
	0x022,						/*  15 */
	0x024,						/*  16 */
	0x025,						/*  17 */
	0x028,						/*  18 */
	0x029,						/*  19 */
	0x02a,						/*  20 */
	0x040,						/*  21 */
	0x041,						/*  22 */
	0x042,						/*  23 */
	0x044,						/*  24 */
	0x045,						/*  25 */
	0x048,						/*  26 */
	0x049,						/*  27 */
	0x04a,						/*  28 */
	0x050,						/*  29 */
	0x051,						/*  30 */
	0x052,						/*  31 */
	0x054,						/*  32 */
	0x055,						/*  33 */
	0x080,						/*  34 */
	0x081,						/*  35 */
	0x082,						/*  36 */
	0x084,						/*  37 */
	0x085,						/*  38 */
	0x088,						/*  39 */
	0x089,						/*  40 */
	0x08a,						/*  41 */
	0x090,						/*  42 */
	0x091,						/*  43 */
	0x092,						/*  44 */
	0x094,						/*  45 */
	0x095,						/*  46 */
	0x0a0,						/*  47 */
	0x0a1,						/*  48 */
	0x0a2,						/*  49 */
	0x0a4,						/*  50 */
	0x0a5,						/*  51 */
	0x0a8,						/*  52 */
	0x0a9,						/*  53 */
	0x0aa,						/*  54 */
	0x100,						/*  55 */
	0x101,						/*  56 */
	0x102,						/*  57 */
	0x104,						/*  58 */
	0x105,						/*  59 */
	0x108,						/*  60 */
	0x109,						/*  61 */
	0x10a,						/*  62 */
	0x110,						/*  63 */
	0x111,						/*  64 */
	0x112,						/*  65 */
	0x114,						/*  66 */
	0x115,						/*  67 */
	0x120,						/*  68 */
	0x121,						/*  69 */
	0x122,						/*  70 */
	0x124,						/*  71 */
	0x125,						/*  72 */
	0x128,						/*  73 */
	0x129,						/*  74 */
	0x12a,						/*  75 */
	0x140,						/*  76 */
	0x141,						/*  77 */
	0x142,						/*  78 */
	0x144,						/*  79 */
	0x145,						/*  80 */
	0x148,						/*  81 */
	0x149,						/*  82 */
	0x14a,						/*  83 */
	0x150,						/*  84 */
	0x151,						/*  85 */
	0x152,						/*  86 */
	0x154,						/*  87 */
	0x155,						/*  88 */
	0x200,						/*  89 */
	0x201,						/*  90 */
	0x202,						/*  91 */
	0x204,						/*  92 */
	0x205,						/*  93 */
	0x208,						/*  94 */
	0x209,						/*  95 */
	0x20a,						/*  96 */
	0x210,						/*  97 */
	0x211,						/*  98 */
	0x212,						/*  99 */
	0x214,						/* 100 */
	0x215,						/* 101 */
	0x220,						/* 102 */
	0x221,						/* 103 */
	0x222,						/* 104 */
	0x224,						/* 105 */
	0x225,						/* 106 */
	0x228,						/* 107 */
	0x229,						/* 108 */
	0x22a,						/* 109 */
	0x240,						/* 110 */
	0x241,						/* 111 */
	0x242,						/* 112 */
	0x244,						/* 113 */
	0x245,						/* 114 */
	0x248,						/* 115 */
	0x249,						/* 116 */
	0x24a,						/* 117 */
	0x250,						/* 118 */
	0x251,						/* 119 */
	0x252,						/* 120 */
	0x254,						/* 121 */
	0x255,						/* 122 */
	0x280,						/* 123 */
	0x281,						/* 124 */
	0x282,						/* 125 */
	0x284,						/* 126 */
	0x285,						/* 127 */
	0x288,						/* 128 */
	0x289,						/* 129 */
	0x28a,						/* 130 */
	0x290,						/* 131 */
	0x291,						/* 132 */
	0x292,						/* 133 */
	0x294,						/* 134 */
	0x295,						/* 135 */
	0x2a0,						/* 136 */
	0x2a1,						/* 137 */
	0x2a2,						/* 138 */
	0x2a4,						/* 139 */
	0x2a5,						/* 140 */
	0x2a8,						/* 141 */
	0x2a9,						/* 142 */
	0x2aa,						/* 143 */
	0x400,						/* 144 */
	0x401,						/* 145 */
	0x402,						/* 146 */
	0x404,						/* 147 */
	0x405,						/* 148 */
	0x408,						/* 149 */
	0x409,						/* 150 */
	0x40a,						/* 151 */
	0x410,						/* 152 */
	0x411,						/* 153 */
	0x412,						/* 154 */
	0x414,						/* 155 */
	0x415,						/* 156 */
	0x420,						/* 157 */
	0x421,						/* 158 */
	0x422,						/* 159 */
	0x424,						/* 160 */
	0x425,						/* 161 */
	0x428,						/* 162 */
	0x429,						/* 163 */
	0x42a,						/* 164 */
	0x440,						/* 165 */
	0x441,						/* 166 */
	0x442,						/* 167 */
	0x444,						/* 168 */
	0x445,						/* 169 */
	0x448,						/* 170 */
	0x449,						/* 171 */
	0x44a,						/* 172 */
	0x450,						/* 173 */
	0x451,						/* 174 */
	0x452,						/* 175 */
	0x454,						/* 176 */
	0x455,						/* 177 */
	0x480,						/* 178 */
	0x481,						/* 179 */
	0x482,						/* 180 */
	0x484,						/* 181 */
	0x485,						/* 182 */
	0x488,						/* 183 */
	0x489,						/* 184 */
	0x48a,						/* 185 */
	0x490,						/* 186 */
	0x491,						/* 187 */
	0x492,						/* 188 */
	0x494,						/* 189 */
	0x495,						/* 190 */
	0x4a0,						/* 191 */
	0x4a1,						/* 192 */
	0x4a2,						/* 193 */
	0x4a4,						/* 194 */
	0x4a5,						/* 195 */
	0x4a8,						/* 196 */
	0x4a9,						/* 197 */
	0x4aa,						/* 198 */
	0x500,						/* 199 */
	0x501,						/* 200 */
	0x502,						/* 201 */
	0x504,						/* 202 */
	0x505,						/* 203 */
	0x508,						/* 204 */
	0x509,						/* 205 */
	0x50a,						/* 206 */
	0x510,						/* 207 */
	0x511,						/* 208 */
	0x512,						/* 209 */
	0x514,						/* 210 */
	0x515,						/* 211 */
	0x520,						/* 212 */
	0x521,						/* 213 */
	0x522,						/* 214 */
	0x524,						/* 215 */
	0x525,						/* 216 */
	0x528,						/* 217 */
	0x529,						/* 218 */
	0x52a,						/* 219 */
	0x540,						/* 220 */
	0x541,						/* 221 */
	0x542,						/* 222 */
	0x544,						/* 223 */
	0x545,						/* 224 */
	0x548,						/* 225 */
	0x549,						/* 226 */
	0x54a,						/* 227 */
	0x550,						/* 228 */
	0x551,						/* 229 */
	0x552,						/* 230 */
	0x554,						/* 231 */
	0x555,						/* 232 */
	0x800,						/* 233 */
	0x801,						/* 234 */
	0x802,						/* 235 */
	0x804,						/* 236 */
	0x805,						/* 237 */
	0x808,						/* 238 */
	0x809,						/* 239 */
	0x80a,						/* 240 */
	0x810,						/* 241 */
	0x811,						/* 242 */
	0x812,						/* 243 */
	0x814,						/* 244 */
	0x815,						/* 245 */
	0x820,						/* 246 */
	0x821,						/* 247 */
	0x822,						/* 248 */
	0x824,						/* 249 */
	0x825,						/* 250 */
	0x828,						/* 251 */
	0x829,						/* 252 */
	0x82a,						/* 253 */
	0x840,						/* 254 */
	0x841,						/* 255 */
	0x842,						/* 256 */
	0x844						/* 257 */
};

/*
 * The first 12 fibonnaci numbers.
 */
static unsigned fib[] = {
	1,
	1,
	2,
	3,
	5,
	8,
	13,
	21,
	34,
	55,
	89,
	144,
	233
};

/*
 * Emit a bit into the code stream.
 */
static INLINE int
emit(int bit, CODESTREAM *cstream)
{

	/*
	 * This should check to see that this is a write stream. Too
	 * expensive though. Just ignore it.
	 */

	if (cstream->_flags & CODESTREAM_F_ERROR)
		return -1;

	cstream->_cbuf |= ((unsigned )bit & 1);
	if (++cstream->_cbuflen >= NBBY) {
		if (!cstream->_resid--) {
			CODESTREAM_SET_ERROR(cstream, ENOMEM);
			return -1;
		}
		*cstream->_buf++ = cstream->_cbuf;
		cstream->_cbuflen = 0;
		cstream->_cbuf = 0;
	} else
		cstream->_cbuf <<= 1;

	return 0;
}

/*
 * Flush (write) the code stream.
 */
static int
csflush(CODESTREAM *cstream)
{
	int	rtn;

	rtn = 0;
	while (rtn == 0 && cstream->_cbuflen)
		rtn = emit(0, cstream);
	return rtn;
}

/*
 * Encode and emit list position.
 */
static INLINE int
emit_position(unsigned pos, CODESTREAM *cstream)
{
	unsigned code;
	int	rtn;

	assert(pos < sizeof(codes) / sizeof(unsigned));
	code = codes[pos];
	do
		rtn = emit(code & 1, cstream);
	while ((code >>= 1) && rtn == 0);
	if (rtn == 0)
		rtn = emit(1, cstream);				/* stop bit */
	return rtn;
}

/*
 * Emit message.
 */
static INLINE int
emit_message(unsigned char c, CODESTREAM *cstream)
{
	int	rtn;

	rtn = emit(c & 0x80 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x40 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x20 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x10 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x08 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x04 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x02 ? 1 : 0, cstream);
	if (rtn == 0)
		rtn = emit(c & 0x01 ? 1 : 0, cstream);

	return rtn;
}

/*
 * Encode (compress) the source buffer into the destination buffer.
 * The destination length is updated to record how many bytes were
 * placed.
 *
 * Note: The destination buffer is updated always. Yes, even on error.
 */
int
imcompress(const char *src, size_t srclen, char *dest, size_t *destlenp)
{
	size_t	n;
	CODESTREAM cstream_record;
	LIST_HEAD(, message_list_entry) head;
	int	rtn;
	int	c;
	size_t	pos;
	struct message_list_entry *entries;
	struct message_list_entry *ep;

	n = 0;
	csopen(&cstream_record, CODESTREAM_F_WRITE, dest, *destlenp);
	entries = m_alloc((1 << NBBY) * sizeof(struct message_list_entry));
	if (entries == NULL)
		return ENOMEM;
	LIST_INIT(&head);
	rtn = 0;
	while (srclen--) {
		c = (unsigned )*src++;
		for (pos = 0, ep = LIST_FIRST(&head);
		     pos < n;
		     pos++, ep = LIST_NEXT(ep, next)) {
			assert(ep != NULL);
			if (ep->c == c)
				break;
		}
		rtn = emit_position(pos, &cstream_record);
		if (rtn != 0)
			break;
		if (pos >= n) {
			assert(n < (1 << NBBY));
			/*
			 * Not found in list. Emit the message too.
			 */
			rtn = emit_message(c, &cstream_record);
			if (rtn != 0)
				break;

			/*
			 * Also, put the new message on the front of the list.
			 */
			ep = &entries[n++];
			ep->c = c;
		} else
			LIST_REMOVE(ep, next);
		/*
		 * (Re)insert message_list_entry at the front.
		 */
		LIST_INSERT_HEAD(&head, ep, next);
	}

	/*
	 * Emit the end code.
	 */
	if (rtn == 0)
		rtn = emit_position(STREAM_END - 1, &cstream_record);

	/*
	 * Flush.
	 */
	if (rtn == 0)
		rtn = csflush(&cstream_record);

	free(entries);
	if (rtn == 0)
		*destlenp -= cstream_record._resid;
	return rtn ? cserror(&cstream_record) : 0;
}

/*
 * Return the next bit from the code stream or -1 if there are no more.
 */
static INLINE int
get_bit(CODESTREAM *cstream)
{
	int	isset;

	if (cstream->_flags & CODESTREAM_F_ERROR)
		return -1;
	if (!cstream->_cbuflen) {
		if (!cstream->_resid--) {
			CODESTREAM_SET_ERROR(cstream, 0);
			return -1;
		}
		cstream->_cbuf = *cstream->_buf++;
		cstream->_cbuflen = NBBY;
	}

	isset = cstream->_cbuf & (1 << (NBBY - 1));
	cstream->_cbuf <<= 1;
	cstream->_cbuflen--;

	return isset ? 1 : 0;
}

/*
 * Return (what is assumed to be) a message from the code stream.
 */
static int
get_message(CODESTREAM *cstream)
{
	unsigned value;
	int	rtn;

	value = (rtn = get_bit(cstream)) << 7;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 6;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 5;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 4;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 3;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 2;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream)) << 1;
	if (rtn >= 0)
		value |= (rtn = get_bit(cstream));

	return rtn >= 0 ? value : rtn;
}

/*
 * Decode (uncompress) a code stream.
 */
int
imuncompress(const char *src, size_t srclen, char *dest, size_t *destlenp)
{
	CODESTREAM cstream_record;
	unsigned n;
	int	bit;
	unsigned obit;
	unsigned bpos;
	unsigned pos;
	struct message_list_entry *entries;
	size_t	cc;
	LIST_HEAD(, message_list_entry) head;
	int	rtn;
	struct message_list_entry *ep;

	csopen(&cstream_record, CODESTREAM_F_READ, (char *)src, srclen);
	n = 0;
	obit = 0;
	bpos = 0;
	pos = 0;
	entries = m_alloc((1 << NBBY) * sizeof(struct message_list_entry));
	if (entries == NULL)
		return ENOMEM;
	LIST_INIT(&head);
	rtn = 0;
	cc = 0;
	while ((bit = get_bit(&cstream_record)) >= 0) {
		bpos++;
		if (obit && bit) {
			/*
			 * Have the code. Expand to position and
			 * write the message.
			 */
			if (pos > n) {
				if (pos >= STREAM_END)
					break;
				/*
				 * Don't have that one yet. Get it now.
				 */
				ep = &entries[n++];
				rtn = get_message(&cstream_record);
				if (rtn < 0)
					break;
				ep->c = rtn;
				rtn = 0;
			} else {
				ep = LIST_FIRST(&head);
				while (pos-- > 1)
					ep = LIST_NEXT(ep, next);
				LIST_REMOVE(ep, next);
			}
			/*
			 * (Re)insert at front.
			 */
			LIST_INSERT_HEAD(&head, ep, next);

			/*
			 * Deliver the message.
			 */
			if (cc++ >= *destlenp) {
				rtn = EINVAL;
				break;
			}
			*dest++ = ep->c;

			/*
			 * Set up for next code.
			 */
			bit = 0;
			bpos = 0;
			pos = 0;
		}

		if (bit)
			pos += fib[bpos];
		obit = bit;
	}

	free(entries);

	if (rtn == 0)
		rtn = bit;
	if (rtn != 0)
		rtn = cserror(&cstream_record);
	if (rtn == 0)
		*destlenp = cc;
	return rtn;
}
