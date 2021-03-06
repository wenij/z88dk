/*
 *      Small C+ Compiler
 *
 *      Z80 Code Generator
 *
 *      $Id: codegen.c,v 1.46 2016-09-19 09:17:50 dom Exp $
 *
 *      21/4/99 djm
 *      Added some conditional code for tests of zero with a char, the
 *      expand char to int code will be removed at optimizatin stage
 *
 *      22/4/99 djm
 *      Major rewrite!! All operations have one single routine now
 *      so the compile might actually run quicker, and uses less
 *      of those dodgy pointers to functions
 *
 *      23/4/99 djm
 *      With a bit of luck this file will no contain all assembler
 *      related output, this means that if Gunther gets macros worked
 *      into z80asm, we can change the output of the compiler to be
 *      macros which we can then optimize a lot easier..hazzah!
 *
 *      21/1/2014 Stefano
 *      z80asm syntax is evolving, now we declare the public objects
 *      with 'EXTERN' and 'PUBLIC'.
 */

#include "ccdefs.h"
#include <time.h>

extern int check_lastop_was_comparison(LVALUE* lval);


extern char Filenorig[];

static void quikmult(int type, int32_t size, char preserve);
static void threereg(void);
static void fivereg(void);
static void sixreg(void);
static void loada(int n);

/*
 * Data for this module
 */

static int    donelibheader;
static char  *current_section = ""; /**< Name of the current section */

/* Begin a comment line for the assembler */

void comment(void)
{
    outbyte(';');
}

/* Put out assembler info before any code is generated */

void header(void)
{
    time_t tim;
    char* timestr;
    char* assembler = "z80 Module Assembler";

    outfmt(";%s\n",Banner);
    outfmt(";%s\n",Version);
    outfmt(";\n");

    if (ISASM(ASM_ASXX)) {
        assembler = "asxx";
    } else if (ISASM(ASM_VASM)) {
        assembler = "vasm";
    } else if (ISASM(ASM_GNU)) {
        assembler = "binutils";
    }

    outfmt(";\tReconstructed for %s\n", assembler);

    donelibheader = 0;
    if ((tim = time(NULL)) != -1) {
        timestr = ctime(&tim);
        outfmt(";\n");
        outfmt(";\tModule compile time: %s\n",timestr);
    }
    nl();
}

/*
 * Print the header for a library function, called from the preprocessor!
 */

void DoLibHeader(void)
{
    char filen[FILENAME_LEN + 1];
    char* incdir;
    char* segment;

    if (donelibheader)
        return;
    /*
     * Copy filename over (obtained by preprocessor), carefully skipping
     * over the quotes!
     */
    strncpy(filen, Filename + 1, strlen(Filename) - 2);
    filen[strlen(Filename) - 1] = '\0';
    changesuffix(filen, "");
    if (1) {
        char* ptr = filen;
        if (!isalpha(*ptr) && *ptr != '_') {
            memmove(ptr + 1, ptr, strlen(ptr) + 1);
            *ptr = 'X';
        }
        while (*ptr) {
            if (!isalnum(*ptr)) {
                *ptr = '_';
            }
            ptr++;
        }
        /* Compiling a program */
        if (ISASM(ASM_ASXX)) {
            outstr("\n\t.module\t");
        } else if (ISASM(ASM_Z80ASM)) {
            outstr("\n\tMODULE\t");
        } else {
            outstr("\n;\t module\t");
        }
        if (strlen(filen) && strncmp(filen, "<stdin>", 7)) {
            if ((segment = strrchr(filen, '/'))) /* Unix */
                ++segment;
            else if ((segment = strrchr(filen, '\\'))) /*DOG*/
                segment++;
            else if ((segment = strrchr(filen, ':'))) /*Amiga*/
                segment++;
            else
                segment = filen;
            outstr(segment);
        } else {
            /* This handles files produced by a filter cpp */
            strcpy(filen, Filenorig);
            if ((segment = strrchr(filen, '/'))) /* Unix */
                ++segment;
            else if ((segment = strrchr(filen, '\\'))) /*DOG*/
                segment++;
            else if ((segment = strrchr(filen, ':'))) /*Amiga*/
                segment++;
            else
                segment = filen;
            changesuffix(filen, ".c");
            outstr("scp_"); /* alpha id incase tmpfile is numeric */
            outstr(segment);
        }
        nl();
    }
    if (ISASM(ASM_ASXX)) {
        incdir = getenv("Z80_OZFILES");
        outstr("\n\n\t.include \"");
        if (incdir)
            outstr(incdir);
        outstr("z80_crt0.hdx\"\n\n");
        ol(".area _CODE\n");
        ol(".radix d\n");
        if (c_notaltreg) {
            ol(".globl\tsaved_hl");
            ol(".globl\tsaved_de");
        }
    } else if (ISASM(ASM_GNU)) {

    } else {
        outstr("\n\n\tINCLUDE \"z80_crt0.hdr\"\n\n\n");
        if (c_notaltreg) {
            ol("EXTERN\tsaved_hl");
            ol("EXTERN\tsaved_de");
        }
    }
    donelibheader = 1;
}

/* Print any assembler stuff needed after all code */
void trailer(void)
{
    outfmt("\n; --- End of Compilation ---\n");
}

/* Print out a name such that it won't annoy the assembler
 *      (by matching anything reserved, like opcodes.)
 */
void outname(const char* sname, char pref)
{
    if (pref) {
        outstr(Z80ASM_PREFIX);
    }
    outstr(sname);
}

/* Fetch a static memory cell into the primary register */
/* Can only have directly accessible things here...so should be
 * able to just check for far to see if we need to pick up second
 * bit of long pointer (check for type POINTER as well...
 */
void getmem(SYMBOL* sym)
{
    if (sym->ctype->kind == KIND_CHAR) {
        if ( (sym->ctype->isunsigned) == 0 )  {
#ifdef PREAPR00
            ot("ld\ta,(");
            outname(sym->name, dopref(sym));
            outstr(")\n");
            callrts("l_sxt");
#else
            ot("ld\thl,");
            outname(sym->name, dopref(sym));
            nl();
            callrts("l_gchar");
#endif

        } else {
            /* Unsigned char - new method - allows more optimizations! */
            ot("ld\thl,");
            outname(sym->name, dopref(sym));
            nl();
            ol("ld\tl,(hl)");
            ol("ld\th,0");
        }
#ifdef OLDLOADCHAR
        ot("ld\ta,(");
        outname(sym->name, dopref(sym));
        outstr(")\n");
        if (sym->ctype->isunsigned == 0 )
            callrts("l_sxt");
        else {
            ol("ld\tl,a");
            ol("ld\th,0");
        }

#endif
    } else if (sym->ctype->kind == KIND_DOUBLE) {
        address(sym);
        callrts("dload");
    } else if (sym->ctype->kind == KIND_LONG) {
        ot("ld\thl,(");
        outname(sym->name, dopref(sym));
        outstr(")\n");
        ot("ld\tde,(");
        outname(sym->name, dopref(sym));
        outstr("+2)\n");
    } else {
        /* this is for KIND_INT and get pointer..will need to change! */
        ot("ld\thl,(");
        outname(sym->name, dopref(sym));
        outstr(")\n");
        /* For long pointers...load de with name+2, then d,0 */
        if (sym->ctype->kind == KIND_CPTR) {
            ot("ld\tde,(");
            outname(sym->name, dopref(sym));
            outstr("+2)\n\tld\td,0\n");
        }
    }
}

/* Fetch the address of the specified symbol (from stack)
 */
int getloc(SYMBOL* sym, int off)
{
    int offs;
    offs = sym->offset.i - Zsp + off;
    vconst(offs);
    ol("add\thl,sp");
    return (offs);
}

/* Store the primary register into the specified */
/*      static memory cell */
void putmem(SYMBOL* sym)
{
    if (sym->ctype->kind == KIND_DOUBLE) {
        address(sym);
        callrts("dstore");
    } else {
        if (sym->ctype->kind == KIND_CHAR) {
            LoadAccum();
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("),a\n");
        } else if (sym->ctype->kind == KIND_LONG) {
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("),hl\n");
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("+2),de\n");
        } else if (sym->ctype->kind == KIND_CPTR) {
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("),hl\n");
            ol("ld\ta,e");
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("+2),a\n");
        } else {
            ot("ld\t(");
            outname(sym->name, dopref(sym));
            outstr("),hl\n");
        }
    }
}

/*
 *  Store type at TOS - used for initialising auto vars
 */

void StoreTOS(char typeobj)
{
    switch (typeobj) {
    case KIND_LONG:
        lpush();
        return;
    case KIND_CHAR:
        ol("dec\tsp");
        LoadAccum();
        mainpop();
        ol("ld\tl,a");
        zpush();
        Zsp--;
        return;
    case KIND_DOUBLE:
        dpush();
        return;
    /* KIND_CPTR..untested */
    case KIND_CPTR:
        ol("dec\tsp");
        ol("ld\ta,e");
        zpop(); /* pop de */
        ol("ld\te,a");
        zpushde();
        zpush();
        Zsp--;
        return;
    default:
        zpush();
        return;
    }
}

/*
 * Store the object at the frame position marked by offset
 * We already know that it's in range
 */

#ifdef USEFRAME

