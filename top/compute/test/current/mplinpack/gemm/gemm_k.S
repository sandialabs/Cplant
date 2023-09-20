    /*        Fast GEMM routine for Alpha 21164(A)         */
    /*         on  Linux, Digital UNIX and NT              */
    /*                         date : 98.11.07             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

/*
 Following documents are written by Kazushige Goto.  If you find
 mistakes or suggestions, please let me know.  

 If you're interested in Alpha Architecture, please refer to 21164
 Hardware reference manual.

 Abstract:
   This dgemm routine is optimized for 21164(A) and requires 96kB
   second internal cache,  and if possible,  more than 2MB 3rd
   external cache.  Of course, it works fine on 21164PC, 21064 or
   21066(A),  but it's slow.

 Usage:
   it's entirely compatible with BLAS dgemm.f routine except for
   calculating speed.  Please remove generic dgemm.f program, and link
   this library instead.

 Distribution:
   based on GPL2.

 Discription:

   The techniques of my routine are 

    1. Blocking algorithm.  It's basic theory.

    2. To avoid cache congruence problem, once it copies block matrix 
       to small matrix(2nd cache image), then calculate on these small
       matrix.  This algorithm is not so fast(especially small
       matrix), but it can caluculate with constant speed(even if the
       matrix size are large or small, it shows same calculating speed).

    3. Using "Hit under miss" system.  21164(A)'s 2nd cache can access 
       with no wait, but this latency is 8 clocks.  So, we must load
       the data in advance.

    4. The floating point calculating latency is 4 clock. 
       The matrix-matrix multiply routine requires "multiply and add".
       If you program as it is, it takes 8 clock. To avoid this, 
       I uses software pipelining technique.

    5. Taking consideration into MAF(Miss Address File).
       21164(A) has six MAF.  Each MAF has 32byte-buffer(4 double
       floating point data).  If we access the data on the 2nd cache,
       we can not request the data to 2nd cache over 6 lines at once.
       For example,
                                   note: This program is example.
	for(k=0; k<x; k++){
	  for(j=0;j<y; j++){
	    double sum = 0.0;
	    for(i=0; i<z; i++)
		 sum += a[j][i]*b[k][i];
	   }
	}

       At a glance,  it's a appropriate access, because the memory
       access is in order.  If you make fast inner product routine,
       maybe it's a good algorithm.  But if you make matrix-matrix
       multiply routine on 21164(A), it isn't the best algorithm.

       Let me explain. 

       First, unrooling for j,

        for(k=0; k<x; k++){
          for(j=0;j<y; j+=4){
            double sum1 = 0.0, sum2 = 0.0;
            double sum3 = 0.0, sum4 = 0.0;
            for(i=0; i<z; i++)
                 sum1 += a[j+0][i]*b[k][i];
                 sum2 += a[j+1][i]*b[k][i];
                 sum3 += a[j+2][i]*b[k][i];
                 sum4 += a[j+3][i]*b[k][i];
           }
        }

       Second, unrolling for k,

        for(k=0; k<x; k+=2){
          for(j=0;j<y; j+=4){
            double sum1 = 0.0, sum2 = 0.0;
            double sum3 = 0.0, sum4 = 0.0;
            double sum5 = 0.0, sum6 = 0.0;
            double sum7 = 0.0, sum8 = 0.0;
            for(i=0; i<z; i++)
                 sum1 += a[j+0][i]*b[k+0][i];
                 sum2 += a[j+1][i]*b[k+0][i];
                 sum3 += a[j+2][i]*b[k+0][i];
                 sum4 += a[j+3][i]*b[k+0][i];

                 sum5 += a[j+0][i]*b[k+1][i];
                 sum6 += a[j+1][i]*b[k+1][i];
                 sum7 += a[j+2][i]*b[k+1][i];
                 sum8 += a[j+3][i]*b[k+1][i];

           }
        }

       Please attention "addressing of a and b".  The matrix a
       requires 4 lines and the matrix b requires 2 lines(total six
       lines).  21164 has six MAF. Therefre, it is saturated easily.
       The calculating ability is upto 1 Flops/Hz(600MFlops) at most.

       According to DEC's DXML benchmark, I think DEC took this
       algorithm.

       So, we must take other algorithm.

	for(k=0; k<x; k++){
	  for(j=0;j<y; j++){
	    double sum = 0.0;
	    for(i=0; i<z; i++)
		 sum += a[i][j]*b[k][i];
	   }
	}

       I swapped the matrix a of column and row(This case is
       Non-Transposed - Non-Transposed routine for FORTRAN).

       First, unrolling for j,

	for(k=0; k<x; k++){
	  for(j=0;j<y; j+=4){
            double sum1 = 0.0, sum2 = 0.0;
            double sum3 = 0.0, sum4 = 0.0;
	    for(i=0; i<z; i++)
		 sum1 += a[i][j+0]*b[k][i];
		 sum2 += a[i][j+1]*b[k][i];
		 sum3 += a[i][j+2]*b[k][i];
		 sum4 += a[i][j+3]*b[k][i];
	   }
	}

       Second, unrooling for k,

	for(k=0; k<x; k+=2){
	  for(j=0;j<y; j+=4){
            double sum1 = 0.0, sum2 = 0.0;
            double sum3 = 0.0, sum4 = 0.0;
            double sum5 = 0.0, sum6 = 0.0;
            double sum7 = 0.0, sum8 = 0.0;
	    for(i=0; i<z; i++)
		 sum1 += a[i][j+0]*b[k+0][i];
		 sum2 += a[i][j+1]*b[k+0][i];
		 sum3 += a[i][j+2]*b[k+0][i];
		 sum4 += a[i][j+3]*b[k+0][i];

		 sum5 += a[i][j+0]*b[k+1][i];
		 sum6 += a[i][j+1]*b[k+1][i];
		 sum7 += a[i][j+2]*b[k+1][i];
		 sum8 += a[i][j+3]*b[k+1][i];
	   }
	}

     In this case, the routine requires only 3 lines(a requires 1
     line, and b requires 2 lines).  The data is transferred 32bytes
     each. So there are no saturations and able to unroll for i.


    6. Instruction align.
       21164(A) operates with a small block called "slot" which is
       consist of 4 instructions with 64 byte-align and it can issue
       on one clock.  But there are some limitations of instruction's
       combination.

       Among 4 instructions, 21164(A) can issue 2 integer instruction
       and 2 floating point instruction at once.  The integer
       combination is pretty flexible, but the floating combination is
       limited. 21164(A) can only issue with Add-Mutiply,  therefore,
       Add-Add or Multiply-Multiply instruction can not issue on one
       clock;it takes 2 clocks.  The only exception is data
       movement(e.g. cpys), which can issue 2 instruction on one
       clock.

    7. Avoiding to use "nop" or "fnop"
       21164(A)'s nop(Non OPeration) and fnop(floating Non OPeration)
       are very convenient instructions.  But these instructions are
       dummy instructions.  Actually, "nop" instruction is replaced
       with "shift" or "addq" instructions with $31(Zero Register),
       and "fnop" instruction is replaced with "cpys" instructions
       with $f31(Floating Zero register).

       Therefore, "nop" and "fnop" instructions fill the pipleline.
       Even if CPU is busy,  the CPU can not skip these instructions.

       However, there is a convenient non operating instruction which
       is called "unop".  This instruction is also dummy instructions
       (ldq_u $31, 0($31)) and do not send any instructions to
       pipeline.  If CPU is busy, the CPU skips this instruction or
       can issue 4 instructions on one clock.

       Normally, the GNU assembler(as) replaces ".align" with some 
       "nop", but it takes one or two clocks.  I programmed to use
       "unop".

    8. predicting data flow and prefetching
       The matrix A and B is entirely blocked in stack matrix "sa" and 
       "sb" which can be accessed with no wait.  But the matrix C is
       not blocked.  So, we must move a part of data of C to 2nd cache
       in advance.

       21164(A) has also convenient instruction which is called
       "prefetch" instruction.  Originally, the Alpha Architecture has
       prefetch instructions which are called "fetch" and "fetch_m"
       instructions. But it's not fast and rather slow, besides, it is
       very doubtful to imprement this instructions(of cource, alpha
       can issue these instructions but no affect speed).

       Instead, 21164(A) has two useful prefetch instructions.  These
       instructions  are "ldq $31, 0($??)" and "ldt $f31, 0($??)".
       There are no difference in terms of "moving data to cache", but
       "ldq" prefetch instruction moves data to 8kB first level cache
       by force.  "ldt" prefetch instruction stops moving data to 8kB
       first level cache or store second 96kB cache if memory bus is
       busy.

       Different from "fetch" instruction, this "ldq" and "ldt"
       instructions can move only 64bytes(the "fetch" instruction can
       move 512bytes at once).  So, we must use in loops with care.
       Because the most inner loop is so very busy that CPU can not 
       move data smoothly if unexpected prefetch instruction is
       issued.

       note) This "ldq" and "ldt" prefetch instructions are regarded
             as "nop" on the 21066 or 21064(EV4 architecture).  So,
             we need not to take consideration into architecture
             difference. 

    9. cache align
       Alpha's stack is very simple(this is not so much architecture
       as promise on Digital UNIX). So, we can get temporary working
       areas as much as we need. 

       ------------
       Usually, this size is limited by kernel and default value is
       maybe 2MB.  My routine uses about 400kB maximum(it depends on
       the situation, 136kB minimum). If you have not enough stack
       area, please change stack limit size by using ulimit(bash) or
       limit(csh) command.
       ------------

       The important thing is the kernel data is placed on these stack
       areas which stand for 2nd cache image.  So, this area should be
       aligned for first and second cache.  According to the Alpha
       Architecture Handbook,  it should be aligned for 8kB.  But it
       causes "unstable" result, which mean it shows different speeds
       each time(slow, fast, slow, fast...).  I tested some values
       to show "stable" result.

       Thus, I found this image should be aligned for 256kB.
       Surprisingly, if this image is aligned for 256kB,  it shows
       very constant speed each time, and further, it is 10 to 20
       MFlops faster than unaligned image(maybe it reduces DTB
       misses).
       
       Fllowing shows minute explanation.


 P, Q, R:
   Internal BLOCK sizes(2nd cache image).  The default values are 
   P=32, Q=112, R=112(dgemm).

   If you have other Alpha CPU, please modify this.  I strongly
   recommend the values are multiply for 8.

   If you find better parameters, please let me know.


 *** cache align map ***

 sizeof sa = Q*LDA = Q*P  = 28kB
 sizeof sb = R*LDB = R*Q  = 98kB
                 <- It's fairly close to second level cache size.

    Possible combinations are

             -----------------------
              Q   |  R  | Total size
             -----+-----+-----------
              96  | 128 |   12288(just in second level cache size)
             104  | 120 |   12480
             112  | 112 |   12544  <- I took this value.
             120  | 104 |   12480
             128  |  96 |   12280
             -----------------------

 stack address table:

 |<->|        first  level cache size
 | <--------- second level cache size  --------> |
 +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 0   8  16  24  32  40  48  56  64  72  80  88  96 104 112 120 128 136kB

 |<----- sa ------>|<-------------------  sb ---------------------->|
     Very Busy            Not so busy(only 2*R(1.75kB) is busy)

 | <- This must be aligned for 8kB and more better.

                      |<->| C matrix size is P*R = 4.375kB(no cached).


 But it seems there's no rooms for storing sa and sb in second cache
entirely(Of cource, we must take consideration into C matrix, but 
it's pretty small that we can ignore it).

 In fact, the size of sa matrix is not sensitive on Non-transposed
* Non-transposed routine.  But if we use Transposed routine, it's
significantly affects the calculating speed.  I've tested many
combinations of sa and sb matrix sizes, I determined this parameter.

 I think this comes from my algorithm.  My routine's main target is
internal second level cache and all the data which are on second level
cache are accessed with no-wait.  and the matrix sb is not so busy, 
maybe a part of sb is out of second level cache and moves 3rd level cache.

 When we need the data of sb matrix, using floating point prefetch($f31)
command, we can move to second level cache easily(but we must wait about
27 clocks from Bcache(3rd cache).  First level cache is very busy, so we
use floating point prefetch rather than integer prefetch($31) which
moves into first level cache by force). 

When we access C matrix, we must load carefully.  Because my routine's
coding waits only 4 clocks which is enough to load from first level cache,
but not enough to load from second level cache.  So, we must load
to first level cache in advance.  You know, integer prefetch command 
is convenient in this case.  To move C matrix data to first level cache,
I took 2-stage prefetch command.  First, using floating point integer
prefetch command, the data move into second level cache.  Then, using
integer prefetch command, the data move into first level cache within
8 clocks.


   10. To acomplish these techniques, this routine uses unrooling
       loops.

   11. Performance Counter

       21164 has a special counter called "Performance Counter"(PMCTR).
       This counter tells us various and useful information,
       such as cycles, cache misses, traps...

       Usually, we can not access this counter, because PAL mode can
       only control this counter.  I modified PAL code and Linux
       kernel to handle performance counter.

       Following two tables are results of PMCTR(PAL, OS, USER) with 
       1000 x 1000 operation.  One is "Total information", and another
       is "Most innter loop information".  If you want to know "What
       happens in my routine", please check this(PMCTR start and end
       points are shown as "PMCTR").

       For example,  double precision in most inner loop, the cycle is
       1159021923, and one of FP Operation issue is 2000000000, so it
       runs about 1.72 Flops/Cycle(1035MFlops! at 600MHz), and single
       precision, 1.80 Flops/Cycle(1077MFlops at 600MHz, theoretical
       value is 1.94 Flops/Cycle(1164MFlops)).


                 1. TOTAL PERFORMANCE(Double Precision)

     I t e m          < P A L >           < O S >            < U S E R>
Cycles              :24040635(  1.6)   88757986(  5.9)      1391413460( 92.5)
Instructions        :10054893(  0.3)    6574276(  0.2)      3064465624( 99.5)
nonissue cycles     :11805424(  4.4)   18244225(  6.9)       235788592( 88.7)
split-issue cycles  :   38174(  0.4)     404458(  4.5)         8470208( 95.0)
pipe-dry cycles     : 6977519(  6.5)    1387619(  1.3)        98169336( 92.1)
replay trap         :  317868(  2.5)    1557948( 12.4)        10712742( 85.1)
single-issue cycles : 2065946(  7.4)    1122060(  4.0)        24873089( 88.6)
dual-issue cycles   : 3986780(  1.2)     326087(  0.1)       326973648( 98.7)
triple-issue cycles :       0(  0.0)      16333(  0.0)       469064912(100.0)
quad-issue cycles   :       0(  0.0)      94461(  0.0)       245124815(100.0)
flow-change insns   : 1980269(  5.2)     324247(  0.8)        35842746( 94.0)
IntOps issued       :  345642(  0.3)     954533(  0.9)       110849333( 98.8)
FPOps issued        :       0(  0.0)         16(  0.0)      2037152000(100.0)
loads issued        :  663822(  0.1)     983167(  0.1)       861625000( 99.8)
stores issued       :   17378(  0.1)     281093(  1.5)        19000161( 98.5)
Icache issued       : 4947642(  0.5)    4875580(  0.5)       966920447( 99.0)
Dcache accesses     :  681028(  0.1)    3859971(  0.4)       880625360( 99.5)
S cache access      : 1755517(  0.5)    1200971(  0.3)       382611552( 99.2)
long(>15) stalls    :  130102(  4.8)     191422(  7.0)         2403319( 88.2)
PC-mispredicts      :   25070( 40.2)      37339( 59.8)               0(  0.0)
BR-mispredicts      :      49(  0.0)       6720(  0.5)         1418693( 99.5)
Icache/RFB misses   :  308679( 34.3)     183935( 20.4)          407212( 45.3)
ITB misses          :      40(100.0)          0(  0.0)               0(  0.0)
Dcache LD misses    :  348236(  0.0)    1751006(  0.2)       797313848( 99.7)
DTB misses          :   12838(  2.0)         74(  0.0)          622846( 98.0)
LDs merged in MAF   :    4733(  0.0)     322860(  0.1)       428665533( 99.9)
LDU replay traps    :  315715( 66.7)     157372( 33.3)               0(  0.0)
WB/MAF replay traps :     155(  0.0)     268411(  2.5)        10630795( 97.5)
MB stall cycles     :   26439(  2.6)    1000964( 97.4)               0(  0.0)
LDx_L instructions  :       0(  0.0)      30693(100.0)               0(  0.0)
Scache misses       :  457459(  3.2)     510195(  3.6)        13143147( 93.1)


                 2. TOTAL PERFORMANCE(Single Precision)

     I t e m          < P A L >           < O S >            < U S E R>
Cycles              : 7320201(  0.6)   19849313(  1.6)      1205834574( 97.8)
Instructions        : 2464631(  0.1)    6188563(  0.2)      2955596001( 99.7)
nonissue cycles     : 2477460(  1.9)    3983745(  3.1)       122191708( 95.0)
split-issue cycles  :   15954(  0.3)     301108(  5.2)         5526437( 94.6)
pipe-dry cycles     : 2760518(  4.9)    5371666(  9.5)        48503371( 85.6)
replay trap         :   94757(  1.4)     383509(  5.5)         6500476( 93.1)
single-issue cycles :  529520(  2.5)    1822707(  8.6)        18745438( 88.9)
dual-issue cycles   :  971043(  0.3)     463624(  0.1)       330787590( 99.6)
triple-issue cycles :       0(  0.0)     167187(  0.0)       469672746(100.0)
quad-issue cycles   :       0(  0.0)     118404(  0.1)       216264337( 99.9)
flow-change insns   :  480347(  1.4)     292649(  0.8)        33754592( 97.8)
IntOps issued       :  310987(  0.3)     842205(  0.8)       103092900( 98.9)
FPOps issued        :       0(  0.0)          4(  0.0)      2020500000(100.0)
loads issued        :  157744(  0.0)     871543(  0.1)       789750199( 99.9)
stores issued       :   14770(  0.2)     255384(  2.9)         8500000( 96.9)
Icache issued       : 1451087(  0.2)    1661207(  0.2)       932178953( 99.7)
Dcache accesses     :  172398(  0.0)    1458506(  0.2)       798250360( 99.8)
S cache access      :  611916(  0.1)     993141(  0.2)       498446770( 99.7)
long(>15) stalls    :   36248(  1.2)     323825( 10.5)         2720491( 88.3)
PC-mispredicts      :   13457( 29.6)      31918( 70.3)              44(  0.1)
BR-mispredicts      :      62(  0.0)       6275(  0.8)          753817( 99.2)
Icache/RFB misses   :  139075( 34.6)     127264( 31.7)          135703( 33.8)
ITB misses          :       6(100.0)          0(  0.0)               0(  0.0)
Dcache LD misses    :  107625(  0.0)    1455598(  0.2)       771737700( 99.8)
DTB misses          :    2719(  1.9)       4798(  3.3)          139353( 94.9)
LDs merged in MAF   :    4321(  0.0)     169742(  0.1)       278756738( 99.9)
LDU replay traps    :   92134( 39.9)     138727( 60.1)             112(  0.0)
WB/MAF replay traps :       7(  0.0)     123794(  2.2)         5631519( 97.8)
MB stall cycles     :   22611(  2.8)     774628( 97.2)              12(  0.0)
LDx_L instructions  :       0(  0.0)       6665(100.0)               0(  0.0)
Scache misses       :  158439(  2.7)     794794( 13.4)         4968657( 83.9)


                 3. MOST INNER LOOP PERFORMANCE(Double Precision)

     I t e m          < P A L >           < O S >            < U S E R>
Cycles              : 8942107(  0.8)   20984396(  1.8)      1159021923( 97.5)
Instructions        : 6708602(  0.2)    5995120(  0.2)      2958627444( 99.6)
nonissue cycles     : 1284498(  1.4)    9295545( 10.0)        82020078( 88.6)
split-issue cycles  : 1142955( 12.2)     319981(  3.4)         7906002( 84.4)
pipe-dry cycles     : 2605269(  4.6)    2432302(  4.3)        51485984( 91.1)
replay trap         :   43999(  0.7)     802643( 12.6)         5542400( 86.7)
single-issue cycles : 3629087( 21.1)     911279(  5.3)        12632438( 73.6)
dual-issue cycles   : 1539791(  0.5)     380440(  0.1)       301712463( 99.4)
triple-issue cycles :       0(  0.0)      75877(  0.0)       463204003(100.0)
quad-issue cycles   :       0(  0.0)      29433(  0.0)       238233812(100.0)
flow-change insns   : 1328068(  3.9)     262959(  0.8)        32375265( 95.3)
IntOps issued       : 2436793(  2.4)     706059(  0.7)        97142318( 96.9)
FPOps issued        :       0(  0.0)         10(  0.0)      2000000000(100.0)
loads issued        :   65524(  0.0)     134050(  0.0)       829872447(100.0)
stores issued       :   14042(  5.7)     230650( 94.3)               0(  0.0)
Icache issued       : 4115170(  0.4)    4257726(  0.5)       910026433( 99.1)
Dcache accesses     :   80150(  0.0)    1370198(  0.2)       829125000( 99.8)
S cache access      : 1366347(  0.4)     945848(  0.3)       366755336( 99.4)
long(>15) stalls    :   17178(  1.4)     109193(  8.8)         1112076( 89.8)
PC-mispredicts      :   16665( 35.2)      30611( 64.7)              24(  0.1)
BR-mispredicts      :    1115(  0.1)       5552(  0.5)         1125014( 99.4)
Icache/RFB misses   :  116714( 17.7)     160151( 24.3)          381791( 58.0)
ITB misses          :      13(100.0)          0(  0.0)               0(  0.0)
Dcache LD misses    :   58902(  0.0)    1702718(  0.2)       773813770( 99.8)
DTB misses          :    3916(  7.9)         47(  0.1)           45771( 92.0)
LDs merged in MAF   :    3887(  0.0)     422379(  0.1)       414117219( 99.9)
LDU replay traps    :   46068( 26.8)     125666( 73.2)              40(  0.0)
WB/MAF replay traps :       0(  0.0)     263574(  5.0)         4969462( 95.0)
MB stall cycles     :   20592(  2.5)     802797( 97.5)              12(  0.0)
LDx_L instructions  :       0(  0.0)       6097(100.0)               0(  0.0)
Scache misses       :  137703(  1.3)     437914(  4.2)         9738174( 94.4)


                 4. MOST INNER LOOP PERFORMANCE(Single Precision)

     I t e m          < P A L >           < O S >            < U S E R>
Cycles              : 4820641(  0.4)   19033425(  1.7)      1114185626( 97.9)
Instructions        : 3636268(  0.1)     284799(  0.0)      2906832025( 99.9)
nonissue cycles     :  258612(  0.4)   11443352( 16.5)        57689054( 83.1)
split-issue cycles  :  637384( 11.0)     413759(  7.1)         4769107( 81.9)
pipe-dry cycles     : 1581592(  3.2)    4742599(  9.6)        43019180( 87.2)
replay trap         :   15724(  0.3)      48219(  0.8)         5889423( 98.9)
single-issue cycles : 2009237( 12.3)     827514(  5.1)        13531948( 82.7)
dual-issue cycles   :  816861(  0.3)     412295(  0.1)       318320733( 99.6)
triple-issue cycles :       0(  0.0)      35287(  0.0)       467131998(100.0)
quad-issue cycles   :       0(  0.0)      25762(  0.0)       212446897(100.0)
flow-change insns   :  717174(  2.2)     258451(  0.8)        31875436( 97.0)
IntOps issued       : 1397810(  1.4)     712877(  0.7)        95625418( 97.8)
FPOps issued        :       0(  0.0)         34(  0.0)      2000000000(100.0)
loads issued        :   27868(  0.0)     829052(  0.1)       773750097( 99.9)
stores issued       :   13502(  6.2)     204496( 93.7)             161(  0.1)
Icache issued       : 2182080(  0.2)    4636035(  0.5)       909099506( 99.3)
Dcache accesses     :   42175(  0.0)    1316166(  0.2)       773750176( 99.8)
S cache access      :  779090(  0.2)    1004147(  0.2)       490343510( 99.6)
long(>15) stalls    :    7337(  0.3)     121189(  5.4)         2128035( 94.3)
PC-mispredicts      :   11013( 10.6)      92718( 89.2)             174(  0.2)
BR-mispredicts      :       5(  0.0)      10601(  1.7)          625030( 98.3)
Icache/RFB misses   :   56879( 20.2)     124347( 44.3)           99758( 35.5)
ITB misses          :      36(100.0)          0(  0.0)               0(  0.0)
Dcache LD misses    :   21976(  0.0)    1587594(  0.2)       759713787( 99.8)
DTB misses          :    1232(  8.3)         55(  0.4)           13540( 91.3)
LDs merged in MAF   :    3822(  0.0)      87517(  0.0)       272338560(100.0)
LDU replay traps    :   15835( 11.7)      47437( 34.9)           72593( 53.4)
WB/MAF replay traps :       0(  0.0)      46680(  0.9)         4872651( 99.1)
MB stall cycles     :   21393(  2.9)     715356( 97.1)              12(  0.0)
LDx_L instructions  :       0(  0.0)       5836(100.0)               0(  0.0)
Scache misses       :   73594(  1.8)     324212(  7.7)         3805258( 90.5)


*/

