/*
** $Id: lanai_device.h,v 1.7 2001/09/21 16:44:03 pumatst Exp $
*/
#ifndef LANAI_DEVICE_H
#define LANAI_DEVICE_H

#include <asm/byteorder.h>	/* For ntohl() and htonl() */
#include "myriInterface.h"

/*
** Globals in the LanaiDevice library
*/
extern volatile unsigned short *LANAI[16];
extern struct LANAI_REG *LANAI_SPECIAL[16];
extern struct MYRINET_EEPROM *LANAI_EEPROM[16];
extern volatile unsigned int *LANAI_BOARD[16];
extern volatile unsigned int *LANAI3[16];
extern volatile unsigned short *LANAI_CONTROL[16];
extern volatile unsigned int *UBLOCK[16];
extern volatile unsigned short *BOARD[16];


/*
** The LanaiDevice library calls it LANAI5_SPECIAL[], so we'll have to as
** well. We're really dealing with an LANai 7, though. We wont support L5
*/
extern struct LANAI7_REG *LANAI5_SPECIAL[16];

#define L0	(0)		/* Unknown/unsupported board */
#define L4	(400)		/* LANai 4.x board */
#define L7	(700)		/* LANai 7.x board */
#define L9	(900)		/* LANai 9.x board */


/*
** Functions in the LanaiDevice library
*/
typedef void (*lanai_mcp_initialize_callback) (void);

int lanai_load_and_reset (const unsigned unit, const char *filename);
unsigned int lanai_memory_size(const unsigned int unit);
int open_lanai_device(void);
int lanai_clear_memory(const unsigned int unit);



struct LANAI_REG {
    volatile unsigned int IPF0;		/* 5 context-0 state registers */
    volatile unsigned int CUR0;
    volatile unsigned int PREV0;
    volatile unsigned int DATA0;
    volatile unsigned int DPF0;
    volatile unsigned int IPF1;		/* 5 context-1 state registers */
    volatile unsigned int CUR1;
    volatile unsigned int PREV1;
    volatile unsigned int DATA1;
    volatile unsigned int DPF1;
    volatile unsigned int ISR;		/* interrupt status register */
    volatile unsigned int EIMR;		/* external-interrupt mask register */
    volatile unsigned int IT1;		/* interrupt timer name compatible
					   with LANai7 */
    volatile unsigned int RTC;		/* real-time clock */
    volatile unsigned int CKS;		/* checksum */
    volatile unsigned int EAR;		/* SBus-DMA exteral address */
    volatile unsigned int LAR;		/* SBus-DMA local address */
    volatile unsigned int DMA_CTR;	/* SBus-DMA counter */

    /*
    ** The next 5 are NOT defined as pointers so it'll still work
    ** on 64-bit machines
    */
    volatile unsigned int RMP;		/* receive-DMA pointer */
    volatile unsigned int RML;		/* receive-DMA limit */
    volatile unsigned int SMP;		/* send-DMA pointer */
    volatile unsigned int SML;		/* send-DMA limit */
    volatile unsigned int SMLT;		/* send-DMA limit with tail */

    unsigned int skip_0x5c;		/* skipped one word */
    volatile unsigned char RB;		/* receive byte */
    unsigned char skip_0x61;
    unsigned char skip_0x62;
    unsigned char skip_0x63;

    volatile unsigned short RH;		/* receive half-word */
    unsigned char skip_0x66;
    unsigned char skip_0x67;
    volatile unsigned int RW;		/* receive word */

    volatile unsigned int SA;		/* send align */

    volatile unsigned int SB;		/* single-send commands */
    volatile unsigned int SH;
    volatile unsigned int SW;
    volatile unsigned int ST;

    volatile unsigned int DMA_DIR;	/* SBus-DMA direction */
    volatile unsigned int DMA_STS;	/* SBus-DMA modes */
    volatile unsigned int TIMEOUT;
    volatile unsigned int MYRINET;