void PutFrame(char typeobj, int offset)
{
    SYMBOL* ptr;
    char flags;
    ptr = retrstk(&flags); /* Not needed but.. */
    switch (typeobj) {
    case KIND_CHAR:
        ot("ld\t");
        OutIndex(offset);
        outstr(",l\n");
        break;
    case KIND_INT:
    case KIND_PTR:
        ot("ld\t");
        OutIndex(offset);
        outstr(",l\n");
        ot("ld\t");
        OutIndex(offset + 1);
        outstr(",h\n");
        break;
    case KIND_CPTR:
    case KIND_LONG:
        ot("ld\t");
        OutIndex(offset);
        outstr(",l\n");
        ot("ld\t");
        OutIndex(offset + 1);
        outstr(",h\n");
        ot("ld\t");
        OutIndex(offset + 2);
        outstr(",e\n");
        ot("ld\t");
        if (typeobj == KIND_LONG) {
            OutIndex(offset + 3);
            outstr(",d\n");
        }
    }
}
#endif

/* Store the specified object type in the primary register */
/*      at the address on the top of the stack */
void putstk(LVALUE *lval)
{
    char flags = 0;
    SYMBOL *ptr;
    Kind typeobj = lval->indirect_kind;


    //outfmt("; %s type=%d val_type=%d indirect=%d\n", lval->ltype->name, lval->type, lval->val_type, lval->indirect_kind);
    /* Store via long pointer... */
    ptr = retrstk(&flags);
    //outfmt(";Restore %p flags %02d\n",ptr, flags);
    if (flags & FARACC) {
        /* exx pop hl, pop de, exx */
        doexx();
        mainpop();
        zpop();
        doexx();
        switch (typeobj) {
        case KIND_DOUBLE:
            callrts("lp_pdoub");
            break;
        case KIND_CPTR:
            callrts("lp_pptr");
            break;
        case KIND_LONG:
            callrts("lp_plong");
            break;
        case KIND_CHAR:
            callrts("lp_pchar");
            break;
        default:
            callrts("lp_pint");
        }
        return;
    }

    switch (typeobj) {
    case KIND_DOUBLE:
        mainpop();
        callrts("dstore");
        break;
    case KIND_CPTR:
        zpopbc();
        callrts("l_putptr");
        break;
    case KIND_LONG:
        zpopbc();
        callrts("l_plong");
        break;
    case KIND_CHAR:
        zpop();
        LoadAccum();
        ol("ld\t(de),a");
        break;
    default:
        zpop();
        callrts("l_pint");
    }
}

/* store a two byte object in the primary register at TOS */
void puttos(void)
{
#ifdef USEFRAME
    if (c_framepointer_is_ix != -1) {
        ot("ld\t");
        OutIndex(0);
        outstr(",l\n");
        ot("ld\t");
        OutIndex(1);
        outstr(",h\n");
        return;
    }
#endif
    ol("pop\tbc");
    ol("push\thl");
}

/* store a two byte object in the primary register at 2nd TOS */
void put2tos(void)
{
#ifdef USEFRAME
    if (c_framepointer_is_ix != -1) {
        ot("ld\t");
        OutIndex(2);
        outstr(",l\n");
        ot("ld\t");
        OutIndex(3);
        outstr(",h\n");
        return;
    }
#endif
    ol("pop\tde");
    puttos();
    ol("push\tde");
}

/*
 * loadargc - load accumulator with number of words of stack
 *            if n=0 then emit xor a instead of ld a,0 
 *            (this is picked up by the optimizer, but even so)
 */
void loadargc(int n)
{
    n >>= 1;
    loada(n);
}

static void loada(int n)
{
    if (n) {
        ot("ld\ta,");
        outdec(n);
        nl();
    } else
        ol("xor\ta");
}

/* Fetch the specified object type indirect through the */
/*      primary register into the primary register */
void indirect(LVALUE* lval)
{
    char sign;
    char flags;
    Kind typeobj;

    typeobj = lval->indirect_kind;
    flags = lval->flags;

    sign = lval->ltype->isunsigned;
    
    /* Fetch from far pointer */
    if (flags & FARACC) { /* Access via far method */
        switch (typeobj) {
        case KIND_CHAR:
            callrts("lp_gchar");
            if (!sign)
                callrts("l_sxt");
            /*                        else ol("ld\th,0"); */
            break;
        case KIND_CPTR:
            callrts("lp_gptr");
            break;
        case KIND_LONG:
            callrts("lp_glong");
            break;
        case KIND_DOUBLE:
            callrts("lp_gdoub");
            break;
        default:
            callrts("lp_gint");
        }
        return;
    }

    switch (typeobj) {
    case KIND_CHAR:
        if (!sign) {
#ifdef PREAPR00
            ol("ld\ta,(hl)");
            callrts("l_sxt");
#else
            callrts("l_gchar");
#endif
        } else {
            ol("ld\tl,(hl)");
            ol("ld\th,0");
        }
        break;
    case KIND_CPTR:
        callrts("l_getptr");
        break;
    case KIND_LONG:
        callrts("l_glong");
        break;
    case KIND_DOUBLE:
        callrts("dload");
        break;
    default:
        ot("call\tl_gint\t;");
#ifdef USEFRAME
        if (c_framepointer_is_ix != -1 && CheckOffset(lval->offset)) {
            OutIndex(lval->offset);
        }
#endif
        nl();
    }
}

/* Swap the primary and secondary registers */
void swap(void)
{
    ol("ex\tde,hl");
}

/* Print partial instruction to get an immediate value */
/*      into the primary register */
void immed(void)
{
    if (ISASM(ASM_ASXX))
        ot("ld\thl,#");
    else
        ot("ld\thl,");
}

/* Print partial instruction to get an immediate value */
/*      into the secondary register */
void immed2(void)
{
    if (ISASM(ASM_ASXX))
        ot("ld\tde,#");
    else
        ot("ld\tde,");
}

/* Partial instruction to access literal */
void immedlit(int lab)
{
    immed();
    queuelabel(lab);
    outbyte('+');
}

/* Push long onto stack */

void lpush(void)
{
    zpushde();
    zpush();
}

void lpush2(void)
{
    callrts("lpush2");
    Zsp -= 4;
}

/* Push and pop flags (used for ? operator) */

void zpushflags(void)
{
    ol("push\taf");
    Zsp -= 2;
}

void zpopflags(void)
{
    ol("pop\taf");
    Zsp += 2;
}

/* Push secondary register/high work of long onto the stack */

void zpushde(void)
{
    ol("push\tde");
    Zsp -= 2;
}

/* Push the primary register onto the stack */

void zpush(void)
{
    ol("push\thl");
    Zsp -= 2;
}

/* Push the primary floating point register onto the stack */

void dpush(void)
{
    callrts("dpush");
    Zsp -= 6;
}

/* Push the primary floating point register, preserving
        the top value  */

void dpush_under(int val_type)
{
    if ( val_type == KIND_LONG ) {
        callrts("dpush3");
    } else {
        callrts("dpush2");
    }
    Zsp -= 6;
}

/* Pop the top of the stack into the primary register */
void mainpop(void)
{
    ol("pop\thl");
    Zsp += 2;
}

/* Pop the top of the stack into the secondary register */
void zpop(void)
{
    ol("pop\tde");
    Zsp += 2;
}

/* Pop top of stack into bc */

void zpopbc(void)
{
    ol("pop\tbc");
    Zsp += 2;
}

/* Swap af & af' (preserve carry) */

static void doexaf(void)
{
    ol("ex\taf,af");
}

/* Swap between the sets of registers */

void doexx(void)
{
    ol("exx");
}

/* Swap the primary register and the top of the stack */
void swapstk(void)
{
    ol("ex\t(sp),hl");
}

/* process switch statement */
void sw(char type)
{
    if (type == KIND_LONG || type == KIND_CPTR)
        callrts("l_long_case");
    else
        callrts("l_case");
}

/* Call a shared library routine FIXME!!!!
 * Dunno which one myself and Garry are gonna hijack yet!
 */

void zclibcallop()
{
    ol("rst\t8");
    defword();
}



/* Output the call op code */

void zcallop(void)
{
    ot("call\t");
}

/* djm (move this!) Decide whether to print a prefix or not 
 * This uses new flags bit LIBRARY
 */

char dopref(SYMBOL* sym)
{
    if (sym->ctype->flags & LIBRARY && (sym->ctype->kind == KIND_FUNC ) ) { // || sym->ident == FUNCTIONP)) {
        return (0);
    }
    return (1);
}

/* Call a run-time library routine */
void callrts(char* sname)
{
    ot("call\t");
    outstr(sname);
    nl();
}

/* Return from subroutine */
void zret(void)
{
    ol("ret");
    nl();
    nl();
}

/*
 * Perform subroutine call to value on top of stack
 * Put arg count in A in case subroutine needs it
 */
void callstk(Type *type, int n)
{
    if (n == 2) {
        /* At this point, TOS = function, hl = argument */
        swapstk();
    }
    
    if ( type->hasva )
        loadargc(n);
    callrts("l_jphl");
}

void jump0(LVALUE* lval, int label)
{
    opjump("", label);
}

/* Jump to specified internal label number */
void jump(int label)
{
    opjump("", label);
}

/* Jump relative to specified internal label */
void jumpr(int label)
{
    opjumpr("", label);
}

/*
 * Output the jump code, with conditions as needed
 */

void opjump(char* cc, int label)
{
    ot("jp\t");
    outstr(cc);
    printlabel(label);
    nl();
}

void opjumpr(char* cc, int label)
{
    ot("jr\t");
    outstr(cc);
    printlabel(label);
    nl();
}

void jumpc(int label)
{
    opjump("c,", label);
}

void jumpnc(int label)
{
    opjump("nc,", label);
}

void setcond(int val)
{
    if (val == 1)
        ol("scf");
    else
        ol("and\ta");
}