#ifdef DGEMM
#define P 32
#define Q 112
#define R 112
#else
#define P 40
#define Q 200
#define R 200
#endif

/* Internal BLOCK Leading size.*/
#define LDA (Q<<2)
#define LDB (Q<<1)

                   /* Now starting Main program */
#ifdef DGEMM

#ifdef NN
#define ROUTINE dgemm_nn
#endif
#ifdef NT
#define ROUTINE dgemm_nt
#endif
#ifdef TN
#define ROUTINE dgemm_tn
#endif
#ifdef TT
#define ROUTINE dgemm_tt
#endif

#else

#ifdef NN
#define ROUTINE sgemm_nn
#endif
#ifdef NT
#define ROUTINE sgemm_nt
#endif
#ifdef TN
#define ROUTINE sgemm_tn
#endif
#ifdef TT
#define ROUTINE sgemm_tt
#endif
#endif

#ifdef DGEMM
#define SIZE	8
#define LD	ldt
#define ST	stt
#define SXADDQ	s8addq
#else
#define SIZE	4
#define LD	lds
#define ST	sts
#define SXADDQ	s4addq
#define mult	muls
#define addt	adds
#endif

#define MATRIX_A   ((P>>2)*LDA*SIZE)
#define MATRIX_B   ((R>>1)*LDB*SIZE)