    volatile unsigned int HW_DEBUG;	/* hardware debugging */
    volatile unsigned int LED;		/* LED pin(s) */
    volatile unsigned int VERSION;	/* the ex-window-pins register */
    volatile unsigned int ACTIVATE;	/* activate Myrinet-link *//* 0x9C */

    unsigned int pad_a[(0xe4 - 0xa0) / sizeof(int)];

    volatile unsigned int SMC;		/* Fake SMC 0xE4 */
    volatile unsigned int IT0;		/* Fake IT0 0xE8 */
    volatile unsigned int IT2;		/* Fake IT2 0xEC */
    volatile unsigned int RMC;		/* Fake RMC 0xF0 */
    volatile unsigned int RMW;		/* Fake RMW 0xF4 */
    volatile unsigned int SMH;		/* Fake SMH 0xF8 */
    volatile unsigned int clock_val;	/* clock register 0xFC */
};

struct LANAI7_REG {
    volatile unsigned int pad0x0;		/*  0x00 */
    volatile unsigned int pad0x4;
    volatile unsigned int pad0x8;		/*  0x08 */
    volatile unsigned int pad0xc;
    volatile unsigned int pad0x10;		/*  0x10 */
    volatile unsigned int pad0x14;
    volatile unsigned int pad0x18;		/*  0x18 */
    volatile unsigned int pad0x1c;
    volatile unsigned int pad0x20;		/*  0x20 */
    volatile unsigned int pad0x24;
    volatile unsigned int pad0x28;		/*  0x28 */
    volatile unsigned int pad0x2c;
    volatile unsigned int pad0x30;		/*  0x30 */
    volatile unsigned int pad0x34;
    volatile unsigned int pad0x38;		/*  0x38 */
    volatile unsigned int pad0x3c;
    volatile unsigned int pad0x40;		/*  0x40 */
    volatile unsigned int pad0x44;
    volatile unsigned int pad0x48;		/*  0x48 */
    volatile unsigned int pad0x4c;
    volatile unsigned int 		ISR;	/*  0x50 */
    volatile unsigned int pad0x54;
    volatile unsigned int 		EIMR;	/*  0x58 */
    volatile unsigned int pad0x5c;
    volatile unsigned int 		IT0;	/*  0x60 */
    volatile unsigned int pad0x64;
    volatile unsigned int 		RTC;	/*  0x68 */
    volatile unsigned int pad0x6c;
    volatile unsigned int 		LAR;	/*  0x70 */
    volatile unsigned int pad0x74;
    volatile unsigned int 		DMA_CTR;/*  0x78 */
    volatile unsigned int pad0x7c;
    volatile unsigned int pad0x80;		/*  0x80 */
    volatile unsigned int pad0x84;
    volatile unsigned int pad0x88;		/*  0x88 */
    volatile unsigned int pad0x8c;
    volatile unsigned int pad0x90;		/*  0x90 */
    volatile unsigned int pad0x94;
    volatile unsigned int pad0x98;		/*  0x98 */
    volatile unsigned int pad0x9c;
    volatile unsigned int pad0xa0;		/*  0xA0 */
    volatile unsigned int pad0xa4;
    volatile unsigned int pad0xa8;		/*  0xA8 */
    volatile unsigned int pad0xac;
    volatile unsigned int pad0xb0;		/*  0xB0 */
    volatile unsigned int pad0xb4;
    volatile unsigned int 		PULSE;	/*  0xB8 */
    volatile unsigned int pad0xbc;			    
    volatile unsigned int 		IT1;	/*  0xC0 */
    volatile unsigned int pad0xc4;			    
    volatile unsigned int 		IT2;	/*  0xC8 */
    volatile unsigned int pad0xcc;			    
    volatile unsigned int 		RMW;	/*  0xD0 */
    volatile unsigned int pad0xd4;			    
    volatile unsigned int 		RMC;	/*  0xD8 */
    volatile unsigned int pad0xdc;			    
    volatile unsigned int 		RMP;	/*  0xE0 */
    volatile unsigned int pad0xe4;			    
    volatile unsigned int 		RML;	/*  0xE8 */
    volatile unsigned int pad0xec;			    
    volatile unsigned int 		SMP;	/*  0xF0 */
    volatile unsigned int pad0xf4;			    
    volatile unsigned int 		SMH;	/*  0xF8 */
    volatile unsigned int pad0xfc;			    
    volatile unsigned int 		SML;	/*  0x100 */
    volatile unsigned int pad0x104;			    
    volatile unsigned int 		SMLT;	/*  0x108 */
    volatile unsigned int pad0x10c;			    
    volatile unsigned int 		SMC;	/*  0x110 */
    volatile unsigned int pad0x114;			    
    volatile unsigned int 		SA;	/*  0x118 */
    volatile unsigned int pad0x11c;			    
    volatile unsigned int pad0x120;		/*  0x120 */
    volatile unsigned int pad0x124;			    
    volatile unsigned int 		TIMEOUT;/*  0x128 */
    volatile unsigned int pad0x12c;			    
    volatile unsigned int 		MYRINET;/*  0x130 */
    volatile unsigned int pad0x134;			    
    volatile unsigned int 		DEBUG;	/*  0x138 */
    volatile unsigned int pad0x13c;			    
    volatile unsigned int 		LED;	/*  0x140 */
    volatile unsigned int pad0x144;			    
    volatile unsigned int		VERSION;/*  Fake 0x148 */
    volatile unsigned int pad0x14c;			    
    volatile unsigned int 		MP;	/*  0x150 */
    volatile unsigned int pad0x154;
    volatile unsigned int pad0x158;		/*  0x158 */
    volatile unsigned int pad0x15c;
    volatile unsigned int pad0x160;		/*  0x160 */
    volatile unsigned int pad0x164;
    volatile unsigned int pad0x168;		/*  0x168 */
    volatile unsigned int pad0x16c;
    volatile unsigned int pad0x170;		/*  0x170 */
    volatile unsigned int pad0x174;
    volatile unsigned int pad0x178;		/*  0x178 */
    volatile unsigned int pad0x17c;
    volatile unsigned int pad0x180;		/*  0x180 */
    volatile unsigned int pad0x184;
    volatile unsigned int pad0x188;		/*  0x188 */
    volatile unsigned int pad0x18c;
    volatile unsigned int pad0x190;		/*  0x190 */
    volatile unsigned int pad0x194;
    volatile unsigned int pad0x198;		/*  0x198 */
    volatile unsigned int pad0x19c;
    volatile unsigned int pad0x1a0;		/*  0x1a0 */
    volatile unsigned int pad0x1a4;
    volatile unsigned int pad0x1a8;		/*  0x1a8 */
    volatile unsigned int pad0x1ac;
    volatile unsigned int pad0x1b0;		/*  0x1b0 */
    volatile unsigned int pad0x1b4;
    volatile unsigned int pad0x1b8;		/*  0x1b8 */
    volatile unsigned int pad0x1bc;
    volatile unsigned int pad0x1c0;		/*  0x1c0 */
    volatile unsigned int pad0x1c4;
    volatile unsigned int pad0x1c8;		/*  0x1c8 */
    volatile unsigned int pad0x1cc;
    volatile unsigned int pad0x1d0;		/*  0x1d0 */
    volatile unsigned int pad0x1d4;
    volatile unsigned int pad0x1d8;		/*  0x1d8 */
    volatile unsigned int pad0x1dc;
    volatile unsigned int pad0x1e0;		/*  0x1e0 */
    volatile unsigned int pad0x1e4;
    volatile unsigned int pad0x1e8;		/*  0x1e8 */
    volatile unsigned int pad0x1ec;
    volatile unsigned int		CKS;	/*  Fake 0x1f0 */
    volatile unsigned int		EAR;	/*  Fake 0x1f4 */
    volatile unsigned int 		CLOCK;	/*  0x1F8 */
};