/* Test the primary register and jump if false to label */
void testjump(LVALUE* lval, int label)
{
    int type;
    ol("ld\ta,h");
    ol("or\tl");

    if (lval->oldval_kind == KIND_LONG) {
        ol("or\td");
        ol("or\te");
    }
    type = lval->oldval_kind;
    if (lval->binop == NULL)
        type = lval->val_type;

    if (type == KIND_LONG && check_lastop_was_comparison(lval)) {
        ol("or\td");
        ol("or\te");
    }
    if (type == KIND_CPTR && check_lastop_was_comparison(lval)) {
        ol("or\te");
    }
    opjump("z,", label);
}

/* test primary register against zero and jump if false */
/* Special conditions for testing char here */
void zerojump(
    void (*oper)(LVALUE*, int),
    int label,
    LVALUE* lval)
{
    clearstage(lval->stage_add, 0); /* purge conventional code */
    lval->stage_add = NULL;
#ifdef CHARCOMP0
    if (lval->oldval_kind == KIND_CHAR) {
        if (oper == testjump) { /* !=0 or >=0U */
            LoadAccum();
            ol("and\ta");
            opjump("z,", label);
            return;
        } else if (oper == le0) { /* <=0 */
            LoadAccum();
            ol("and\ta");
            if (IS_ASM(ASM_Z80ASM)) {
                ol("jr\tz,ASMPC+5");
            } else {
                ol("jr\tz,$+5");
            }
            opjump("p,", label);
            return;
        } else if (oper == ge0) { /* > 0 */
            LoadAccum();
            ol("and\ta");
            opjump("m,", label);
            return;
        } else if (oper == gt0) {
            LoadAccum();
            ol("and\ta");
            opjump("m,", label);
            opjump("z,", label);
            return;
        }
    }
#endif
    (*oper)(lval, label);
}

/* Print pseudo-op to define a byte */
void defbyte(void)
{
    if (ISASM(ASM_ASXX))
        ot(".db\t");
    else
        ot("defb\t");
}

/*Print pseudo-op to define storage */
void defstorage(void)
{
    if (ISASM(ASM_ASXX))
        ot(".ds\t");
    else if (ISASM(ASM_Z80ASM)) {
        ot("defs\t");
    } else {
        ot("defs\t");
    }
}

/* Print pseudo-op to define a word */
void defword(void)
{
    if (ISASM(ASM_ASXX))
        ot(".dw\t");
    else
        ot("defw\t");
}

/* Print pseudo-op to dump a long */
void deflong(void)
{
    if (ISASM(ASM_Z80ASM))
        ot("defq\t");
    else
        ot("defl\t");
}

/* Print pseudo-op to define a string */
void defmesg(void)
{
    if (ISASM(ASM_ASXX) || ISASM(ASM_GNU))
        ot(".ascii\t\"");
    else
        ot("defm\t\"");
}

/* Point to following object */
void point(void)
{
    if (ISASM(ASM_ASXX))
        ol(".dw\t.+2");
    else
        ol("defw\tASMPC+2");
}

/* Modify the stack pointer to the new value indicated 
 * \param newsp - Where we need to be 
 * \param save - NO or the variable type that we need to preserve
 * \param saveaf - Whether we should save af
 */
int modstk(int newsp, int save, int saveaf)
{
    int k, flag = NO;

    k = newsp - Zsp;

    if (k == 0)
        return newsp;
    if ( (c_cpu & CPU_RABBIT) && abs(k) > 1 && abs(k) <= 127 ) {
        outstr("\tadd\tsp,"); outdec(k); nl();
        return newsp;
    }

#ifdef USEFRAME
    if (c_framepointer_is_ix != -1)
        goto modstkcht;
#endif
    if (k > 0) {
        if (k < 11) {
            if (k & 1) {
                ol("inc\tsp");
                --k;
            }
            while (k) {
                ol("pop\tbc");
                k -= 2;
            }
            return newsp;
        }
    }
    if (k < 0) {
        if (k > -11) {
            if (k & 1) {
                flag = YES;
                ++k;
            }
            while (k) {
                ol("push\tbc");
                k += 2;
            }
            if (flag)
                ol("dec\tsp");
            return newsp;
        }
    }
/*
 * These doexx() here swap() but if we return a long then we've fubarred
 * up!
 */
#ifdef USEFRAME
modstkcht:
#endif
    if (saveaf) {
        if (c_notaltreg) {
            zpushflags();
            zpopbc();
        } else {
            doexaf();
        }
    }
#ifdef USEFRAME
    if (c_framepointer_is_ix != -1) {
        ot("ld\t");
        FrameP();
        outstr(",");
        outdec(k);
        nl();
        ot("add\t");
        FrameP();
        outstr(",sp\n");
        RestoreSP(NO);
    } else {
        if (save)
            doexx();
        vconst(k);
        ol("add\thl,sp");
        ol("ld\tsp,hl");
        if (save)
            doexx();
    }
#else
    if (save) {
        if (c_notaltreg) savehl();
        else doexx();
    }
    vconst(k);
    ol("add\thl,sp");
    ol("ld\tsp,hl");
    if (save) {
        if (c_notaltreg) restorehl();
        else doexx();
    }
#endif
    if (saveaf) {
        if (c_notaltreg) {
            ol("push\tbc");
            Zsp -= 2;
            zpopflags();
        } else {
            doexaf();
        }
    }
    return newsp;
}

/* Multiply the primary register by the length of some variable */
void scale(Kind type, Type *tag)
{
    switch (type) {
    case KIND_INT:
    case KIND_PTR:
        ol("add\thl,hl");;
        break;
    case KIND_CPTR:
        threereg();
        break;
    case KIND_LONG:
        ol("add\thl,hl");;
        ol("add\thl,hl");;
        break;
    case KIND_DOUBLE:
        sixreg();
        break;
    case KIND_STRUCT:
        /* try to avoid multiplying if possible */
        quikmult(KIND_INT, tag->size, YES);
        break;
    default:
        break;
    }
}




void quikmult(int type, int32_t size, char preserve)
{
    if ( type == KIND_LONG ) {
        /* Normal long multiplication is:
           push, push, ld hl, ld de, call l_long_mult = 11 bytes
        */
        switch ( size ) {
            case 0:
                vlongconst(0);
                break;
            case 1:
                break;  
            case 65536:
                ol("ex\tde,hl");
                vconst(0);
                break;
            case 256:  /* 5 bytes */
                ol("ld\td,e");
                ol("ld\te,h");
                ol("ld\th,l");
                ol("ld\tl,0");
                break;      
            case 8: /* 15 bytes */
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");  
                /* Fall through */              
            case 4: /* 10 bytes */
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");  
                /* Fall through */            
            case 2: /* 5 bytes */
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");   
                break;
            case 3: /* 13 bytes */
                ol("push\tde");
                ol("push\thl");
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");   
                ol("pop\tbc");
                ol("add\thl,bc");
                ol("pop\tbc");
                ol("ex\tde,hl");
                ol("adc\thl,bc");
                ol("ex\tde,hl");
                break;
            case 6:  /* 19 bytes */
                ol("push\tde");
                ol("push\thl");
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");   
                ol("pop\tbc");
                ol("add\thl,bc");
                ol("pop\tbc");
                ol("ex\tde,hl");
                ol("adc\thl,bc");
                ol("ex\tde,hl");
                ol("add\thl,hl");
                ol("rl\te");
                ol("rl\td");  
                break;
            case 5: /* 19 bytes */
                ol("push\tde");
                ol("push\thl");
                ol("add\thl,hl");;
                ol("rl\te");
                ol("rl\td");  
                ol("add\thl,hl");;
                ol("rl\te");
                ol("rl\td"); 
                ol("pop\tbc"); 
                ol("add\thl,bc");
                ol("pop\tbc");
                ol("ex\tde,hl");
                ol("adc\thl,bc");
                ol("ex\tde,hl");
                break;
            default:
                lpush();       
                vlongconst(size);
                callrts("l_long_mult");
                Zsp += 4;
        }
        return;
    }


    switch (size) {
    case 0:
        vconst(0);
        break;
    case 2048:
        ol("ld\th,l"); /* 6 bytes, 44T */
        ol("ld\tl,0");
        ol("add\thl,hl");
        ol("add\thl,hl");
        ol("add\thl,hl");
        break;
    case 1024:
        ol("ld\th,l"); /* 5 bytes, 33T */
        ol("ld\tl,0");
        ol("add\thl,hl");
        ol("add\thl,hl");
        break;
    case 512:
        ol("ld\th,l");  /* 4 bytes, 22T */
        ol("ld\tl,0");
        ol("add\thl,hl");
        break;
    case 256:
        ol("ld\th,l"); /* 3 bytes, 11T */
        ol("ld\tl,0");
        break;
    case 1:
        break;
    case 64:
        ol("add\thl,hl");  /* 6 bytes, 66T, (RCM) 6 bytes, 12T */
    case 32:
        ol("add\thl,hl");
    case 16:
        ol("add\thl,hl");
    case 8:
        ol("add\thl,hl");
    case 4:
        ol("add\thl,hl");
    case 2:
        ol("add\thl,hl");
        break;
    case 12:
        ol("add\thl,hl");
    case 6:
        sixreg();
        break;
    case 9:
        threereg();
    case 3:
        threereg();
        break;
    case 15:
        threereg();
    case 5:
        fivereg();
        break;
    case 10:
        fivereg();
        ol("add\thl,hl");
        break;
    case 14:
        ol("add\thl,hl");
    case 7:
        sixreg();
        ol("add\thl,bc");  /* BC contains original value */
        break;
    case 65535:
    case -1:
        callrts("l_neg");
        break;
    default:
        if (preserve)
            ol("push\tde");
        const2(size);
        callrts("l_mult"); /* WATCH OUT!! */
        if (preserve)
            ol("pop\tde");
        break;
    }
}