#define STACKSIZE  16*8

	.set noreorder
	.set noat

.text
	.align 5
	.globl ROUTINE
	.ent ROUTINE

/* Initial Routine */
ROUTINE:
	lda	$30, -STACKSIZE($30)		# prepare stack

/* save original register value */
	stq	$26, 0($30)
	stq	$9,  8($30)
	stq	$10,16($30)
	stq	$11,24($30)
	stq	$12,32($30)
	stq	$13,40($30)
	stq	$14,48($30)
	stq	$15,56($30)
	stt	$f2,64($30)
	stt	$f3,72($30)
	stt	$f4,80($30)
	stq	$29,88($30)
	stt	$f5,96($30)
	stt	$f6,104($30)
	stt	$f7,112($30)
	stt	$f8,120($30)

	.prologue	0

/* restore argument value */
#ifndef WINNT
	ldq	$9,  0+STACKSIZE($30)	# B
	ldl	$19, 8+STACKSIZE($30)	# ldb
	ldq	$13,16+STACKSIZE($30)	# C
	ldl	$10,24+STACKSIZE($30)	# ldc
#else
	ldl	$9,  0+STACKSIZE($30)	# B
	ldl	$19, 8+STACKSIZE($30)	# ldb
	ldl	$13,16+STACKSIZE($30)	# C
	ldl	$10,24+STACKSIZE($30)	# ldc
#endif

	clr	$3			# ls = 0
	fmov	$f19, $f3

	/* PMCTR START in case of Total Performance */

/*
   Prepare internal matrix.  We must align Stack Pointer.
   This is a little complex, though...
*/
	lda	$14, 0x3ffff			# 256kB align(2 instructions)
	mov	$30, $29			# backup Current Stack Pointer

	lda	$30, -MATRIX_B($30)
	lda	$30, -MATRIX_A($30)		# &sa[0][0]: Size is Q*LDA*8
	andnot	$30, $14, $30			# align for 256kB
	lda	$28,  MATRIX_A($30)		# &sb[0][0]: Size is R*LDB*8
	.align 5

$L5:
	clr	$14			# js = 0
	subl	$18,$3,$7		# min_l = k - ls
	cmplt	$7, Q+1, $1		# (min_l>Q)?
	cmoveq	$1, Q,   $7		# if $1 then min_l = Q
	.align 4

#if defined(NN) || (!defined(C_VERSION) && defined(TN)) \
	|| (defined(C_VERSION) && defined(NT))