struct LANAI9_REG {
    volatile unsigned int pad0x0;		/*  0x00 */
    volatile unsigned int pad0x4;
    volatile unsigned int pad0x8;		/*  0x08 */
    volatile unsigned int pad0xc;
    volatile unsigned int pad0x10;		/*  0x10 */
    volatile unsigned int pad0x14;
    volatile unsigned int pad0x18;		/*  0x18 */
    volatile unsigned int pad0x1c;
    volatile unsigned int pad0x20;		/*  0x20 */
    volatile unsigned int pad0x24;
    volatile unsigned int pad0x28;		/*  0x28 */
    volatile unsigned int pad0x2c;
    volatile unsigned int pad0x30;		/*  0x30 */
    volatile unsigned int pad0x34;
    volatile unsigned int pad0x38;		/*  0x38 */
    volatile unsigned int pad0x3c;
    volatile unsigned int pad0x40;		/*  0x40 */
    volatile unsigned int pad0x44;
    volatile unsigned int pad0x48;		/*  0x48 */
    volatile unsigned int pad0x4c;
    volatile unsigned int		ISR;	/*  0x50 */
    volatile unsigned int pad0x54;
    volatile unsigned int		EIMR;	/*  0x58 */
    volatile unsigned int pad0x5c;
    volatile unsigned int		IT0;	/*  0x60 */
    volatile unsigned int pad0x64;
    volatile unsigned int		RTC;	/*  0x68 */
    volatile unsigned int pad0x6c;
    volatile unsigned int		LAR;	/*  0x70 */
    volatile unsigned int pad0x74;
    volatile unsigned int		CPUC;	/*  0x78 */
    volatile unsigned int pad0x7c;
    volatile unsigned int pad0x80;		/*  0x80 */
    volatile unsigned int pad0x84;
    volatile unsigned int pad0x88;		/*  0x88 */
    volatile unsigned int pad0x8c;
    volatile unsigned int pad0x90;		/*  0x90 */
    volatile unsigned int pad0x94;
    volatile unsigned int pad0x98;		/*  0x98 */
    volatile unsigned int pad0x9c;
    volatile unsigned int pad0xa0;		/*  0xA0 */
    volatile unsigned int pad0xa4;
    volatile unsigned int pad0xa8;		/*  0xA8 */
    volatile unsigned int pad0xac;
    volatile unsigned int pad0xb0;		/*  0xB0 */
    volatile unsigned int pad0xb4;
    volatile unsigned int		PULSE;	/*  0xB8 */
    volatile unsigned int pad0xbc;
    volatile unsigned int		IT1;	/*  0xC0 */
    volatile unsigned int pad0xc4;
    volatile unsigned int		IT2;	/*  0xC8 */
    volatile unsigned int pad0xcc;
    volatile unsigned int		RMW;	/*  0xD0 */
    volatile unsigned int pad0xd4;
    volatile unsigned int		RMC;	/*  0xD8 */
    volatile unsigned int pad0xdc;
    volatile unsigned int		RMP;	/*  0xE0 */
    volatile unsigned int pad0xe4;
    volatile unsigned int		RML;	/*  0xE8 */
    volatile unsigned int pad0xec;
    volatile unsigned int		SMP;	/*  0xF0 */
    volatile unsigned int pad0xf4;
    volatile unsigned int		SMH;	/*  0xF8 */
    volatile unsigned int pad0xfc;
    volatile unsigned int		SML;	/*  0x100 */
    volatile unsigned int pad0x104;
    volatile unsigned int		SMLT;	/*  0x108 */
    volatile unsigned int pad0x10c;
    volatile unsigned int		SMC;	/*  0x110 */
    volatile unsigned int pad0x114;
    volatile unsigned int		SA;	/*  0x118 */
    volatile unsigned int pad0x11c;
    volatile unsigned int pad0x120;		/*  0x120 */
    volatile unsigned int pad0x124;
    volatile unsigned int pad0x128;		/*  0x128 */
    volatile unsigned int pad0x12c;
    volatile unsigned int		MYRINET;/*  0x130 */
    volatile unsigned int pad0x134;
    volatile unsigned int		DEBUG;	/*  0x138 */
    volatile unsigned int pad0x13c;
    volatile unsigned int		LED;	/*  0x140 */
    volatile unsigned int pad0x144;
    volatile unsigned int		ILLEGAL;/*  Fake 0x148 */
    volatile unsigned int pad0x14c;
    volatile unsigned int		GM_MP;	/*  0x150 */
    volatile unsigned int pad0x154;
    volatile unsigned int pad0x158;
    volatile unsigned int pad0x15c;
    volatile unsigned int pad0x160;
    volatile unsigned int pad0x164;
    volatile unsigned int pad0x168;
    volatile unsigned int pad0x16c;
    volatile unsigned int pad0x170;
    volatile unsigned int pad0x174;
    volatile unsigned int pad0x178;
    volatile unsigned int pad0x17c;
    volatile unsigned int pad0x180;
    volatile unsigned int pad0x184;
    volatile unsigned int pad0x188;
    volatile unsigned int pad0x18c;
    volatile unsigned int pad0x190;
    volatile unsigned int pad0x194;
    volatile unsigned int pad0x198;
    volatile unsigned int pad0x19c;
    volatile unsigned int pad0x1a0;
    volatile unsigned int pad0x1a4;
    volatile unsigned int pad0x1a8;
    volatile unsigned int pad0x1ac;
    volatile unsigned int pad0x1b0;
    volatile unsigned int pad0x1b4;
    volatile unsigned int pad0x1b8;
    volatile unsigned int pad0x1bc;
    volatile unsigned int pad0x1c0;
    volatile unsigned int pad0x1c4;
    volatile unsigned int pad0x1c8;
    volatile unsigned int pad0x1cc;
    volatile unsigned int pad0x1d0;
    volatile unsigned int pad0x1d4;
    volatile unsigned int pad0x1d8;
    volatile unsigned int pad0x1dc;
    volatile unsigned int pad0x1e0;
    volatile unsigned int pad0x1e4;
    volatile unsigned int pad0x1e8;
    volatile unsigned int pad0x1ec;
    volatile unsigned int pad0x1f0;
    volatile unsigned int pad0x1f4;
    volatile unsigned int		CLOCK;	/*  0x1F8 */
};