/* Multiply the primary register by three */
static void threereg(void)
{
    ol("ld\tb,h");
    ol("ld\tc,l");
    ol("add\thl,bc");
    ol("add\thl,bc");
}

/* Multiply the primary register by five */
static void fivereg(void)
{
    ol("ld\tb,h");
    ol("ld\tc,l");
    ol("add\thl,hl");;
    ol("add\thl,hl");;
    ol("add\thl,bc");
}

/* Multiply the primary register by six */
static void sixreg(void)
{
    threereg();
    ol("add\thl,hl");;
}

/*
 * New routines start here! What we do is have a single routine for
 * each operation type, the routine takes an lval, and it all works
 * out well..honest!
 */

/* Add the primary and secondary registers (result in primary) */
void zadd(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if ( c_size_optimisation & OPT_ADD32 ) {
            ol("pop\tbc");        /* 7 bytes, 54T */
            ol("add\thl,bc");
            ol("ex\tde,hl");
            ol("pop\tbc");
            ol("adc\thl,bc");
            ol("ex\tde,hl");
        } else {
            callrts("l_long_add"); /* 3 bytes, 76 + 17 = 93T */
        }
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dadd");
        Zsp += 6;
        break;
    default:
        ol("add\thl,de");
    }
}


void zadd_const(LVALUE *lval, int32_t value)
{
    switch (value) {
    case -3:
        dec(lval);
    case -2:
        dec(lval);
    case -1:
        dec(lval);
    case 0:
        break;
    case 3:
        inc(lval);
    case 2:
        inc(lval);
    case 1:
        inc(lval);
        break;
    default:
        if ( lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR ) {
            // 11 bytes, 54T vs 11 bytes + l_long_add ( 11 + 11 + 10 + 10 + 17  + 76 = 135T)
            constbc(((uint32_t)value) % 65536);   // 3, 10
            ol("add\thl,bc");                     // 1, 11
            ol("ex\tde,hl");                      // 1, 4
            constbc(((uint32_t)value) / 65536);   // 3, 10
            ol("adc\thl,bc");                     // 2, 15
            ol("ex\tde,hl");                      // 1, 4
        } else {
            addbchl(value);
        }
    }
}

/* Subtract the primary register from the secondary */
/*      (results in primary) */
void zsub(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if ( c_size_optimisation & OPT_SUB32 ) {
            ol("ld\tc,l");        /* 13 bytes: 4 + 4 + 10 + 4 + 15 + 4  + 4 + 4 + 10 + 15 + 4 = 78T */
            ol("ld\tb,h");
            ol("pop\thl");        
            ol("and\ta");
            ol("sbc\thl,bc");
            ol("ex\tde,hl");
            ol("ld\tc,l");
            ol("ld\tb,h");
            ol("pop\thl");
            ol("sbc\thl,bc");
            ol("ex\tde,hl");
        } else {
            callrts("l_long_sub"); /* 3 bytes: 100 + 17T = 117t */
        }
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dsub");
        Zsp += 6;
        break;
    default:
        if ( c_size_optimisation & OPT_SUB16 ) {
            swap();
            ol("and\ta");
            ol("sbc\thl,de");
        } else {
            callrts("l_sub");
        }
    }
}

/* Multiply the primary and secondary registers */
/*      (results in primary */
void mult(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_mult");
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dmul");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if ( lval->ltype->isunsigned && c_cpu == CPU_Z180 ) {
            ot("ld\th,e\n");
            ot("mlt\thl\n");
            break;
        }
    default:
        callrts("l_mult"); 
    }
}

void mult_const(LVALUE *lval, int32_t value)
{
    quikmult(lval->val_type, value, NO);
}



/* Divide the secondary register by the primary */
/*      (quotient in primary, remainder in secondary) */
void zdiv(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_div_u");
        else
            callrts("l_long_div");
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("ddiv");
        Zsp += 6;
        break;
    default:
        if (utype(lval))
            callrts("l_div_u");
        else
            callrts("l_div");
    }
}

void zdiv_const(LVALUE *lval, int32_t value)
{
    if ( lval->val_type == KIND_LONG && utype(lval) ) {
        if ( value == 256 ) {
            ol("ld\tl,h");
            ol("ld\th,e");
            ol("ld\te,d");
            ol("ld\td,0");
            return;
        } else if ( value == 65536 ) {
            swap();
            const2(0);
            return;
        }
    } else if ( utype(lval) ) {
        if ( value == 512 ) {
            ol("ld\tl,h");
            ol("ld\th,0");
            ol("srl\tl");
            return;
        } else if ( value == 256 ) {
            ol("ld\tl,h");
            ol("ld\th,0");
            return;
        }
    }

    switch ( value ) {
        case 1:
            break;
        case 2:
            asr_const(lval,1);
            break;
        case 4:
            asr_const(lval,2);
            break;
        case 8:
            asr_const(lval,3);
            break;
        default:
            if ( lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR ) {
                lpush();
                vlongconst(value);
            } else {
                const2(value);
                swap();
            }
            zdiv(lval);
    }
}

/* Compute remainder (mod) of secondary register divided
 *      by the primary
 *      (remainder in primary, quotient in secondary)
 */
void zmod(LVALUE* lval)
{
    if (c_notaltreg && (lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR)) {
        callrts("l_long_mod2");
    } else {
        zdiv(lval);
        if (lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR)
            doexx();
        else
            swap();
    }
}

void zmod_const(LVALUE *lval, int32_t value)
{
    LVALUE  templval={0};

    if ( lval->val_type == KIND_LONG ) {
        if ( value <= 256 && value > 0 ) {
            // Fall through into int handling
        } else if ( value == 65536 ) {
            const2(0);
            return;
        } else if ( value == 65536 * 256 ) {
            ol("ld\td,0");
            return;
        } else {
            lpush();
            vlongconst(value);
            zmod(lval);
            return;
        }
    } 

    templval.val_type = KIND_INT;
    if ( utype(lval) ) 
        templval.ltype = type_uint;
    else
        templval.ltype = type_int;
    switch ( value ) {
        case 256:
            ol("ld\th,0");
            break;
        case 1:
            vconst(0);
            break;
        case 2:
            zand_const(&templval,1);
            break;
        case 4:
            zand_const(&templval, 3);
            break;
        case 8:
            zand_const(&templval,7);
            break;
        case 16:
            zand_const(&templval,15);
            break;
        case 32:
            zand_const(&templval, 31);
            break;
        case 64:
            zand_const(&templval,63);
            break;
        case 128:
            zand_const(&templval,127);
            break;
        default:
            const2(value);
            swap();
            zmod(&templval);
    }
    if ( lval->val_type == KIND_LONG ) 
        const2(0);
}

/* Inclusive 'or' the primary and secondary */
/*      (results in primary) */
void zor(LVALUE* lval )
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_or");
        Zsp += 4;
        break;
    default:
        callrts("l_or");
    }
}


int zor_handle_pow2(int32_t value) 
{
    int c = 0;
    switch ( value ) {
        case 0:
            return 1;
        case 0x80:
            c++;
        case 0x40:
            c++;
        case 0x20:
            c++;
        case 0x10:
            c++;
        case 0x08:
            c++;
        case 0x04:
            c++;
        case 0x02:
            c++;
        case 0x01:
            c++;
            outfmt("\tset\t%d,l\n",c-1);
            break;
        case 0x8000:
            c++;
        case 0x4000:
            c++;
        case 0x2000:
            c++;
        case 0x1000:
            c++;
        case 0x800:
            c++;
        case 0x400:
            c++;
        case 0x200:
            c++;
        case 0x100:
            c++;
            outfmt("\tset\t%d,h\n",c-1);
            break;
        case 0x800000:
            c++;
        case 0x400000:
            c++;
        case 0x200000:
            c++;
        case 0x100000:
            c++;
        case 0x80000:
            c++;
        case 0x40000:
            c++;
        case 0x20000:
            c++;
        case 0x10000:
            c++;
            outfmt("\tset\t%d,e\n",c-1);
            break;
        case 0x80000000:
            c++;
        case 0x40000000:
            c++;
        case 0x20000000:
            c++;
        case 0x10000000:
            c++;
        case 0x8000000:
            c++;
        case 0x4000000:
            c++;
        case 0x2000000:
            c++;
        case 0x1000000:
            c++;
            outfmt("\tset\t%d,d\n",c-1);
            break;           
    }
    return c;
}


void zor_const(LVALUE *lval, int32_t value)
{
    if ( lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR) {
        if ( zor_handle_pow2(value) ) {
            return;
        } else if ( (value & 0xFFFFFF00) == 0 ) {
            ol("ld\ta,l");
            ot("or\t"); outdec(value % 256); nl();
            ol("ld\tl,a");
        } else if ( ( value & 0xFFFF00FF) == 0 ) {
            ol("ld\ta,h");
            ot("or\t"); outdec((value % 65536)/256); nl();
            ol("ld\th,a");            
       } else if ( ( value & 0xFF00FFFF) == 0 ) {
            ol("ld\ta,e");
            ot("or\t"); outdec((value / 65536)%256); nl();
            ol("ld\te,a");            
       } else if ( ( value & 0x00FFFFFF) == 0 ) {
            ol("ld\ta,d");
            ot("or\t"); outdec((value / 65536)/256); nl();
            ol("ld\td,a");            
        } else if ( value != 0 ) {
            lpush();
            vlongconst(value);
            zor(lval);
        }
    } else {
        if ( zor_handle_pow2(value % 65536) ) {
            return;
        } else if ( ((value % 65536) & 0xff00) == 0 ) {
            ol("ld\ta,l");
            ot("or\t"); outdec(value % 256); nl();
            ol("ld\tl,a");    
        } else if ( ((value % 65536) & 0x00ff) == 0 ) {
            ol("ld\ta,h");
            ot("or\t"); outdec((value % 65536) / 256); nl();
            ol("ld\th,a");    
        } else if ( value != 0 ) {
            const2(value);
            zor(lval);
        }        
    }
}