$L10:
#ifndef C_VERSION
	mull	$19,$14,$2		# js*ldb
	subl	$17,$14,$12		# min_j = n - js
#else
	mull	$21,$14,$2		# js*ldb
	subl	$16,$14,$12		# min_j = n - js
#endif

	cmplt	$12,R+1, $1		# (min_j>R) ?
	cmoveq	$1, R,   $12		# if $1 then min_j = R

	mov	$28, $24		# a_offset = &sb[0][0]
	mull	$10,$14,$25		# jsldc = js * ldc

	mov	$12, $27		# j = min_j
#ifndef C_VERSION
	SXADDQ	$2,$9,$0		# b_offset = b + js*ldb
#else
	SXADDQ	$2,$20,$0		# b_offset = b + js*ldb
#endif
	.align 4

$L15:
	mov	$7,  $5			# l = min_l
	mov	$24, $2			# a1_offset = a_offset

	SXADDQ	$3,  $0, $4		# b1_offset = b_offset + ls
#ifndef C_VERSION
	SXADDQ	$19, $0, $0		# b_offset += ldb
#else
	SXADDQ	$21, $0, $0		# b_offset += ldb
#endif

	SXADDQ	$3,  $0, $22		# c1_offset = b_offset + ls
#ifndef C_VERSION
	SXADDQ	$19, $0, $0		# b_offset += ldb
#else
	SXADDQ	$21, $0, $0		# b_offset += ldb
#endif
 	lda	$24,LDB*SIZE($24)	# a_offset += LDB
	subl	$27, 2, $27		# j--
	.align 4

$L19:
	LD	$f10,  0*SIZE($4)
	LD	$f11,  1*SIZE($4)
	LD	$f12,  2*SIZE($4)
	LD	$f13,  3*SIZE($4)

	LD	$f23,  4*SIZE($4)
	LD	$f24,  5*SIZE($4)
	LD	$f25,  6*SIZE($4)
	LD	$f26,  7*SIZE($4)

	LD	$f14,  0*SIZE($22)
	LD	$f15,  1*SIZE($22)
	LD	$f21,  2*SIZE($22)
	LD	$f22,  3*SIZE($22)

	LD	$f27,  4*SIZE($22)
	LD	$f28,  5*SIZE($22)
	LD	$f29,  6*SIZE($22)
	LD	$f30,  7*SIZE($22)

	lda	$2,   16*SIZE($2)	# a1_offset += 8
	subl	$5,  8, $5		# l -= 4
	lda	$4,    8*SIZE($4)	# b1_offset += 4
	lda	$22,   8*SIZE($22)	# c1_offset += 4

	ST	$f10, -16*SIZE($2)
	ST	$f14, -15*SIZE($2)
	ST	$f11, -14*SIZE($2)
	ST	$f15, -13*SIZE($2)

	ST	$f12, -12*SIZE($2)
	ST	$f21, -11*SIZE($2)
	ST	$f13, -10*SIZE($2)
	ST	$f22,  -9*SIZE($2)

	ST	$f23,  -8*SIZE($2)
	ST	$f27,  -7*SIZE($2)
	ST	$f24,  -6*SIZE($2)
	ST	$f28,  -5*SIZE($2)

	ST	$f25,  -4*SIZE($2)
	ST	$f29,  -3*SIZE($2)
	ST	$f26,  -2*SIZE($2)
	ST	$f30,  -1*SIZE($2)

	bgt	$5,  $L19		# if l>0 goto $L19
	bgt	$27, $L15		# if j>0 goto $L15

#else			/* NT or TT */

$L10:
#ifndef C_VERSION
	mull	$19, $3, $2		# ls*ldb
	subl	$17,$14,$12		# min_j = n - js
#else
	mull	$21, $3, $2		# ls*ldb
	subl	$16,$14,$12		# min_j = n - js
#endif

	cmplt	$12,R+1, $1		# (min_j>R) ?
	cmoveq	$1, R,   $12		# if $1 then min_j = R

	mov	$28, $24		# a_offset = &sb[0][0]
	mull	$10,$14,$25		# jsldc = js * ldc

	mov	$12, $27		# j = min_j
#ifndef C_VERSION
	SXADDQ	$2,$9,$0		# b_offset = b + ls*ldb
#else
	SXADDQ	$2,$20,$0		# b_offset = b + ls*ldb
#endif
	.align 4

$L101:
	mov	$24, $2			# a1_offset = a_offset
	mov	$7,  $5			# l = min_l

	lda	$24, LDB*SIZE($24)	# a_offset += LDB
	SXADDQ	$14, $0, $4		# b1_offset = b_offset + js
	lda	$0,    2*SIZE($0)	# b_offset += 2
	.align 4

$L102:
#ifndef C_VERSION
	LD	$f10, 0*SIZE($4)	# atemp1 = *(b1_offset+0)
	LD	$f11, 1*SIZE($4)	# atemp2 = *(b1_offset+1)
	SXADDQ	$19, $4, $4		# b1_offset += ldb

	LD	$f12, 0*SIZE($4)	# atemp3 = *(b1_offset+0)
	LD	$f13, 1*SIZE($4)	# atemp4 = *(b1_offset+1)
	SXADDQ	$19, $4, $4		# b1_offset += ldb
#else
	LD	$f10, 0*SIZE($4)	# atemp1 = *(b1_offset+0)
	LD	$f11, 1*SIZE($4)	# atemp2 = *(b1_offset+1)
	SXADDQ	$21, $4, $4		# b1_offset += ldb

	LD	$f12, 0*SIZE($4)	# atemp3 = *(b1_offset+0)
	LD	$f13, 1*SIZE($4)	# atemp4 = *(b1_offset+1)
	SXADDQ	$21, $4, $4		# b1_offset += ldb
#endif

	subl	$5, 2, $5		# l--
	ST	$f10, 0*SIZE($2)	# *(a1_offset+0) = atemp1
	ST	$f11, 1*SIZE($2)	# *(a1_offset+1) = atemp2
	ST	$f12, 2*SIZE($2)	# *(a1_offset+3) = atemp3
	ST	$f13, 3*SIZE($2)	# *(a1_offset+4) = atemp4

	lda	$2,   4*SIZE($2)	# a1_offset += 4

	bgt	$5, $L102		# if l>0 goto $102

	subl	$27, 2, $27		# l--
	bgt	$27, $L101
#endif
	clr	$11			# is = 0
	.align 4

#if defined(NN) || (!defined(C_VERSION) && defined(NT)) \
	|| (defined(C_VERSION) && defined(TN))

$L30:
#ifndef C_VERSION
	mull	$21,$3,$2		# lslda = lda * ls
	subl	$16,$11,$15		# min_i = m - is
#else
	mull	$19,$3,$2		# lslda = lda * ls
	subl	$17,$11,$15		# min_i = m - is
#endif
	cmplt	$15,P+1,$1		# (min_i>P)?
	cmoveq	$1, P,  $15		# if $1 then min_i = P

	mov	$30, $0			# b_offset = &sa[0][0]
	mov	$7,  $5			# l = min_l

#ifndef C_VERSION
	SXADDQ	$2,$20,$24		# a_offset = a + lslda
#else
	SXADDQ	$2,$9, $24		# a_offset = a + lslda
#endif
	.align 4

$L35:
	SXADDQ	$11, $24, $2		# a1_offset = a_offset + is
#ifndef C_VERSION
	SXADDQ	$21, $24, $24		# a_offset += lda
#else
	SXADDQ	$19, $24, $24		# a_offset += lda
#endif

	SXADDQ	$11, $24, $22		# a1_offset = a_offset + is
#ifndef C_VERSION
	SXADDQ	$21, $24, $24		# a_offset += lda
#else
	SXADDQ	$19, $24, $24		# a_offset += lda
#endif

	mov	$0,  $4			# b1_offset = b_offset
	subl	$15,  8, $8		# i -= 8

	lda	$0,    8*SIZE($0)	# b_offset += 4
	subl	$5, 2, $5		# l--
	.align 4

#ifdef DGEMM
	LD	$f10,  0*SIZE($2)
	LD	$f11,  1*SIZE($2)
	LD	$f12,  2*SIZE($2)
	LD	$f13,  3*SIZE($2)

	LD	$f23,  0*SIZE($22)
	LD	$f24,  1*SIZE($22)
	LD	$f25,  2*SIZE($22)
	LD	$f26,  3*SIZE($22)

	LD	$f14,  4*SIZE($2)
	LD	$f15,  5*SIZE($2)
	LD	$f21,  6*SIZE($2)
	LD	$f22,  7*SIZE($2)

	LD	$f27,  4*SIZE($22)
	LD	$f28,  5*SIZE($22)
	LD	$f29,  6*SIZE($22)
	LD	$f30,  7*SIZE($22)
#else
	ldt	$f10,  0*SIZE($2)
	ldt	$f12,  2*SIZE($2)
	ldt	$f23,  0*SIZE($22)
	ldt	$f25,  2*SIZE($22)

	ldt	$f14,  4*SIZE($2)
	ldt	$f21,  6*SIZE($2)
	ldt	$f27,  4*SIZE($22)
	ldt	$f29,  6*SIZE($22)
#endif
	ble	$8,$L38			# if i>0 goto $L39
	.align 4