/*************************
 ** Interrupt bit names **
 *************************/

#define	DBG_BIT		0x80000000
#define	HOST_SIG_BIT	0x40000000

#define	LAN7_SIG_BIT	0x00800000
#define	LAN6_SIG_BIT	0x00400000
#define	LAN5_SIG_BIT	0x00200000
#define	LAN4_SIG_BIT	0x00100000
#define	LAN3_SIG_BIT	0x00080000
#define	LAN2_SIG_BIT	0x00040000
#define	LAN1_SIG_BIT	0x00020000
#define	LAN0_SIG_BIT	0x00010000
#define WORD_RDY_BIT    0x00008000
#define HALF_RDY_BIT    0x00004000
#define SEND_RDY_BIT    0x00002000
#define LINK_INT_BIT	0x00001000
#define NRES_INT_BIT	0x00000800
#define WAKE4_INT_BIT	0x00000400
#define OFF_BY_2_BIT	0x00000200
#define OFF_BY_1_BIT	0x00000100
#define TAIL_INT_BIT    0x00000080
#define WDOG_INT_BIT	0x00000040
#define TIME_INT_BIT	0x00000020
#define DMA_INT_BIT 	0x00000010
#define SEND_INT_BIT 	0x00000008
#define BUFF_INT_BIT 	0x00000004
#define RECV_INT_BIT 	0x00000002
#define BYTE_RDY_BIT    0x00000001