/* Exclusive 'or' the primary and secondary */
/*      (results in primary) */
void zxor(LVALUE *lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_xor");
        Zsp += 4;
        break;
    default:
        callrts("l_xor");
    }
}

void zxor_const(LVALUE *lval, int32_t value)
{
    if ( lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR) {
        if ( (value & 0xFFFFFF00) == 0 ) {
            ol("ld\ta,l");
            ot("xor\t"); outdec(value % 256); nl();
            ol("ld\tl,a");
        } else if ( ( value & 0xFFFF00FF) == 0 ) {
            ol("ld\ta,h");
            ot("xor\t"); outdec((value % 65536)/256); nl();
            ol("ld\th,a");            
       } else if ( ( value & 0xFF00FFFF) == 0 ) {
            ol("ld\ta,e");
            ot("xor\t"); outdec((value / 65536)%256); nl();
            ol("ld\te,a");            
       } else if ( ( value & 0x00FFFFFF) == 0 ) {
            ol("ld\ta,d");
            ot("xor\t"); outdec((value / 65536)/256); nl();
            ol("ld\td,a");  
        } else if ( ( value & 0xffffffff) == 0xffffffff ) {
            com(lval);          
        } else if ( value != 0 ) {
            lpush();
            vlongconst(value);
            zxor(lval);
        }
    } else {
        if ( ((value % 65536) & 0xff00) == 0 ) {
            ol("ld\ta,l");
            ot("xor\t"); outdec(value % 256); nl();
            ol("ld\tl,a");    
        } else if ( ((value % 65536) & 0x00ff) == 0 ) {
            ol("ld\ta,h");
            ot("xor\t"); outdec((value % 65536) / 256); nl();
            ol("ld\th,a");   
        } else if ( ( value & 0xffff) == 0xffff ) {
            com(lval);
        } else if ( value != 0 ) {
            const2(value);
            zxor(lval);
        }        
    }
}


/* 'And' the primary and secondary */
/*      (results in primary) */
void zand(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_and");
        Zsp += 4;
        break;
    default:
        callrts("l_and");
    }
}

void zand_const(LVALUE *lval, int32_t value)
{
    if ( lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR) {
        if ( value == 0 ) {
            vlongconst(0);
        } else if ( value == 0xff ) {  // 5
            ol("ld\th,0");
            const2(0);
        } else if ( value >= 0 && value < 256 ) { // 9 bytes
            ol("ld\ta,l");
            ot("and\t"); outdec(value % 256); nl();
            ol("ld\tl,a");
            ol("ld\th,0");
            const2(0);
        } else if ( value == 0xffff ) { // 3 bytes
            const2(0);
        } else if ( value == 0xffffff ) { // 2 bytes
            ol("ld\td,0");
        } else if ( value == 0xffffffff ) {
            // Do nothing
        } else if ( (value & 0xffffff00) == 0xffffff00 ) {
           // Only the bottom 8 bits
           ol("ld\ta,l");
           outfmt("\tand\t%d\n",(value & 0xff));
           ol("ld\tl,a");
        } else if ( (value & 0xffff00ff) == 0xffff00ff  ) {
           // Only the bits 15-8
           ol("ld\ta,h");
           outfmt("\tand\t%d\n",(value & 0xff00)>>8);
           ol("ld\th,a");
        } else if ( (value & 0xff00ffff ) == 0xff00ffff) {
           // Only the bits 23-16
           ol("ld\ta,e");
           outfmt("\tand\t%d\n",(value & 0xff0000)>>16);
           ol("ld\te,a");
        } else if ( (value & 0x00ffffff) == 0x00ffffff ) {
           // Only the bits 32-23
           ol("ld\ta,d");
           outfmt("\tand\t%d\n",(value & 0xff000000) >> 24);
           ol("ld\td,a");
        } else { // 13 bytes
            lpush(); // 4
            vlongconst(value); // 6
            zand(lval); // 3
        }
    } else {
        if ( value == 0 ) {
            vconst(0);
        } else if ( (value % 65536) == 0xff ) {
            ol("ld\th,0");
        } else if ( value >= 0 && value < 256 ) {
            // 6 bytes, library call is 6 bytes, this is faster
            ol("ld\ta,l");
            ot("and\t"); outdec(value % 256); nl();
            ol("ld\tl,a");
            ol("ld\th,0");
        } else if ( value % 256 == 0 ) {
            ol("ld\ta,h");
            ot("and\t"); outdec( (value % 65536) / 256); nl();
            ol("ld\th,a");
            ol("ld\tl,0");            
        } else if ( value == 0xffff ) {
            // Do nothing
        } else {
            const2(value);
            zand(lval);
        }
    }
}

/* Arithmetic shift right the secondary register number of */
/*      times in primary (results in primary) */
void asr(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_asr_u");
        else
            callrts("l_long_asr");
        Zsp += 4;
        break;
    default:
        if (utype(lval))
            callrts("l_asr_u");
        else
            callrts("l_asr");
    }
}

void asr_const(LVALUE *lval, int32_t value)
{
    if  (lval->val_type == KIND_LONG || lval->val_type == KIND_CPTR ) {
        if ( value == 1 ) {
            if ( utype(lval) ) { /* 8 bytes, 8 + 8 + 8 + 8 + 8 = 40T */
                ol("srl\td");
            } else {
                ol("sra\td");
            }
            ol("rr\te");
            ol("rr\th");
            ol("rr\tl");
        } else if ( value == 8 && utype(lval) )  {
            ol("ld\tl,h"); /* 5 bytes, 4 + 4 + 4 +7 = 19T */
            ol("ld\th,e");
            ol("ld\te,d");
            ol("ld\td,0");
        } else if ( value == 9 && utype(lval) ) {
            ol("ld\tl,h");  /* 11 bytes, 4+ 4 +4 +7 + 8 +8 + 8 = 43T */
            ol("ld\th,e");
            ol("ld\te,d");
            ol("ld\td,0");
            ol("srl\te");
            ol("rr\th");
            ol("rr\tl");
        } else if ( value == 10 && utype(lval) && (c_size_optimisation & OPT_RSHIFT32) )  {
            ol("ld\tl,h"); /* 17 bytes, 19 + 48 = 67T */
            ol("ld\th,e");
            ol("ld\te,d");
            ol("ld\td,0");
            ol("srl\te");
            ol("rr\th");
            ol("rr\tl");
            ol("srl\te");
            ol("rr\th");
            ol("rr\tl");
        } else if ( (value == 11 || value == 12 || value == 13 || value == 14) && utype(lval) ) {
            ol("ld\tl,h"); /* 12 bytes */
            ol("ld\th,e");
            ol("ld\te,d");
            ol("ld\td,0");
            ot("ld\tc,"); outdec(value -8); nl();
            callrts("l_long_asr_uo");
        } else if ( value == 15 && utype(lval)) {
            ol("ex\tde,hl"); /* 10 bytes, 45T */
            ol("rl\td");                // Lowest bit
            ol("adc\thl,hl");
            ol("ld\tde,0");
            ol("rl\te");
        } else if ( value == 16 && utype(lval)) {
            ol("ex\tde,hl"); /* 4 bytes 14T */
            ol("ld\tde,0");
        } else if ( value == 17 && utype(lval)) {
            ol("srl\td"); /* 8 bytes 30T */
            ol("rr\te");
            ol("ex\tde,hl");
            ol("ld\tde,0");
        } else if ( value == 18 && utype(lval) ) {
            ol("ld\thl,0"); /* 12 bytes, 46T */
            ol("ex\tde,hl");
            ol("srl\th");
            ol("rr\tl");
            ol("srl\th");
            ol("rr\tl");
        } else if ( value == 20 && utype(lval) && (c_size_optimisation & OPT_RSHIFT32) ) {
            ol("ex\tde,hl"); /* 20 bytes, 78T */
            ol("ld\tde,0");
            ol("srl\th");
            ol("rr\tl");
            ol("srl\th");
            ol("rr\tl");
            ol("srl\th");
            ol("rr\tl");
            ol("srl\th");
            ol("rr\tl");
        } else if ( value == 23 && utype(lval)) {
            ol("ld\tl,d"); /* 12 bytes, 37T */
            ol("rl\te");
            ol("rl\tl");
            ol("ld\th,0");
            ol("rl\th");
            ol("ld\tde,0");
        } else if ( value == 24 && utype(lval)) {
            ol("ld\tl,d"); /* 6 bytes , 21T */
            ol("ld\th,0");
            ol("ld\tde,0");
        } else if ( value == 25 && utype(lval)) {
            ol("ld\tl,d"); /* 8 bytes, 29T */
            ol("srl\tl");
            ol("ld\th,0");
            ol("ld\tde,0");
        } else if ( value == 27 && utype(lval)) {
            ol("ld\tl,d"); /* 12 bytes, 47T */
            ol("srl\tl");
            ol("srl\tl");
            ol("srl\tl");
            ol("ld\th,0");
            ol("ld\tde,0");
        } else if ( value == 30 && utype(lval) && (c_size_optimisation & OPT_RSHIFT32)) {
            ol("ld\tl,0"); /* 15 bytes, 51T */
            ol("rl\td");
            ol("rl\tl");
            ol("rl\td");
            ol("rl\tl");
            ol("ld\th,0");
            ol("ld\tde,0");
        } else if  ( value == 31 && utype(lval)) {
            ol("ld\tl,0"); /* 12 bytes, 40T */
            ol("rl\td");
            ol("rl\tl");
            ol("ld\th,0");
            ol("ld\tde,0");
        } else if ( value != 0 ) {
            value &= 31;
            if ( value >= 16 && utype(lval)) {  /* 7 bytes */
                ot("ld\thl,");outdec( value - 16); nl(); /* We don't want it marked as const otherwise it gets optimised away */
                callrts("l_asr_u");
                ol("inc\te");
            } else {
                lpush();  /* 11 bytes, optimised to 5 */
                vlongconst(value);
                asr(lval);
            }
        }
    } else {
        if ( value == 1 ) { /* 4 bytes, 16T */
            if ( utype(lval) ) {
                ol("srl\th");
            } else {
                ol("sra\th");
            }
            ol("rr\tl");
        } else if ( value == 8 && utype(lval) ) { /* 3 bytes, 11T */
            ol("ld\tl,h");
            ol("ld\th,0");
        } else if ( value == 15 && utype(lval) ) {
            ol("rl\th");   /* 7 bytes, 26T */
            vconst(0);
            ol("rl\tl");
        } else if ( value != 0 ) {
            const2(value);
            swap();
            asr(lval);
        }
    }
}