$L39:
#ifdef DGEMM
	ST	$f10, (0*LDA+0)*SIZE($4)
	ST	$f11, (0*LDA+1)*SIZE($4)
	ST	$f12, (0*LDA+2)*SIZE($4)
	ST	$f13, (0*LDA+3)*SIZE($4)

	LD	$f10,  8*SIZE($2)
	LD	$f11,  9*SIZE($2)
	LD	$f12, 10*SIZE($2)
	LD	$f13, 11*SIZE($2)

	ST	$f23, (0*LDA+4)*SIZE($4)
	ST	$f24, (0*LDA+5)*SIZE($4)
	ST	$f25, (0*LDA+6)*SIZE($4)
	ST	$f26, (0*LDA+7)*SIZE($4)

	LD	$f23,  8*SIZE($22)
	LD	$f24,  9*SIZE($22)
	LD	$f25, 10*SIZE($22)
	LD	$f26, 11*SIZE($22)

	ST	$f14, (LDA+0)*SIZE($4)
	ST	$f15, (LDA+1)*SIZE($4)
	ST	$f21, (LDA+2)*SIZE($4)
	ST	$f22, (LDA+3)*SIZE($4)

	LD	$f14, 12*SIZE($2)
	LD	$f15, 13*SIZE($2)
	LD	$f21, 14*SIZE($2)
	LD	$f22, 15*SIZE($2)

	ST	$f27, (LDA+4)*SIZE($4)
	ST	$f28, (LDA+5)*SIZE($4)
	ST	$f29, (LDA+6)*SIZE($4)
	ST	$f30, (LDA+7)*SIZE($4)

	LD	$f27, 12*SIZE($22)
	LD	$f28, 13*SIZE($22)
	LD	$f29, 14*SIZE($22)
	LD	$f30, 15*SIZE($22)
#else
	stt	$f10, (0*LDA+0)*SIZE($4)
	stt	$f12, (0*LDA+2)*SIZE($4)
	stt	$f23, (0*LDA+4)*SIZE($4)
	stt	$f25, (0*LDA+6)*SIZE($4)

	ldt	$f10,  8*SIZE($2)
	ldt	$f12, 10*SIZE($2)
	ldt	$f23,  8*SIZE($22)
	ldt	$f25, 10*SIZE($22)

	stt	$f14, (LDA+0)*SIZE($4)
	stt	$f21, (LDA+2)*SIZE($4)
	stt	$f27, (LDA+4)*SIZE($4)
	stt	$f29, (LDA+6)*SIZE($4)

	ldt	$f14, 12*SIZE($2)
	ldt	$f21, 14*SIZE($2)
	ldt	$f27, 12*SIZE($22)
	ldt	$f29, 14*SIZE($22)
#endif
	subl	$8,  8, $8		# i -= 8
	lda	$4,  2*LDA*SIZE($4)	# b1_offset += LDA
	lda	$2,     8*SIZE($2)	# a1_offset += 4
	lda	$22,    8*SIZE($22)	# a1_offset += 4

	bgt	$8,$L39			# if i>0 goto $L39
	.align 4
$L38:
#ifdef DGEMM
	ST	$f10, (0*LDA+0)*SIZE($4)
	ST	$f11, (0*LDA+1)*SIZE($4)
	ST	$f12, (0*LDA+2)*SIZE($4)
	ST	$f13, (0*LDA+3)*SIZE($4)

	ST	$f23, (0*LDA+4)*SIZE($4)
	ST	$f24, (0*LDA+5)*SIZE($4)
	ST	$f25, (0*LDA+6)*SIZE($4)
	ST	$f26, (0*LDA+7)*SIZE($4)

	ST	$f14, (LDA+0)*SIZE($4)
	ST	$f15, (LDA+1)*SIZE($4)
	ST	$f21, (LDA+2)*SIZE($4)
	ST	$f22, (LDA+3)*SIZE($4)

	ST	$f27, (LDA+4)*SIZE($4)
	ST	$f28, (LDA+5)*SIZE($4)
	ST	$f29, (LDA+6)*SIZE($4)
	ST	$f30, (LDA+7)*SIZE($4)
#else
	stt	$f10, (0*LDA+0)*SIZE($4)
	stt	$f12, (0*LDA+2)*SIZE($4)
	stt	$f23, (0*LDA+4)*SIZE($4)
	stt	$f25, (0*LDA+6)*SIZE($4)

	stt	$f14, (LDA+0)*SIZE($4)
	stt	$f21, (LDA+2)*SIZE($4)
	stt	$f27, (LDA+4)*SIZE($4)
	stt	$f29, (LDA+6)*SIZE($4)
#endif
	bgt	$5, $L35		# if l>0 goto $L35
	.align 4

#else       /* TN or TT */

$L30:

#ifndef C_VERSION
	mull	$11, $21, $2		# is*lda
	subl	$16, $11, $15		# min_i = m - is
#else
	mull	$11, $19, $2		# is*lda
	subl	$17, $11, $15		# min_i = m - is
#endif

	cmplt	$15,P+1,$1		# (min_i>P)?
	cmoveq	$1, P,  $15		# if $1 then min_i = P

#ifndef C_VERSION
	SXADDQ	$2, $20, $6		# c_offset = a + is*lda
	s4addq	$21, 0,  $1		# j = 4*lda
#else
	SXADDQ	$2, $9,  $6		# c_offset = a + is*lda
	s4addq	$19, 0,  $1		# j = 4*lda
#endif

	mov	$30, $0			# b_offset = &sa[0][0]
	mov	$15, $8			# i = min_i
	.align 4

$L201:
	mov	$0, $4			# b1_offset = b_offset
	lda	$0, LDA*SIZE($0)	# b_offset += LDA

	SXADDQ	$3, $6, $24		# a_offset = c_offset + ls
	mov	$7,  $5			# l = min_l
	.align 4

$L202:
	mov	$24, $2			# a1_offset = a_offset
	lda	$24,   4*SIZE($24)	# a_offset += 4

	LD	$f10,  0*SIZE($2)
	LD	$f11,  1*SIZE($2)
	LD	$f12,  2*SIZE($2)
	LD	$f13,  3*SIZE($2)

#ifndef C_VERSION
	SXADDQ	$21, $2, $2		# a1_offset += lda
#else
	SXADDQ	$19, $2, $2		# a1_offset += lda
#endif

	LD	$f14,  0*SIZE($2)
	LD	$f15,  1*SIZE($2)
	LD	$f21,  2*SIZE($2)
	LD	$f22,  3*SIZE($2)

#ifndef C_VERSION
	SXADDQ	$21, $2, $2		# a1_offset += lda
#else
	SXADDQ	$19, $2, $2		# a1_offset += lda
#endif

	LD	$f23,  0*SIZE($2)
	LD	$f24,  1*SIZE($2)
	LD	$f25,  2*SIZE($2)
	LD	$f26,  3*SIZE($2)

#ifndef C_VERSION
	SXADDQ	$21, $2, $2		# a1_offset += lda
#else
	SXADDQ	$19, $2, $2		# a1_offset += lda
#endif

	LD	$f27,  0*SIZE($2)
	LD	$f28,  1*SIZE($2)
	LD	$f29,  2*SIZE($2)
	LD	$f30,  3*SIZE($2)

	ST	$f10,  0*SIZE($4)	# atemp1
	ST	$f14,  1*SIZE($4)	# atemp5
	ST	$f23,  2*SIZE($4)	# ctemp1
	ST	$f27,  3*SIZE($4)	# ctemp5

	ST	$f11,  4*SIZE($4)	# atemp2
	ST	$f15,  5*SIZE($4)	# atemp6
	ST	$f24,  6*SIZE($4)	# ctemp2
	ST	$f28,  7*SIZE($4)	# ctemp6

	ST	$f12,  8*SIZE($4)	# atemp3
	ST	$f21,  9*SIZE($4)	# atemp7
	ST	$f25, 10*SIZE($4)	# ctemp3
	ST	$f29, 11*SIZE($4)	# ctemp7

	ST	$f13, 12*SIZE($4)	# atemp4
	ST	$f22, 13*SIZE($4)	# atemp8
	ST	$f26, 14*SIZE($4)	# ctemp4
	ST	$f30, 15*SIZE($4)	# ctemp8

	lda	$4,   16*SIZE($4)	# b1_offset += 16

	subl	$5, 4, $5		# l -= 4
	bgt	$5, $L202

	subl	$8, 4, $8		# i -= 4
	SXADDQ	$1, $6, $6		# c_offset += j
	bgt	$8, $L201

	.align 4
#endif

$L33:
	sra	$12, 1, $27		# j = (min_j>>1)
	mov	$28, $26		# b_orig = &sb[0][0]

	SXADDQ	$25,$13,$6		# c_offset = c + jsldc;
	ble	$27, $L48
	.align 4

$L50:
	mov	$26, $0			# b_offset = b_orig
	fclr	$f11
	lda	$26, SIZE*LDB($26)	# b_orig += LDB
	fclr	$f13

	mov	$30, $24		# a_offset = &sa[0][0]
	fclr	$f14
	sra	$15, 2, $8		# i = (min_i>>1)
 	fclr	$f15

	SXADDQ	$11, $6, $22		# c1_offset = c_offset + is
	fclr	$f12
	SXADDQ	$10, $6, $6		# c_offset += ldc(2 times)
	fclr	$f25

	SXADDQ	$10, $6, $6		# c_offset += ldc
	fclr	$f26
	fclr	$f27
	ble	$8,$L52			# if j<=0 goto $L52
	.align 4