/*
** New bits for LANai 7 and some name changes
*/
#define LINK2_INT_BIT   0x04000000
#define LINK1_INT_BIT   0x02000000
#define LINK0_INT_BIT   0x01000000

#define PARITY_INT_BIT  0x00008000
#define MEMORY_INT_BIT  0x00004000
#define TIME2_INT_BIT   0x00002000
#define WAKE7_INT_BIT    0x00001000

#define OFF_BY_4_BIT    0x00000400

#define TIME1_INT_BIT   0x00000080
#define TIME0_INT_BIT   0x00000040
#define LAN9_SIG_BIT    0x00000020
#define LAN8_SIG_BIT    0x00000010

#define HEAD_INT_BIT    0x00000001



/*
** New bits for LANai 9 or name changes
*/
#define DEBUG_BIT		DBG_BIT
#define LANAI_SIG_BIT		(1<<29)
#define BEAT_MISSED_INT_BIT	(1<<24)
#define TX_BLOCKED_INT_BIT	(1<<23)
#define TX_TOO_LONG_INT_BIT	(1<<22)
#define RX_TOO_LONG_INT_BIT	(1<<21)
#define ILLEGAL_RECEIVED_INT_BIT (1<<20)
#define BEAT_RECEIVED_INT_BIT	(1<<19)
#define DATA_RECEIVED_INT_BIT	(1<<18)
#define DATA_SENT_INT_BIT	(1<<17)
#define LINK_DOWN_BIT		(1<<16)
#define PAR_INT_BIT		PARITY_INT_BIT
#define MEM_INT_BIT		MEMORY_INT_BIT
#define WAKE0_INT_BIT		WAKE7_INT_BIT
#define ORUN4_INT_BIT		OFF_BY_4_BIT
#define ORUN2_INT_BIT		OFF_BY_2_BIT
#define ORUN1_INT_BIT		OFF_BY_1_BIT
#define WAKE2_INT_BIT		(1<<5)
#define WAKE1_INT_BIT		(1<<4)