/* Arithmetic left shift the secondary register number of */
/*      times in primary (results in primary) */
void asl(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_asl");
        Zsp += 4;
        break;
    default:
        callrts("l_asl");
    }
}

void asl_16bit_const(LVALUE *lval, int value)
{
    switch ( value ) {
        case 0:
            return;
        case 10:  // 7 bytes, 8 + 8 + 4 + 7 = 27T
            ol("sla\tl");
            ol("sla\tl");
            ol("ld\th,l");
            ol("ld\tl,0");
            break;
        case 9: // 6 bytes, 8 + 4 + 7 = 19T
            ol("sla\tl"); 
            ol("ld\th,l");
            ol("ld\tl,0");
            break;
        case 8: // 3 bytes, 4 + 7 = 11T
            ol("ld\th,l");
            ol("ld\tl,0");
        break;
        case 7:
            if ( c_size_optimisation & OPT_LSHIFT32 ) {
                ol("rr\th");  // 9 bytes, 8 + 4  + 8 + 7 + 8 = 35T
                ol("ld\th,l");
                ol("rr\th");
                ol("ld\tl,0");
                ol("rr\tl");
                break;
            }
            ol("add\thl,hl");  // 77T
        case 6:
            ol("add\thl,hl");  // 66T
            // Fall through
        case 5:
            ol("add\thl,hl");  // 55T
        case 4:
            ol("add\thl,hl"); // 44T
        case 3:
            ol("add\thl,hl"); // 33T
        case 2:
            ol("add\thl,hl"); // 22T
        case 1:
            ol("add\thl,hl"); // 11T
            break;
        default: // 7 bytes
            if ( value >= 16 ) {
                warningfmt("Left shifting by more than the size of the object");
                vconst(0);
            } else {
                const2(value);
                swap();
                callrts("l_asl");
            }
            break;
    }
}

void asl_const(LVALUE *lval, int32_t value)
{
    if ( lval->val_type == KIND_LONG  ) { 
        switch ( value ) {
        case 0: 
            return;
        case 24: // 6 bytes, 4 + 7 + 10 = 21T
            ol("ld\td,l");
            ol("ld\te,0");
            vconst(0);
            break;
        case 17: // 5 bytes
            ol("add\thl,hl");  // 5 bytes, 11 + 4 + 10 = 25T
            // Fall through
        case 16: // 4 bytes
            swap();
            vconst(0);
            break;
        case 8: // 5 bytes, 4 + 4 + 4 +7 = 19T
            ol("ld\td,e");
            ol("ld\te,h");
            ol("ld\th,l");
            ol("ld\tl,0");
            break;         
        case 1: /* 5 bytes, 11 + 8 + 8 = 27T */
            ol("add\thl,hl");;
            ol("rl\te");
            ol("rl\td");   
            break;
        case 9:
        case 10:
        case 11:
        case 12: 
            // Shift by 8, 10 bytes, 4 + 4 + 4+ 7 = 19T + 
            ol("ld\td,e");
            ol("ld\te,h");
            ol("ld\th,l");
            ol("ld\tl,0");
            loada( value - 8 ); 
            callrts("l_long_aslo");
            break;
        default: //  5 bytes
            if ( value >= 32 ) warningfmt("Left shifting by more than the size of the object");
            value &= 31;
            if (  value >= 16 ) {
                asl_16bit_const(lval, value - 16);
                swap();
                ot("ld\thl,"); outdec(0); nl();
            } else {
                loada( value );
                callrts("l_long_aslo");
            }
            break;
        }
    } else {
        asl_16bit_const(lval, value);
    }
}


static void set_carry(LVALUE *lval)
{
    lval->val_type = KIND_CARRY;
    lval->ltype = type_carry;
}


/* Form logical negation of primary register */
void lneg(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        lval->val_type = KIND_INT;
        lval->ltype = type_int;
        callrts("l_long_lneg");
        break;
    case KIND_CARRY:
        set_carry(lval);
        ol("ccf");
        break;
    case KIND_DOUBLE:
        convdoub2int();
    default:
        set_carry(lval);
        callrts("l_lneg");
    }
}

/* Form two's complement of primary register */
void neg(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_neg");
        break;
    case KIND_DOUBLE:
        callrts("minusfa");
        break;
    default:
        callrts("l_neg");
    }
}

/* Form one's complement of primary register */
void com(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_long_com");
        break;
    default:
        callrts("l_com");
    }
}


/*
 * Increment value held in main register
 */

void inc(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_DOUBLE:
        // FA = value to be incremented
        dpush();
        vlongconst(1);
        convSlong2doub();
        callrts("dadd");
        Zsp += 6;
        break;
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_inclong");
        break;
    default:
        ol("inc\thl");
    }
}

/*
 * Decrement value held in main register
 */

void dec(LVALUE* lval)
{
    switch (lval->val_type) {
    case KIND_DOUBLE:
        // FA = value to be incremented
        dpush();
        vlongconst(-1);
        convSlong2doub();
        callrts("dadd");
        Zsp += 6;
        break;
    case KIND_LONG:
    case KIND_CPTR:
        callrts("l_declong");
        break;
    default:
        ol("dec\thl");
    }
}

/* Following are the conditional operators */
/* They compare the secondary register against the primary */
/* and put a literal 1 in the primary if the condition is */
/* true, otherwise they clear the primary register */

void dummy(LVALUE *lval)
{
    /* Dummy function to allows us to check for c/nc at end of if clause */
}

/* test for equal to zero */
void eq0(LVALUE* lval, int label)
{
    check_lastop_was_comparison(lval);
    switch (lval->oldval_kind) {
#ifdef CHARCOMP0
    case KIND_CHAR:
        ol("ld\ta,l");
        ol("and\ta");
        break;
#endif
    case KIND_LONG:
        ol("ld\ta,h");
        ol("or\tl");
        ol("or\td");
        ol("or\te");
        break;
    case KIND_CPTR:
        ol("ld\ta,e");
        ol("or\th");
        ol("or\tl");
        break;
    default:
        ol("ld\ta,h");
        ol("or\tl");
    }
    opjump("nz,", label);
}

void lt0(LVALUE* lval, int label)
{
    switch (lval->oldval_kind) {
#ifdef CHARCOMP0
    case KIND_CHAR:
        ol("xor\ta");
        ol("or\tl");
        break;
#endif
    case KIND_LONG:
        ol("xor\ta");
        ol("or\td");
        break;
    case KIND_CPTR:
        ol("xor\ta");
        ol("or\te");
        break;
    default:
        ol("xor\ta");
        ol("or\th");
    }
    opjump("p,", label);
}

/* Test for less than or equal to zero */
void le0(LVALUE* lval, int label)
{
    ol("ld\ta,h");
    ol("or\tl");
    if (lval->oldval_kind == KIND_LONG) {
        ol("or\td");
        ol("or\te");
    }
    if (lval->oldval_kind == KIND_CPTR) {
        ol("or\te");
    }
    if (ISASM(ASM_Z80ASM)) {
        ol("jr\tz,ASMPC+7");
    } else {
        ol("jr\tz,$+7");
    }
    lt0(lval, label);
}

/* test for greater than zero */
void gt0(LVALUE* lval, int label)
{    
    ge0(lval, label);
    if (lval->oldval_kind == KIND_LONG) {
        ol("or\th");
        ol("or\te");
    }
    if (lval->oldval_kind == KIND_CPTR) {
        ol("or\th");
    }
    ol("or\tl");
    opjump("z,", label);
}

/* test for greater than or equal to zero */
void ge0(LVALUE* lval, int label)
{
    ol("xor\ta");
    switch (lval->oldval_kind) {
    case KIND_LONG:
        ol("or\td");
        break;
    case KIND_CPTR:
        ol("or\te");
        break;
    default:
        ol("or\th");
    }
    opjump("m,", label);
}


/* Test for equal */
void zeq(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        set_carry(lval);
        callrts("l_long_eq");
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("deq");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            set_carry(lval);
            ol("ld\ta,l");
            ol("sub\te");
            ol("and\ta");
            if (ISASM(ASM_Z80ASM)) {
                ol("jr\tnz,ASMPC+3");
            } else {
                ol("jr\tnz,$+3");
            }
            ol("scf");
            break;
        }
    default:
        set_carry(lval);
        callrts("l_eq");
    }
}