$L54:
	mov	$24, $2			# a1_offset = a_offset
	fclr	$f28
	sra	$7, 2, $5		# l = (min_l>>2)
	fclr	$f29

	mov	$0, $4			# b1_offset = b_offset
	fclr	$f30
	SXADDQ	$10, $22, $23		# c2_offset = c1_offset + ldc
	fclr	$f21

	lda	$24, LDA*SIZE($24)	# a_offset += LDA
	LD	$f5, 0*SIZE($22)
	LD	$f6, 0*SIZE($23)
	ble	$5, $L55		# if l<=0 goto $L55

	LD	$f1,   0*SIZE($4)	# btemp1 = *(b1_offset+0)
	LD	$f10,  1*SIZE($4)	# btemp2 = *(b1_offset+1)
	LD	$f24,  2*SIZE($4)	# btemp3 = *(b1_offset+2)
	LD	$f23,  3*SIZE($4)	# btemp4 = *(b1_offset+3)

	LD	$f22,  0*SIZE($2)	# atemp1
	LD	$f20,  1*SIZE($2)	# atemp2
	LD	$f18,  2*SIZE($2)	# atemp3
	LD	$f16,  3*SIZE($2)	# atemp4

	LD	$f17,  4*SIZE($2)	# atemp5
	LD	$f0,   5*SIZE($2)	# atemp6
	LD	$f19,  6*SIZE($2)	# atemp7
	LD	$f2,   7*SIZE($2)	# atemp8

	/* PMCTR START in case of Most Inner Loop Performance */

	lda	$2, 8*SIZE($2)		# a1_offset += 8
	subl	$5,  1, $5		# l--
	addq	$4, 4*SIZE, $4		# b1_offset += 2
	ble	$5, $L57		# if l<=0 goto $L57
	.align 4

/* 
   Main Loop.
   This loop is very important and affects calculating speed directry.
   Each load waits 12 clocks which is enough to load from 2nd cache.
   And one loop takes 33 clocks(multiply and add = 32 clock plus jump
   latency = 1 clock).  If you have 21164A with 600MHz machine,  it'll
   take 1163.6 MFlops in this loop.  This is theoretical value, but it's
   pretty fast, isn't it?
*/
$L58:
	addt	$f28,$f11,$f28
#ifdef DGEMM
	LD	$f31,      16*SIZE($4)	# prefetch
#else
	LD	$f31,      12*SIZE($4)	# prefetch
#endif
	mult	$f1,$f22,$f11
#ifdef DGEMM
	LD	$f31,      32*SIZE($2)	# prefetch
#else
	unop
#endif

	addt	$f29,$f13,$f29
	mult	$f1,$f20,$f13
	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14

	addt	$f21,$f15,$f21
	subl	$5,1,$5
	mult	$f1,$f16,$f15
	LD	$f1,   0*SIZE($4)

	addt	$f12,$f11,$f12
	unop
	mult	$f10,$f22,$f11
	LD	$f22,  0*SIZE($2)

	addt	$f25,$f13,$f25
	unop
	mult	$f10,$f20,$f13
	LD	$f20,  1*SIZE($2)

	addt	$f26,$f14,$f26
	LD	$f7,   1*SIZE($4)
	mult	$f10,$f18,$f14
	LD	$f18,  2*SIZE($2)

	addt	$f27,$f15,$f27
	unop
	mult	$f10,$f16,$f15
	LD	$f16,  3*SIZE($2)

	addt	$f28,$f11,$f28
	unop
	mult	$f24,$f17,$f11
	unop

	addt	$f29,$f13,$f29
	mult	$f24,$f0,$f13
	addt	$f30,$f14,$f30
	mult	$f24,$f19,$f14

	addt	$f21,$f15,$f21
	unop
	mult	$f24,$f2,$f15
	LD	$f24,  2*SIZE($4)

	addt	$f12,$f11,$f12
	unop
	mult	$f23,$f17,$f11
	LD	$f17,  4*SIZE($2)

	addt	$f25,$f13,$f25
	LD	$f4,   3*SIZE($4)
	mult	$f23,$f0,$f13
	LD	$f0,   5*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f23,$f19,$f14
	LD	$f19,  6*SIZE($2)

	addt	$f27,$f15,$f27
	unop
	mult	$f23,$f2,$f15
	LD	$f2,   7*SIZE($2)

	addt	$f28,$f11,$f28
#ifdef DGEMM
	LD	$f31,   40*SIZE($2)
#else
	unop
#endif
	mult	$f1,$f22,$f11
	unop

	addt	$f29,$f13,$f29
	mult	$f1,$f20,$f13
	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14

	addt	$f21,$f15,$f21
	unop
	mult	$f1,$f16,$f15
	LD	$f1,   4*SIZE($4)

	addt	$f12, $f11, $f12
	unop
	mult	$f7,  $f22, $f11
	LD	$f22,  8*SIZE($2)

	addt	$f25,$f13,$f25
	unop
	mult	$f7,  $f20, $f13
	LD	$f20,  9*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f7,  $f18, $f14
	LD	$f18, 10*SIZE($2)

	addt	$f27,$f15,$f27
	LD	$f10,  5*SIZE($4)
	mult	$f7,  $f16, $f15
	LD	$f16, 11*SIZE($2)

	addt	$f28,$f11,$f28
	lda	$4,    8*SIZE($4)
	mult	$f24,$f17,$f11
	lda	$2,   16*SIZE($2)		# a1_offset += 16

	addt	$f29,$f13,$f29
	mult	$f24,$f0,$f13
	addt	$f30,$f14,$f30
	mult	$f24,$f19,$f14

	addt	$f21,$f15,$f21
	unop
	mult	$f24,$f2,$f15
	LD	$f24, -2*SIZE($4)

	addt	$f12,$f11,$f12
	LD	$f23, -1*SIZE($4)
	mult	$f4,$f17,$f11
	LD	$f17, -4*SIZE($2)

	addt	$f25,$f13,$f25
	unop
	mult	$f4,$f0,$f13
	LD	$f0,  -3*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f4,$f19,$f14
	LD	$f19, -2*SIZE($2)

	addt	$f27,$f15,$f27
	mult	$f4,$f2,$f15
	LD	$f2,  -1*SIZE($2)
	bgt	$5,$L58
	.align 4

$L57:
	addt	$f28,$f11,$f28
	LD	$f31,  8*SIZE($22)	# prefetch
	mult	$f1,$f22,$f11
#ifdef DGEMM
	LD	$f31, 12*SIZE($23)	# prefetch
#else
	unop
#endif

	addt	$f29,$f13,$f29
	mult	$f1,$f20,$f13
	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14

	addt	$f21,$f15,$f21
	unop
	mult	$f1,$f16,$f15
	LD	$f1,  0*SIZE($4)

	addt	$f12,$f11,$f12
	unop
	mult	$f10,$f22,$f11
	LD	$f22, 0*SIZE($2)

	addt	$f25,$f13,$f25
	LD	$f7,  1*SIZE($4)
	mult	$f10,$f20,$f13
	LD	$f20, 1*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f10,$f18,$f14
	LD	$f18, 2*SIZE($2)

	addt	$f27,$f15,$f27
	unop
	mult	$f10,$f16,$f15
	LD	$f16, 3*SIZE($2)

	addt	$f28,$f11,$f28
	unop
	mult	$f24,$f17,$f11
	lda	$2,   4*SIZE($2)	# a1_offset += 4

	addt	$f29,$f13,$f29
	mult	$f24,$f0,$f13
	addt	$f30,$f14,$f30
	mult	$f24,$f19,$f14

	addt	$f21,$f15,$f21
	unop
	mult	$f24,$f2,$f15
	LD	$f24, 2*SIZE($4)

	addt	$f12,$f11,$f12
	unop
	mult	$f23,$f17,$f11
	LD	$f17, 0*SIZE($2)

	addt	$f25,$f13,$f25
	LD	$f4,  3*SIZE($4)
	mult	$f23,$f0,$f13
	LD	$f0,  1*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f23,$f19,$f14
	LD	$f19, 2*SIZE($2)

	addt	$f27,$f15,$f27
	unop
	mult	$f23,$f2,$f15
	LD	$f2,  3*SIZE($2)

	addt	$f28,$f11,$f28
	unop
	mult	$f1,$f22,$f11
	lda	$2,  4*SIZE($2)		# a1_offset += 4

	addt	$f29,$f13,$f29
	addq	$4, 4*SIZE, $4
	mult	$f1,$f20,$f13
	unop

	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14
	addt	$f21,$f15,$f21
	mult	$f1,$f16,$f15

	addt	$f12, $f11, $f12
	mult	$f7,  $f22, $f11
	addt	$f25, $f13, $f25
	mult	$f7,  $f20, $f13

	addt	$f26,$f14,$f26
	mult	$f7, $f18,$f14
	addt	$f27,$f15,$f27
	mult	$f7, $f16,$f15

	addt	$f28,$f11,$f28
	mult	$f24,$f17,$f11
	addt	$f29,$f13,$f29
	mult	$f24,$f0,$f13

	addt	$f30,$f14,$f30
	mult	$f24,$f19,$f14
	addt	$f21,$f15,$f21
	mult	$f24,$f2,$f15

	addt	$f12, $f11, $f12
	mult	$f4,  $f17, $f11
	addt	$f25, $f13, $f25
	mult	$f4,  $f0,  $f13

	addt	$f26, $f14, $f26
	mult	$f4,  $f19, $f14
	addt	$f27, $f15, $f27
	mult	$f4,  $f2,  $f15

	/* PMCTR STOP in case of Most Inner Loop Performance */
	.align 4

$L55:
	and	$7,3,$5			# l = (min_l&3)
	beq	$5,$L60			# if l<=0 goto $L60

	LD	$f22, 0*SIZE($2)	# atemp1
	LD	$f1,  0*SIZE($4)	# btemp1

	LD	$f20, 1*SIZE($2)	# atemp2
	LD	$f10, 1*SIZE($4)	# btemp2
	LD	$f18, 2*SIZE($2)	# atemp3
	LD	$f16, 3*SIZE($2)	# atemp4

	lda	$2,   4*SIZE($2)	# a1_offset += 4
	subl	$5, 1, $5		# l--
	lda	$4,   2*SIZE($4)	# b1_offset += 2
	ble	$5,$L62			# if l<=0 goto $L62
	.align 4

$L63:
	addt	$f28,$f11,$f28
#ifdef DGEMM
	LD	$f31, 8*8($4)
#else
	unop
