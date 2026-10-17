/* SPDX-License-Identifier: GPL-3.0-only */
/* Transparent AArch64 receiver adapter: preserve x1..x8 and stack args. */
__asm__(".text\n.align 2\n.global hub_tag_0\n.hidden hub_tag_0\n.type hub_tag_0,%function\nhub_tag_0:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#0\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#0]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_0,.-hub_tag_0\n");
extern void hub_tag_0(void);
__asm__(".text\n.align 2\n.global hub_tag_1\n.hidden hub_tag_1\n.type hub_tag_1,%function\nhub_tag_1:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#1\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#8]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_1,.-hub_tag_1\n");
extern void hub_tag_1(void);
__asm__(".text\n.align 2\n.global hub_tag_2\n.hidden hub_tag_2\n.type hub_tag_2,%function\nhub_tag_2:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#2\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#16]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_2,.-hub_tag_2\n");
extern void hub_tag_2(void);
__asm__(".text\n.align 2\n.global hub_tag_3\n.hidden hub_tag_3\n.type hub_tag_3,%function\nhub_tag_3:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#3\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#24]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_3,.-hub_tag_3\n");
extern void hub_tag_3(void);
__asm__(".text\n.align 2\n.global hub_tag_4\n.hidden hub_tag_4\n.type hub_tag_4,%function\nhub_tag_4:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#4\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#32]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_4,.-hub_tag_4\n");
extern void hub_tag_4(void);
__asm__(".text\n.align 2\n.global hub_tag_5\n.hidden hub_tag_5\n.type hub_tag_5,%function\nhub_tag_5:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#5\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#40]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_5,.-hub_tag_5\n");
extern void hub_tag_5(void);
__asm__(".text\n.align 2\n.global hub_tag_6\n.hidden hub_tag_6\n.type hub_tag_6,%function\nhub_tag_6:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#6\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#48]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_6,.-hub_tag_6\n");
extern void hub_tag_6(void);
__asm__(".text\n.align 2\n.global hub_tag_7\n.hidden hub_tag_7\n.type hub_tag_7,%function\nhub_tag_7:\n"
        "stp x29,x30,[sp,#-96]!\nmov x29,sp\n"
        "stp x0,x1,[sp,#16]\nstp x2,x3,[sp,#32]\n"
        "stp x4,x5,[sp,#48]\nstp x6,x7,[sp,#64]\nstr x8,[sp,#80]\n"
        "mov x1,#7\nldr x2,[sp,#8]\nbl hub_tag_cache_this\nmov x9,x0\n"
        "ldr x1,[sp,#24]\nldp x2,x3,[sp,#32]\nldp x4,x5,[sp,#48]\n"
        "ldp x6,x7,[sp,#64]\nldr x8,[sp,#80]\nmov x0,x9\n"
        "adrp x16,hub_tag_cache_originals\nadd x16,x16,:lo12:hub_tag_cache_originals\n"
        "ldr x16,[x16,#56]\nldp x29,x30,[sp],#96\nbr x16\n"
        ".size hub_tag_7,.-hub_tag_7\n");
extern void hub_tag_7(void);