/* Test for not equal */
void zne(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        set_carry(lval);
        callrts("l_long_ne");
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dne");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            set_carry(lval);
            ol("ld\ta,l");
            ol("sub\te");
            ol("and\ta");
            if (ISASM(ASM_Z80ASM)) {
                ol("jr\tz,ASMPC+3");
            } else {
                ol("jr\tz,$+3");
            }
            ol("scf");
            break;
        }
    default:
        set_carry(lval);
        callrts("l_ne");
    }
}

/* Test for less than*/
void zlt(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_ult");
        else
            callrts("l_long_lt");
        Zsp += 4;
        set_carry(lval);        
        break;
    case KIND_DOUBLE:
        callrts("dlt");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            if (utype(lval)) {
                ol("ld\ta,e");
                ol("sub\tl");
            } else {
                ol("ld\ta,e");
                ol("sub\tl");
                ol("rra");
                ol("xor\te");
                ol("xor\tl");
                ol("rlca");
            }
            set_carry(lval);
            break;
        }
    default:
        if (utype(lval))
            callrts("l_ult");
        else
            callrts("l_lt");
        set_carry(lval);            
    }
}

/* Test for less than or equal to (signed/unsigned) */
void zle(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_ule");
        else
            callrts("l_long_le");
        set_carry(lval);            
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dleq");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            if (utype(lval)) { /* unsigned */
                ol("ld\ta,e");
                ol("sub\tl"); /* If l < e then carry set */
                if (ISASM(ASM_Z80ASM)) {
                    ol("jr\tnz,ASMPC+3"); /* If zero, then set carry */
                } else {
                    ol("jr\tnz,$+3"); /* If zero, then set carry */
                }
                ol("scf");
            } else {
                int label = getlabel();
                ol("ld\ta,e");
                ol("sub\tl");
                ol("rra");
                ol("scf");
                opjumpr("z,", label);
                ol("xor\te");
                ol("xor\tl");
                ol("rlca");
                postlabel(label);
            }
            set_carry(lval);
            break;
        }
    default:
        if (utype(lval))
            callrts("l_ule");
        else
            callrts("l_le");
        set_carry(lval);            
    }
}

/* Test for greater than (signed/unsigned) */
void zgt(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_ugt");
        else
            callrts("l_long_gt");
        set_carry(lval);            
        Zsp += 4;
        break;
    case KIND_DOUBLE:
        callrts("dgt");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            if (utype(lval)) {
                ol("ld\ta,e");
                ol("sub\tl");
                if (ISASM(ASM_Z80ASM)) {
                    ol("jr\tz,ASMPC+3"); /* If zero, nc */
                } else {
                    ol("jr\tz,$+3"); /* If zero, nc */
                }
                ol("ccf");
            } else {
                ol("ld\ta,e");
                ol("sub\tl");
                ol("rra");
                ol("xor\te");
                ol("xor\tl");
                ol("rlca");
                ol("ccf");
            }
            set_carry(lval);
            break;
        }
    default:
        if (utype(lval))
            callrts("l_ugt");
        else
            callrts("l_gt");
        set_carry(lval);            
    }
}

/* Test for greater than or equal to */
void zge(LVALUE* lval)
{
    lval->oldval_kind = lval->val_type;
    lval->ptr_type = KIND_NONE;
    switch (lval->val_type) {
    case KIND_LONG:
    case KIND_CPTR:
        if (utype(lval))
            callrts("l_long_uge");
        else
            callrts("l_long_ge");
        Zsp += 4;
        set_carry(lval);        
        break;
    case KIND_DOUBLE:
        callrts("dge");
        Zsp += 6;
        break;
    case KIND_CHAR:
        if (c_doinline) {
            if (utype(lval)) {
                ol("ld\ta,l");
                ol("sub\te"); /* If l > e, carry set */
                if (ISASM(ASM_Z80ASM)) {
                    ol("jr\tnz,ASMPC+3"); /* If l == e then we need to set carry */
                } else {
                    ol("jr\tnz,$+3"); /* If l == e then we need to set carry */
                }
                ol("scf");
            } else {
                int label = getlabel();
                ol("ld\ta,e");
                ol("sub\tl");
                ol("rra");
                ol("scf");
                opjumpr("z,", label);
                ol("xor\te");
                ol("xor\tl");
                ol("rlca");
                ol("ccf");
                postlabel(label);
            }
            set_carry(lval);
            break;
        }
    default:
        if (utype(lval))
            callrts("l_uge");
        else
            callrts("l_ge");
        set_carry(lval);            
    }
}

void zcarryconv(void)
{
    // vconst(0);
    // ol("rl\tl");
}

/*
 *      Routines for conversion between different types, kept in this
 *      file to aid conversion etc
 */

void convUint2char(void)
{
    ol("ld\th,0");
}

void convSint2char(void)
{
    ol("ld\ta,l");
    callrts("l_sxt");
}

/* Unsigned int to long */
void convUint2long(void)
{
    const2(0);
}

/* Signed int to long */
void convSint2long(void)
{
    callrts("l_int2long_s");
}

/* signed Int to doub */
void convSint2doub(void)
{
    callrts("float");
}

/* unsigned int to double */

void convUint2doub(void)
{
    callrts("ufloat");
}

/* signed long to double */
void convSlong2doub(void)
{
    convSint2doub();
}

/* unsigned long to double */
void convUlong2doub(void)
{
    convUint2doub();
}

/* double to integerl/long */
void convdoub2int(void)
{
    callrts("ifix");
}

/* Swap double positions on stack */

void DoubSwap(void)
{
    callrts("dswap");
}

/*
 * Load long into hl and de 
 * Takes respect of sign, so if signed and high word=0 then
 * print 65535 else print whats there..could possibly come unstuck!
 * this is so that -1 -> -32768 are correcly represented
 *
 * djm 21/2/99 fixed, so that sign is disregarded! this allows us
 * to have -1 entered correctly
 */

void vlongconst(uint32_t val)
{
    vconst(val % 65536);
    const2(val / 65536);
}


void vlongconst_tostack(uint32_t val)
{
    constbc(val / 65536);
    ol("push\tbc");
    constbc(val % 65536);
    ol("push\tbc");
    Zsp -= 4;
}

void vlongconst_noalt(uint32_t val)
{
    constbc(val / 65536);
    ol("push\tbc");
    constbc(val % 65536);
    ol("push\tbc");
    Zsp -= 4;
}

/*
 * load constant into primary register
 */
void vconst(int32_t val)
{
    if (val < 0)
        val += 65536;
    immed();
    outdec(val % 65536);
    ot(";const\n");
}

/*
 * load constant into secondary register
 */
void const2(int32_t val)
{
    if (val < 0)
        val += 65536;
    immed2();
    outdec(val);
    nl();
}

void constbc(int32_t val)
{
    if (val < 0)
        val += 65536;
    ot("ld\tbc,");
    outdec(val);
    nl();
}

void addbchl(int val)
{
    if ( c_cpu == CPU_Z80ZXN ) {
        ot("add\thl,");
        outdec(val); nl();
    } else {
        ot("ld\tbc,");
        outdec(val);
        outstr("\n\tadd\thl,bc\n");
    }
}

/* Load accumulator with lower half of int */

void LoadAccum(void)
{
    ol("ld\ta,l");
}

/* Compare the accumulator with a value (mod 256) */

void CpCharVal(int val)
{
    ot("cp\t#(");
    outdec(val);
    outstr("% 256)\n");
}

/*
 *      Print prefix for global defintion
 */

void GlobalPrefix(void)
{
    if (ISASM(ASM_ASXX)) {
        ot(".globl\t");
    } else if (ISASM(ASM_VASM)) {
        ot("GLOBAL\t");
    } else if (ISASM(ASM_GNU)) {
        ot(".global\t");
    } else {
        ot("GLOBAL\t");
    }
}


/*
 *  Emit a LINE opcode for assembler
 *  error reporting
 */

void EmitLine(int line)
{
    char filen[FILENAME_LEN];
    char  *ptr;

    snprintf(filen, sizeof(filen),"%s", Filename[0] == '\"'? Filename + 1 : Filename);
    if ( (ptr = strrchr(filen,'\"')) != NULL ) {
        *ptr = 0;
    }

    if (ISASM(ASM_Z80ASM) && (c_cline_directive || c_intermix_ccode)) {
        outfmt("\tC_LINE\t%d,\"%s\"\n", line, filen);
    }
}

/* These routines save and restore hl/de from special places */

void savehl(void)
{
    ol("ld\t(saved_hl),hl");
}

void savede(void)
{
    ol("ld\t(saved_de),de");
}

void restorehl(void)
{
    ol("ld\thl,(saved_hl)");
}

void restorede(void)
{
    ol("ld\tde,(saved_de)");
}

/* Prefix for assembler */

void prefix()
{
    if (ISASM(ASM_Z80ASM)) {
        outbyte('.');
    }
}

/* Print specified number as label */
void printlabel(int label)
{
    if (ISASM(ASM_ASXX)) {
        outdec(label);
        outstr("$");
    } else {
        outfmt("i_%d", label);            
    }
}

/* Print a label suffix */
void col()
{
    if (!ISASM(ASM_Z80ASM))
        outbyte(58);
}

void function_appendix(SYMBOL* func)
{
    /* Asz80 needs a label at the end to sort out local symbols */
    if (ISASM(ASM_ASXX)) {
        nl();
        prefix();
        outstr("smce_");
        outname(func->name, NO);
        col();
        nl();
    }
}