#endif
	mult	$f1,$f22,$f11
	unop

	addt	$f29,$f13,$f29
	mult	$f1,$f20,$f13
	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14

	addt	$f21,$f15,$f21
	lda	$2,    4*SIZE($2)	# a1_offset += 4
	mult	$f1,$f16,$f15
	LD	$f1,   0*SIZE($4)

	addt	$f12,$f11,$f12
	lda	$4,    2*SIZE($4)	# b1_offset += 2
	mult	$f10,$f22,$f11
	LD	$f22, -4*SIZE($2)

	addt	$f25,$f13,$f25
	subl	$5,1,$5			# l--
	mult	$f10,$f20,$f13
	LD	$f20,  -3*SIZE($2)

	addt	$f26,$f14,$f26
	unop
	mult	$f10,$f18,$f14
	LD	$f18,  -2*SIZE($2)

	addt	$f27,$f15,$f27
	mult	$f10,$f16,$f15
	LD	$f10,  -1*SIZE($4)
	LD	$f16,  -1*SIZE($2)

	bgt	$5,$L63			# if l>0 goto $L63
	unop
	unop
	unop
	.align 4

$L62:
	addt	$f28,$f11,$f28
	mult	$f1,$f22,$f11
	addt	$f29,$f13,$f29
	mult	$f1,$f20,$f13

	addt	$f30,$f14,$f30
	mult	$f1,$f18,$f14
	addt	$f21,$f15,$f21
	mult	$f1,$f16,$f15

	addt	$f12,$f11,$f12
	mult	$f10,$f22,$f11
	addt	$f25,$f13,$f25
	mult	$f10,$f20,$f13

	addt	$f26,$f14,$f26
	mult	$f10,$f18,$f14
	addt	$f27,$f15,$f27
	mult	$f10,$f16,$f15
	.align 4

$L60:
	addt	$f28,$f11,$f28
	LD	$f20, 1*SIZE($22)	# atemp2 = *(c1_offset+1)
	mult	$f3,$f12,$f11
	LD	$f18, 2*SIZE($22)	# atemp3 = *(c1_offset+2)

	addt	$f29,$f13,$f29
	LD	$f16, 3*SIZE($22)	# atemp4 = *(c1_offset+3)
	mult	$f3,$f25,$f13
	LD	$f0,  1*SIZE($23)	# atemp6 = *(c2_offset+1)

	addt	$f30,$f14,$f30
	LD	$f19, 2*SIZE($23)	# atemp7 = *(c2_offset+2)
	mult	$f3,$f26,$f14
	LD	$f2,  3*SIZE($23)	# atemp8 = *(c2_offset+3)

	addt	$f21,$f15,$f21
	unop
	mult	$f3,$f27,$f15
	unop

	addt	$f5,$f11,$f12
	addq	$22, 4*SIZE, $22	# c1_offset += 4
	mult	$f3,$f28,$f11
	subl	$8,1,$8			# i--

	addt	$f20, $f13, $f25
	mult	$f3,  $f29, $f13
	addt	$f18, $f14, $f26
	mult	$f3,  $f30, $f14

	addt	$f16,$f15,$f27
	mult	$f3,$f21,$f15
	addt	$f6,$f11,$f28
	ST	$f12, -4*SIZE($22)	# *(c1_offset+0) = ctemp1

	addt	$f0,$f13,$f29
	unop
	ST	$f25, -3*SIZE($22)	# *(c1_offset+1) = ctemp2
	fclr	$f11
	
	addt	$f19,$f14,$f30
	unop
	ST	$f26, -2*SIZE($22)	# *(c1_offset+2) = ctemp3
	fclr	$f13

	addt	$f2,$f15,$f21
	unop
	ST	$f27,  -1*SIZE($22)	# *(c1_offset+3) = ctemp4
	fclr	$f14

	ST	$f28, 0*SIZE($23)	# *(c2_offset+0) = ctemp5
	fclr	$f15
	unop
	fclr	$f12

	ST	$f29, 1*SIZE($23)	# *(c2_offset+1) = ctemp6
	fclr	$f25
	unop
	fclr	$f26

	ST	$f30, 2*SIZE($23)	# *(c2_offset+2) = ctemp7
	fclr	$f27
	ST	$f21, 3*SIZE($23)	# *(c2_offset+3) = ctemp8
	bgt	$8,$L54			# if i>0 goto $L54
	.align 4

$L52:
	and	$15,3,$8		# i = (min_i&3)
	beq	$8,$L49			# if i<0 goto $L49
	unop
	unop
	.align 4

$L69:
	mov	$24, $2			# a1_offset = a_offset
	addq	$24, SIZE, $24		# a_offset ++
	mov	$0, $4			# b1_offset = b_offset
	SXADDQ	$10, $22, $23		# c2_offset = c1_offset + ldc

	fclr	$f12			# ctemp1 = ZERO
	mov	$7, $5			# l = min_l
	fclr	$f28			# ctemp5 = ZERO
	ble	$7,$L71			# if l<=0 goto $L71
	.align 4

$L73:
	LD	$f22,    0($2)		# atemp1
	lda	$2,    4*SIZE($2)	# a1_offset += 4

	LD	$f1,   0*SIZE($4)	# btemp1 = *(b1_offset+0)
	LD	$f10,  1*SIZE($4)	# btemp2 = *(b1_offset+2)

	mult	$f1, $f22,$f11		# temp1 = btemp1 * atemp1
	mult	$f10,$f22,$f13		# temp2 = btemp2 * atemp1
	lda	$4,    2*SIZE($4)	# b1_offset ++
	subl	$5,1,$5			# l--

	addt	$f12,$f11,$f12		# ctemp1 = ctemp1 + temp1
	addt	$f28,$f13,$f28		# ctemp5 = ctemp5 + temp2
	bgt	$5,$L73			# if l>0 goto $L73
	unop
	.align 4

$L71:
	mult	$f3,$f12,$f11		# temp1 = alpha*ctemp1
	mult	$f3,$f28,$f13		# temp2 = alpha*ctemp2
	LD	$f22, 0*SIZE($22)	# atemp1 = *c1_offset
	LD	$f17, 0*SIZE($23)	# atemp5 = *c2_offset

	addt	$f22,$f11,$f12		# ctemp1 = atemp1 + temp1
	addt	$f17,$f13,$f28		# ctemp2 = atemp5 + temp2
	subl	$8,1,$8			# i--
	ST	$f12, 0*SIZE($22)	# *c1_offset = ctemp1

	ST	$f28, 0*SIZE($23)	# *c2_offset = ctemp2
	addq	$22, SIZE, $22		# c1_offset ++
	bgt	$8,$L69			# if i>0 goto $L69
	unop
	.align 4

$L49:
	subl	$27,1,$27		# j --
	bgt	$27,$L50		# if j>0 goto $L50
	unop
	unop
	.align 4

$L48:
	blbc	$12,$L29		# if (!min_j&1) goto $L29
	fclr	$f11
	sra	$15,2,$8		# i = (min_i>>2)
	fclr	$f13

	mov	$26, $0			# b_offset = b_orig
	fclr	$f14
	mov	$30, $24		# a_offset = &sa[0][0]
	fclr	$f15

	SXADDQ	$11, $6, $22		# c1_offset = c_offset + is
	ble	$8,$L79			# if i<0 goto $L79
	unop
	unop
	.align 4

$L81:
	mov	$24, $2			# a1_offset = a_offset
	fclr	$f12
	sra	$7, 2, $5		# l = (min_l>>2)
	fclr	$f25

	mov	$0, $4			# b1_offset = b_offset
	fclr	$f26
	lda	$24, LDA*SIZE($24)	# a_offset += LDA
	fclr	$f27

	unop
	ble	$5,$L82

	LD	$f1,   0*SIZE($4)	# btemp1
	LD	$f10,  2*SIZE($4)	# btemp2

	LD	$f22,  0*SIZE($2)	# atemp1
	LD	$f20,  1*SIZE($2)	# atemp2
	LD	$f18,  2*SIZE($2)	# atemp3
	LD	$f16,  3*SIZE($2)	# atemp4
	LD	$f17,  4*SIZE($2)	# atemp5
	LD	$f0,   5*SIZE($2)	# atemp6
	LD	$f19,  6*SIZE($2)	# atemp7
	LD	$f2,   7*SIZE($2)	# atemp8

	lda	$2,    8*SIZE($2)	# a1_offset += 8

	subl	$5, 1, $5		# l--
	lda	$4,    4*SIZE($4)	# b1_offset += 4
	ble	$5, $L84		# if l<= 0 goto $L84
	.align 4