#define get_TEMPLATE(REG)					\
    extern __inline__ int					\
    get##REG(int unit, int LANaiType)				\
    {								\
	if ((LANaiType == L7) || (LANaiType == L9))   {		\
	    return ntohl(LANAI5_SPECIAL[unit]->REG);		\
	} else   {						\
	    return ntohl(LANAI_SPECIAL[unit]->REG);		\
	}							\
    }

#define SET_TEMPLATE(REG)					\
    extern __inline__ void					\
    set##REG(int unit, int LANaiType, int value)		\
    {								\
	if ((LANaiType == L7) || (LANaiType == L9))   {		\
	    LANAI5_SPECIAL[unit]->REG= htonl(value);		\
	} else   {						\
	    LANAI_SPECIAL[unit]->REG= htonl(value);		\
	}							\
    }

/* Registers not present on L9 */
get_TEMPLATE(DMA_CTR)

/* Registers not present on L7 & L9 */
get_TEMPLATE(EAR)
get_TEMPLATE(CKS)

/* Registers not present on L4 */
get_TEMPLATE(SMC)
get_TEMPLATE(SMH)
get_TEMPLATE(RMC)
get_TEMPLATE(RMW)
get_TEMPLATE(IT0)
get_TEMPLATE(IT2)

/* Common registers L4, L7, and L9 */
get_TEMPLATE(EIMR)
get_TEMPLATE(ISR)
get_TEMPLATE(LAR)
get_TEMPLATE(RMP)
get_TEMPLATE(RML)
get_TEMPLATE(SML)
get_TEMPLATE(SMLT)
get_TEMPLATE(RTC)
get_TEMPLATE(LED)
get_TEMPLATE(SMP)
get_TEMPLATE(IT1)


/* Registers not present on L7 & L9 */
SET_TEMPLATE(VERSION)

/* Common registers L4, L7, and L9 */
SET_TEMPLATE(EIMR)
SET_TEMPLATE(ISR)
SET_TEMPLATE(RTC)
SET_TEMPLATE(LED)
SET_TEMPLATE(SMP)


#define MLANAI_GET_INFO _IOR('M',0,struct board_info)
#ifndef OFFSETOF
    #define OFFSETOF(struct_type, elem)				\
	((unsigned int)(&((struct_type *)0)->elem))
#endif


#endif /* LANAI_DEVICE_H */