void output_section(char* section_name)
{
    /* If the same section don't do anything */
    if (strcmp(section_name, current_section) == 0) {
        return;
    }
    if (ISASM(ASM_ASXX)) {
        outfmt("\t.area\t%s\n", section_name);
    } else if (ISASM(ASM_Z80ASM)) {
        outfmt("\tSECTION\t%s\n", section_name);
    } else if (ISASM(ASM_VASM)) {
        outfmt("\tSECTION\t%s\n", section_name);
    } else if (ISASM(ASM_GNU)) {
        outfmt("\t.section\t%s\n", section_name);
    }
    current_section = section_name;
}

#ifdef USEFRAME
/*
 * Check offset is within range for frame pointer
 */

int CheckOffset(int val)
{
    if (val >= -126 && val <= 127)
        return 1;
    return 0;
}

/*
 *  Output offset to index register
 *
 *  FRAME POINTER STUFF IS BROKEN - DO NOT USE!!!
 */

void OutIndex(int val)
{
    if (ISASM(ASM_ASXX)) {
        outdec(val);
        if (c_framepointer_is_ix)
            outstr("(ix)");
        else
            outstr("(iy)");
    } else {
        outstr("(");
        if (c_framepointer_is_ix)
            outstr("ix ");
        else
            outstr("iy ");
        if (val >= 0)
            outstr("+");
        outdec(val);
        outstr(")");
    }
}

void RestoreSP(char saveaf)
{
    if (saveaf)
        doexaf();
    ot("ld\tsp,");
    FrameP();
    nl();
    if (saveaf)
        doexaf();
}

void setframe(void)
{
#ifdef USEFRAME
    if (c_framepointer_is_ix == -1)
        return;
    ot("ld\t");
    FrameP();
    outstr(",0\n");
    ot("add\t");
    FrameP();
    outstr(",sp\n");
#endif
}

#endif

void FrameP(void)
{
    outstr(c_framepointer_is_ix ? "ix" : "iy");
}

void pushframe(void)
{
    if (c_framepointer_is_ix != -1 || (currfn->ctype->flags & (SAVEFRAME|NAKED)) == SAVEFRAME ) {
        ot("push\t");
        FrameP();
        nl();
    }
}

void popframe(void)
{
    if (c_framepointer_is_ix != -1 || (currfn->ctype->flags & (SAVEFRAME|NAKED)) == SAVEFRAME ) {
        ot("pop\t");
        FrameP();
        nl();
    }
}

void gen_builtin_strcpy()
{
    int label;
    // hl holds src on entry, on stack= dest
    ol("pop\tde");
    ol("push\tde");
    ol("xor\ta");
    label = getlabel();
    postlabel(label);
    ol("cp\t(hl)");
    ol("ldi");
    outstr("\tjr\tnz,");
    printlabel(label);
    nl();
    ol("pop\thl");
}


void gen_builtin_strchr(int32_t c)
{
    int startlabel, endlabel;
    if ( c == -1 ) {
        /* hl = c, stack = buffer */
        ol("ex\tde,hl");
        ol("pop\thl");
        Zsp += 2;
    } else {
        /* hl = buffer */
        outstr("\tld\te,"); outdec(c % 256); nl();
    }
    startlabel = getlabel();
    endlabel = getlabel();
    postlabel(startlabel);
    ol("ld\ta,(hl)");
    ol("cp\te");
    outstr("\tjr\tz,");
    printlabel(endlabel); nl();
    ol("and\ta");
    ol("inc\thl");
    outstr("\tjr\tnz,");
    printlabel(startlabel); nl();
    ol("ld\th,a");
    ol("ld\tl,h");
    postlabel(endlabel);
}

void gen_builtin_memset(int32_t c, int32_t s)
{
    if ( c == -1 ) {
        /* Entry hl = c, on stack = buffer */
        ol("ex\tde,hl");  /* c */
        ol("pop\thl");  /* buffer */
        Zsp += 2;
    } else {
        /* hl is buffer - data load happens a bit later*/
    }
    ol("push\thl");

    /* Now decide what to do about the count */
    if ( s < 4 ) {
        int i;
        for ( i = 0; i < s; i++ ) {
            if ( i  != 0 ) {
                ol("inc\thl");
            }
            if ( c != -1 ) {
                outstr("\tld\t(hl),"); outdec(c % 256); nl();
            } else {
                ol("ld\t(hl),e");
            }
        }
    } else if ( s < 256 ) {
        int looplabel = getlabel();
        if ( c != -1 ) {
            outstr("\tld\te,"); outdec(c % 256); nl();
        }
        outstr("\tld\tb,"); outdec(s); nl();
        postlabel(looplabel);
        ol("ld\t(hl),e");
        ol("inc\thl");
        outstr("\tdjnz\t"); printlabel(looplabel); nl();
    } else {
        if ( c != -1 ) {
            outstr("\tld\t(hl),"); outdec(c % 256); nl();
        } else {
            ol("ld\t(hl),e");
        }
        ol("ld\td,h");
        ol("ld\te,l");
        ol("inc\tde");
        outstr("\tld\tbc,"); outdec((s % 65536) - 1); nl();
        ol("ldir");
    }
    ol("pop\thl");
}

void gen_builtin_memcpy(int32_t src, int32_t n)
{
    if ( src == -1 ) {
        /* Entry hl = src, on stack = dst */
        ol("pop\tde");  /* dst */
        ol("push\tde");
        Zsp += 2;
        outstr("\tld\tbc,"); outdec(n % 65536); nl();
        ol("ldir");
    } else {
        /* hl is dst */
        ol("push\thl");
        ol("ex\tde,hl");
        outstr("\tld\thl,"); outdec(src % 65536); nl();
        outstr("\tld\tbc,"); outdec(n % 65536); nl();
        ol("ldir");
    }
    ol("pop\thl");
}

void copy_to_stack(char *label, int stack_offset,  int size)
{
    vconst(stack_offset);
    ol("add\thl,sp");  
    ol("ex\tde,hl");
    outstr("\tld\thl,"); outname(label, 1); nl();
    outfmt("\tld\tbc,%d\n",size);
    ol("ldir");
}

void copy_to_extern(const char *src, const char *dest, int size)
{
    if ( size == 1 ) {
        outfmt("\tld\ta,(_%s)\n",src);  // 6 bytes
        outfmt("\tld\t(_%s),a\n",dest);
    } else if ( size == 2 ) {
        outfmt("\tld\thl,(_%s)\n",src);  // 6 bytes
        outfmt("\tld\t(_%s),hl\n",dest);
    } else {
        outfmt("\tld\thl,_%s\n",src);  // 11 bytes
        outfmt("\tld\tde,_%s\n",dest);
        outfmt("\tld\tbc,%d\n",size);
        outfmt("\tldir\n",src);
    }
}


void intrinsic_in(SYMBOL *sym)
{
    if ( c_cpu & CPU_RABBIT ) {
        ol("ioi");
        outstr("\tld\thl,("); outname(sym->name, 1); outstr(")"); nl();
        if ( c_cpu == CPU_R2K ) {
            ol("nop"); // Rabbit bug workaround
        }
        return;
    }
    if (sym->type == KIND_PORT8 ) {
        if ( c_cpu == CPU_Z180 ) {
            outstr("\tin0\tl,("); outname(sym->name, 1); outstr(")"); nl();
        } else {
            outstr("\tin\ta,("); outname(sym->name, 1); outstr(")"); nl();
            ol("ld\tl,a");
        }
        ol("ld\th,0");
    } else {
        outstr("\tld\ta,");  outname(sym->name, 1); outstr(" / 256"); nl();
        outstr("\tin\ta,("); outname(sym->name, 1); outstr(" % 256)"); nl();
        ol("ld\tl,a");
        ol("ld\th,0");
    }
}

void intrinsic_out(SYMBOL *sym)
{
    if ( c_cpu & CPU_RABBIT ) {
        ol("ld\ta,l");
        ol("ioi");
        outstr("\tld\t("); outname(sym->name, 1); outstr("),a"); nl();
        if ( c_cpu == CPU_R2K ) {
            ol("nop"); // Rabbit bug workaround
        }
        return;
    }
    if (sym->type == KIND_PORT8 ) {
        if ( c_cpu == CPU_Z180 ) {
            outstr("\tout0\t("); outname(sym->name, 1); outstr("),l"); nl();
        } else {
            ol("ld\ta,l");
            outstr("\tout\t("); outname(sym->name, 1); outstr("),a"); nl();
        }
    } else {
        ol("ld\ta,l");
        outstr("\tld\tbc,"); outname(sym->name, 1);  nl();
        ol("out\t(c),a");
    }
}


void zentercritical(void)
{
    if ( c_cpu & CPU_RABBIT ) {
        ol("ipset\t3");
    } else {
        callrts("l_push_di");
    }
}

void zleavecritical(void)
{
    if ( c_cpu & CPU_RABBIT ) {
        ol("ipres");
    } else {
        callrts("l_pop_ei");
    }
}

int zcriticaloffset(void)
{
    if ( c_cpu & CPU_RABBIT ) {
        return 0;
    }
    return 2;
}

void push_char_sdcc_style(void)
{
    ol("ld\tb,l");
    ol("push\tbc");
    ol("inc\tsp");
    Zsp--;
}


/*
 * Local Variables:
 *  indent-tabs-mode:nil
 *  require-final-newline:t
 *  c-basic-offset: 4
 *  eval: (c-set-offset 'case-label 0)
 *  eval: (c-set-offset 'substatement-open 0)
 *  eval: (c-set-offset 'access-label 0)
 *  eval: (c-set-offset 'class-open 4)
 *  eval: (c-set-offset 'class-close 4)
 * End:
 */