$L85:
	addt	$f12,$f11,$f12
	unop
	mult	$f1,$f22,$f11
	LD	$f22,  0*SIZE($2)	# atemp1

	addt	$f25,$f13,$f25
	unop
	mult	$f1,$f20,$f13
	LD	$f20,  1*SIZE($2)	# atemp2

	addt	$f26,$f14,$f26
	unop
	mult	$f1,$f18,$f14
	LD	$f18,  2*SIZE($2)	# atemp3

	addt	$f27,$f15,$f27
	unop
	mult	$f1,$f16,$f15
	LD	$f1,   0*SIZE($4)	# btemp1

	addt	$f12,$f11,$f12
	LD	$f16,  3*SIZE($2)	# atemp4
	mult	$f10,$f17,$f11
	LD	$f17,  4*SIZE($2)	# atemp5

	addt	$f25,$f13,$f25
	unop
	mult	$f10,$f0,$f13
	LD	$f0,   5*SIZE($2)	# atemp6

	addt	$f26,$f14,$f26
	unop
	mult	$f10,$f19,$f14
	LD	$f19,  6*SIZE($2)	# atemp7

	addt	$f27,$f15,$f27
	unop
	mult	$f10,$f2,$f15
	LD	$f10,  2*SIZE($4)	# btemp2

	addt	$f12,$f11,$f12
 	LD	$f2,   7*SIZE($2)	# atemp8
	mult	$f1,$f22,$f11
	LD	$f22,  8*SIZE($2)	# atemp1

	addt	$f25,$f13,$f25
	unop
	mult	$f1,$f20,$f13
	LD	$f20,  9*SIZE($2)	# atemp2

	addt	$f26,$f14,$f26
	unop
	mult	$f1,$f18,$f14
	LD	$f18, 10*SIZE($2)	# atemp3

	addt	$f27,$f15,$f27
	lda	$2,   16*SIZE($2)	# a1_offset += 16
	mult	$f1,$f16,$f15
	LD	$f1,   4*SIZE($4)	# btemp1

	addt	$f12,$f11,$f12
	LD	$f16, -5*SIZE($2)	# atemp4
	mult	$f10,$f17,$f11
	LD	$f17, -4*SIZE($2)	# atemp5

	addt	$f25,$f13,$f25
	unop
	mult	$f10,$f0,$f13
	LD	$f0,  -3*SIZE($2)	# atemp6

	addt	$f26,$f14,$f26
	unop
	mult	$f10,$f19,$f14
	LD	$f19, -2*SIZE($2)	# atemp7

	addt	$f27,$f15,$f27
	unop
	mult	$f10,$f2,$f15
	LD	$f10,  6*SIZE($4)	# btemp2

	LD	$f2,  -1*SIZE($2)	# atemp8
	subl	$5, 1, $5		# l--
	lda	$4,    8*SIZE($4)	# b1_offset += 8
	bgt	$5,$L85			# if l>0 goto $L85

$L84:
	addt	$f12,$f11,$f12
	unop
	mult	$f1,$f22,$f11
	LD	$f22,  0*SIZE($2)	# atemp1

	addt	$f25,$f13,$f25
	unop
	mult	$f1,$f20,$f13
	LD	$f20,  1*SIZE($2)	# atemp2

	addt	$f26,$f14,$f26
	unop
	mult	$f1,$f18,$f14
	LD	$f18,  2*SIZE($2)	# atemp3

	addt	$f27,$f15,$f27
	unop
	mult	$f1,$f16,$f15
	LD	$f1,   0*SIZE($4)	# btemp1

	addt	$f12,$f11,$f12
	LD	$f16,  3*SIZE($2)	# atemp4
	mult	$f10,$f17,$f11
	LD	$f17,  4*SIZE($2)	# atemp5

	addt	$f25,$f13,$f25
	unop
	mult	$f10,$f0,$f13
	LD	$f0,   5*SIZE($2)	# atemp6

	addt	$f26,$f14,$f26
	unop
	mult	$f10,$f19,$f14
	LD	$f19,  6*SIZE($2)	# atemp7

	addt	$f27,$f15,$f27
	lda	$2,    8*SIZE($2)	# a1_offset += 8
	mult	$f10, $f2, $f15
	LD	$f10,  2*SIZE($4)	# b2_offset

	addt	$f12,$f11,$f12
	LD	$f2,  -1*SIZE($2)	# atemp8
	mult	$f1,$f22,$f11
	lda	$4,    4*SIZE($4)	# b1_offset ++

	addt	$f25,$f13,$f25
	mult	$f1,$f20,$f13
	addt	$f26,$f14,$f26
	mult	$f1,$f18,$f14

	addt	$f27,$f15,$f27
	mult	$f1,$f16,$f15
	addt	$f12,$f11,$f12
	mult	$f10,$f17,$f11

	addt	$f25,$f13,$f25
	mult	$f10,$f0,$f13
	addt	$f26,$f14,$f26
	mult	$f10,$f19,$f14

	addt	$f27,$f15,$f27
	unop
	mult	$f10,$f2,$f15
	unop
	.align 4

$L82:
	and	$7, 3, $5		# l = (min_l&3)
	beq	$5, $L87		# if l<=0 goto $L87

	LD	$f22, 0*SIZE($2)	# atemp1
	LD	$f1,  0*SIZE($4)	# b1_offset

	LD	$f20, 1*SIZE($2)	# atemp2
	LD	$f18, 2*SIZE($2)	# atemp3
	LD	$f16, 3*SIZE($2)	# atemp4
	lda	$2,   4*SIZE($2)	# a1_offset += 4

	subl	$5, 1, $5		# l--
	lda	$4,   2*SIZE($4)	# b1_offset += 2

	ble	$5,$L89			# if l<=0 goto $L89
	unop
	.align 4

$L90:
	addt	$f12,$f11,$f12
	unop
	mult	$f1,$f22,$f11
	LD	$f22,  0*SIZE($2)	# atemp1

	addt	$f25,$f13,$f25
	unop
	mult	$f1,$f20,$f13
	LD	$f20,  1*SIZE($2)	# atemp2

	addt	$f26,$f14,$f26
	unop
	mult	$f1,$f18,$f14
	LD	$f18,  2*SIZE($2)	# atemp3

	addt	$f27,$f15,$f27
	lda	$2,    4*SIZE($2)	# a1_offset += 4
	mult	$f1,$f16,$f15
	LD	$f1,   0*SIZE($4)	# btemp1

	LD	$f16, -1*SIZE($2)	# atemp4
	subl	$5, 1, $5		# l--
	lda	$4,    2*SIZE($4)	# b1_offset += 2
	bgt	$5, $L90		# if l>0 goto $L90
	.align 4

$L89:
	addt	$f12,$f11,$f12
	mult	$f1,$f22,$f11
	addt	$f25,$f13,$f25
	mult	$f1,$f20,$f13

	addt	$f26,$f14,$f26
	mult	$f1,$f18,$f14
	addt	$f27,$f15,$f27
	mult	$f1,$f16,$f15
	.align 4

$L87:
	addt	$f12,$f11,$f12
	LD	$f22, 0*SIZE($22)
	addt	$f25,$f13,$f25
	LD	$f20, 1*SIZE($22)

	addt	$f26,$f14,$f26
	LD	$f18, 2*SIZE($22)
	addt	$f27,$f15,$f27
	LD	$f16, 3*SIZE($22)

	mult	$f3,$f12,$f11
	mult	$f3,$f25,$f13
	mult	$f3,$f26,$f14
	mult	$f3,$f27,$f15

	addt	$f22,$f11,$f12
	addt	$f20,$f13,$f25
	addt	$f18,$f14,$f26
	addt	$f16,$f15,$f27

	ST	$f12, 0*SIZE($22)
	fclr	$f11
	ST	$f25, 1*SIZE($22)
	fclr	$f13
	ST	$f26, 2*SIZE($22)
	fclr	$f14
	ST	$f27, 3*SIZE($22)
	fclr	$f15

	subl	$8, 1, $8		# i--
	addq	$22, 4*SIZE, $22	# c1_offset += 4
	bgt	$8, $L81
	unop
	.align 4

$L79:
	and	$15,3,$8		# i = (min_i&3)
	beq	$8,$L29			# if i<=0 goto $L29
	unop
	unop
	.align 4

$L96:
	mov	$24, $2			# a1_offset = a_offset
	fclr	$f12
	addq	$24, SIZE, $24		# a_offset ++
	fclr	$f11

	mov	$0, $4			# b1_offset = b_offset
	mov	$7, $5			# l = min_l
	ble	$7,$L98			# if l<=0 goto $L98
	unop
	.align 4

$L100:
	LD	$f22, 0*SIZE($2)	# atemp1
	LD	$f1,  0*SIZE($4)	# btemp1

	lda	$2,   4*SIZE($2)	# a1_offset += 4
	lda	$4,   2*SIZE($4)	# b1_offset += 2

	addt	$f12,$f11,$f12
	subl	$5,1,$5			# l--
	mult	$f1,$f22,$f11
	bgt	$5,$L100		# if l>0 goto $L100
	.align 4

$L98:
	addt	$f12,$f11,$f12		# ctemp1 += temp1
	mult	$f3,$f12,$f11		# temp1 = alpha*ctemp1
	LD	$f22, 0*SIZE($22)	# atemp1 = *c1_offset
	addt	$f22,$f11,$f12		# ctemp1 = atemp1 + temp1

	subl	$8,1,$8			# i--
	ST	$f12, 0*SIZE($22)	# *c1_offset = ctemp1
	addq	$22, SIZE, $22		# c1_offset ++
	bgt	$8,$L96			# if i>0 goto $L98
	.align 4

$L29:
	addl	$11, P, $11		# is += P
	nop
#ifndef C_VERSION
	cmplt	$11,$16,$1		# is < m ?
#else
	cmplt	$11,$17,$1		# is < m ?
#endif
	bne	$1,$L30
	.align 4

$L9:
	addl	$14,R,$14		# js += R
	nop
#ifndef C_VERSION
	cmplt	$14,$17,$1		# js < n ?
#else
	cmplt	$14,$16,$1		# js < n ?
#endif
	bne	$1,$L10
	.align 4

$L4:
	addl	$3,Q,$3			# ls += Q
	nop
	cmplt	$3,$18,$1		# ls < k ??
	bne	$1,$L5
	.align 4

$L3:
	/* PMCTR STOP in case of Total Performance */

	mov	$29, $30			# Restore Stack Pointer
	ldq	$26, 0($30)
	ldq	$9,  8($30)
	ldq	$10,16($30)
	ldq	$11,24($30)
	ldq	$12,32($30)
	ldq	$13,40($30)
	ldq	$14,48($30)
	ldq	$15,56($30)
	ldt	$f2,64($30)
	ldt	$f3,72($30)
	ldt	$f4,80($30)
	ldq	$29,88($30)
	ldt	$f5,96($30)
	ldt	$f6,104($30)
	ldt	$f7,112($30)
	ldt	$f8,120($30)

	lda	$30, STACKSIZE($30)

	ret	$31,($26),1
	.end	ROUTINE

/* Finish !! */
