/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

#if 1

 .text
 .align 2
 TRAPHANDLER_NOEC(__handler0, 0)
 TRAPHANDLER_NOEC(__handler1, 1)
 TRAPHANDLER_NOEC(__handler2, 2)
 TRAPHANDLER_NOEC(__handler3, 3)
 TRAPHANDLER_NOEC(__handler4, 4)
 TRAPHANDLER_NOEC(__handler5, 5)
 TRAPHANDLER_NOEC(__handler6, 6)
 TRAPHANDLER_NOEC(__handler7, 7)
 TRAPHANDLER(__handler8, 8)
 TRAPHANDLER_NOEC(__handler9, 9)
 TRAPHANDLER(__handler10, 10)
 TRAPHANDLER(__handler11, 11)
 TRAPHANDLER(__handler12, 12)
 TRAPHANDLER(__handler13, 13)
 TRAPHANDLER(__handler14, 14)
 TRAPHANDLER(__handler15, 15)
 TRAPHANDLER_NOEC(__handler16, 16)
 TRAPHANDLER(__handler17, 17)
 TRAPHANDLER_NOEC(__handler18, 18)
 TRAPHANDLER_NOEC(__handler19, 19)
 TRAPHANDLER_NOEC(__handler20, 20)
 TRAPHANDLER_NOEC(__handler21, 21)
 TRAPHANDLER_NOEC(__handler22, 22)
 TRAPHANDLER_NOEC(__handler23, 23)
 TRAPHANDLER_NOEC(__handler24, 24)
 TRAPHANDLER_NOEC(__handler25, 25)
 TRAPHANDLER_NOEC(__handler26, 26)
 TRAPHANDLER_NOEC(__handler27, 27)
 TRAPHANDLER_NOEC(__handler28, 28)
 TRAPHANDLER_NOEC(__handler29, 29)
 TRAPHANDLER_NOEC(__handler30, 30)
 TRAPHANDLER_NOEC(__handler31, 31)
 TRAPHANDLER_NOEC(__handler32, 32)
 TRAPHANDLER_NOEC(__handler33, 33)
 TRAPHANDLER_NOEC(__handler34, 34)
 TRAPHANDLER_NOEC(__handler35, 35)
 TRAPHANDLER_NOEC(__handler36, 36)
 TRAPHANDLER_NOEC(__handler37, 37)
 TRAPHANDLER_NOEC(__handler38, 38)
 TRAPHANDLER_NOEC(__handler39, 39)
 TRAPHANDLER_NOEC(__handler40, 40)
 TRAPHANDLER_NOEC(__handler41, 41)
 TRAPHANDLER_NOEC(__handler42, 42)
 TRAPHANDLER_NOEC(__handler43, 43)
 TRAPHANDLER_NOEC(__handler44, 44)
 TRAPHANDLER_NOEC(__handler45, 45)
 TRAPHANDLER_NOEC(__handler46, 46)
 TRAPHANDLER_NOEC(__handler47, 47)
 TRAPHANDLER_NOEC(__handler48, 48)
 TRAPHANDLER_NOEC(__handler49, 49)
 TRAPHANDLER_NOEC(__handler50, 50)
 TRAPHANDLER_NOEC(__handler51, 51)
 TRAPHANDLER_NOEC(__handler52, 52)
 TRAPHANDLER_NOEC(__handler53, 53)
 TRAPHANDLER_NOEC(__handler54, 54)
 TRAPHANDLER_NOEC(__handler55, 55)
 TRAPHANDLER_NOEC(__handler56, 56)
 TRAPHANDLER_NOEC(__handler57, 57)
 TRAPHANDLER_NOEC(__handler58, 58)
 TRAPHANDLER_NOEC(__handler59, 59)
 TRAPHANDLER_NOEC(__handler60, 60)
 TRAPHANDLER_NOEC(__handler61, 61)
 TRAPHANDLER_NOEC(__handler62, 62)
 TRAPHANDLER_NOEC(__handler63, 63)
 TRAPHANDLER_NOEC(__handler64, 64)
 TRAPHANDLER_NOEC(__handler65, 65)
 TRAPHANDLER_NOEC(__handler66, 66)
 TRAPHANDLER_NOEC(__handler67, 67)
 TRAPHANDLER_NOEC(__handler68, 68)
 TRAPHANDLER_NOEC(__handler69, 69)
 TRAPHANDLER_NOEC(__handler70, 70)
 TRAPHANDLER_NOEC(__handler71, 71)
 TRAPHANDLER_NOEC(__handler72, 72)
 TRAPHANDLER_NOEC(__handler73, 73)
 TRAPHANDLER_NOEC(__handler74, 74)
 TRAPHANDLER_NOEC(__handler75, 75)
 TRAPHANDLER_NOEC(__handler76, 76)
 TRAPHANDLER_NOEC(__handler77, 77)
 TRAPHANDLER_NOEC(__handler78, 78)
 TRAPHANDLER_NOEC(__handler79, 79)
 TRAPHANDLER_NOEC(__handler80, 80)
 TRAPHANDLER_NOEC(__handler81, 81)
 TRAPHANDLER_NOEC(__handler82, 82)
 TRAPHANDLER_NOEC(__handler83, 83)
 TRAPHANDLER_NOEC(__handler84, 84)
 TRAPHANDLER_NOEC(__handler85, 85)
 TRAPHANDLER_NOEC(__handler86, 86)
 TRAPHANDLER_NOEC(__handler87, 87)
 TRAPHANDLER_NOEC(__handler88, 88)
 TRAPHANDLER_NOEC(__handler89, 89)
 TRAPHANDLER_NOEC(__handler90, 90)
 TRAPHANDLER_NOEC(__handler91, 91)
 TRAPHANDLER_NOEC(__handler92, 92)
 TRAPHANDLER_NOEC(__handler93, 93)
 TRAPHANDLER_NOEC(__handler94, 94)
 TRAPHANDLER_NOEC(__handler95, 95)
 TRAPHANDLER_NOEC(__handler96, 96)
 TRAPHANDLER_NOEC(__handler97, 97)
 TRAPHANDLER_NOEC(__handler98, 98)
 TRAPHANDLER_NOEC(__handler99, 99)
 TRAPHANDLER_NOEC(__handler100, 100)
 TRAPHANDLER_NOEC(__handler101, 101)
 TRAPHANDLER_NOEC(__handler102, 102)
 TRAPHANDLER_NOEC(__handler103, 103)
 TRAPHANDLER_NOEC(__handler104, 104)
 TRAPHANDLER_NOEC(__handler105, 105)
 TRAPHANDLER_NOEC(__handler106, 106)
 TRAPHANDLER_NOEC(__handler107, 107)
 TRAPHANDLER_NOEC(__handler108, 108)
 TRAPHANDLER_NOEC(__handler109, 109)
 TRAPHANDLER_NOEC(__handler110, 110)
 TRAPHANDLER_NOEC(__handler111, 111)
 TRAPHANDLER_NOEC(__handler112, 112)
 TRAPHANDLER_NOEC(__handler113, 113)
 TRAPHANDLER_NOEC(__handler114, 114)
 TRAPHANDLER_NOEC(__handler115, 115)
 TRAPHANDLER_NOEC(__handler116, 116)
 TRAPHANDLER_NOEC(__handler117, 117)
 TRAPHANDLER_NOEC(__handler118, 118)
 TRAPHANDLER_NOEC(__handler119, 119)
 TRAPHANDLER_NOEC(__handler120, 120)
 TRAPHANDLER_NOEC(__handler121, 121)
 TRAPHANDLER_NOEC(__handler122, 122)
 TRAPHANDLER_NOEC(__handler123, 123)
 TRAPHANDLER_NOEC(__handler124, 124)
 TRAPHANDLER_NOEC(__handler125, 125)
 TRAPHANDLER_NOEC(__handler126, 126)
 TRAPHANDLER_NOEC(__handler127, 127)
 TRAPHANDLER_NOEC(__handler128, 128)
 TRAPHANDLER_NOEC(__handler129, 129)
 TRAPHANDLER_NOEC(__handler130, 130)
 TRAPHANDLER_NOEC(__handler131, 131)
 TRAPHANDLER_NOEC(__handler132, 132)
 TRAPHANDLER_NOEC(__handler133, 133)
 TRAPHANDLER_NOEC(__handler134, 134)
 TRAPHANDLER_NOEC(__handler135, 135)
 TRAPHANDLER_NOEC(__handler136, 136)
 TRAPHANDLER_NOEC(__handler137, 137)
 TRAPHANDLER_NOEC(__handler138, 138)
 TRAPHANDLER_NOEC(__handler139, 139)
 TRAPHANDLER_NOEC(__handler140, 140)
 TRAPHANDLER_NOEC(__handler141, 141)
 TRAPHANDLER_NOEC(__handler142, 142)
 TRAPHANDLER_NOEC(__handler143, 143)
 TRAPHANDLER_NOEC(__handler144, 144)
 TRAPHANDLER_NOEC(__handler145, 145)
 TRAPHANDLER_NOEC(__handler146, 146)
 TRAPHANDLER_NOEC(__handler147, 147)
 TRAPHANDLER_NOEC(__handler148, 148)
 TRAPHANDLER_NOEC(__handler149, 149)
 TRAPHANDLER_NOEC(__handler150, 150)
 TRAPHANDLER_NOEC(__handler151, 151)
 TRAPHANDLER_NOEC(__handler152, 152)
 TRAPHANDLER_NOEC(__handler153, 153)
 TRAPHANDLER_NOEC(__handler154, 154)
 TRAPHANDLER_NOEC(__handler155, 155)
 TRAPHANDLER_NOEC(__handler156, 156)
 TRAPHANDLER_NOEC(__handler157, 157)
 TRAPHANDLER_NOEC(__handler158, 158)
 TRAPHANDLER_NOEC(__handler159, 159)
 TRAPHANDLER_NOEC(__handler160, 160)
 TRAPHANDLER_NOEC(__handler161, 161)
 TRAPHANDLER_NOEC(__handler162, 162)
 TRAPHANDLER_NOEC(__handler163, 163)
 TRAPHANDLER_NOEC(__handler164, 164)
 TRAPHANDLER_NOEC(__handler165, 165)
 TRAPHANDLER_NOEC(__handler166, 166)
 TRAPHANDLER_NOEC(__handler167, 167)
 TRAPHANDLER_NOEC(__handler168, 168)
 TRAPHANDLER_NOEC(__handler169, 169)
 TRAPHANDLER_NOEC(__handler170, 170)
 TRAPHANDLER_NOEC(__handler171, 171)
 TRAPHANDLER_NOEC(__handler172, 172)
 TRAPHANDLER_NOEC(__handler173, 173)
 TRAPHANDLER_NOEC(__handler174, 174)
 TRAPHANDLER_NOEC(__handler175, 175)
 TRAPHANDLER_NOEC(__handler176, 176)
 TRAPHANDLER_NOEC(__handler177, 177)
 TRAPHANDLER_NOEC(__handler178, 178)
 TRAPHANDLER_NOEC(__handler179, 179)
 TRAPHANDLER_NOEC(__handler180, 180)
 TRAPHANDLER_NOEC(__handler181, 181)
 TRAPHANDLER_NOEC(__handler182, 182)
 TRAPHANDLER_NOEC(__handler183, 183)
 TRAPHANDLER_NOEC(__handler184, 184)
 TRAPHANDLER_NOEC(__handler185, 185)
 TRAPHANDLER_NOEC(__handler186, 186)
 TRAPHANDLER_NOEC(__handler187, 187)
 TRAPHANDLER_NOEC(__handler188, 188)
 TRAPHANDLER_NOEC(__handler189, 189)
 TRAPHANDLER_NOEC(__handler190, 190)
 TRAPHANDLER_NOEC(__handler191, 191)
 TRAPHANDLER_NOEC(__handler192, 192)
 TRAPHANDLER_NOEC(__handler193, 193)
 TRAPHANDLER_NOEC(__handler194, 194)
 TRAPHANDLER_NOEC(__handler195, 195)
 TRAPHANDLER_NOEC(__handler196, 196)
 TRAPHANDLER_NOEC(__handler197, 197)
 TRAPHANDLER_NOEC(__handler198, 198)
 TRAPHANDLER_NOEC(__handler199, 199)
 TRAPHANDLER_NOEC(__handler200, 200)
 TRAPHANDLER_NOEC(__handler201, 201)
 TRAPHANDLER_NOEC(__handler202, 202)
 TRAPHANDLER_NOEC(__handler203, 203)
 TRAPHANDLER_NOEC(__handler204, 204)
 TRAPHANDLER_NOEC(__handler205, 205)
 TRAPHANDLER_NOEC(__handler206, 206)
 TRAPHANDLER_NOEC(__handler207, 207)
 TRAPHANDLER_NOEC(__handler208, 208)
 TRAPHANDLER_NOEC(__handler209, 209)
 TRAPHANDLER_NOEC(__handler210, 210)
 TRAPHANDLER_NOEC(__handler211, 211)
 TRAPHANDLER_NOEC(__handler212, 212)
 TRAPHANDLER_NOEC(__handler213, 213)
 TRAPHANDLER_NOEC(__handler214, 214)
 TRAPHANDLER_NOEC(__handler215, 215)
 TRAPHANDLER_NOEC(__handler216, 216)
 TRAPHANDLER_NOEC(__handler217, 217)
 TRAPHANDLER_NOEC(__handler218, 218)
 TRAPHANDLER_NOEC(__handler219, 219)
 TRAPHANDLER_NOEC(__handler220, 220)
 TRAPHANDLER_NOEC(__handler221, 221)
 TRAPHANDLER_NOEC(__handler222, 222)
 TRAPHANDLER_NOEC(__handler223, 223)
 TRAPHANDLER_NOEC(__handler224, 224)
 TRAPHANDLER_NOEC(__handler225, 225)
 TRAPHANDLER_NOEC(__handler226, 226)
 TRAPHANDLER_NOEC(__handler227, 227)
 TRAPHANDLER_NOEC(__handler228, 228)
 TRAPHANDLER_NOEC(__handler229, 229)
 TRAPHANDLER_NOEC(__handler230, 230)
 TRAPHANDLER_NOEC(__handler231, 231)
 TRAPHANDLER_NOEC(__handler232, 232)
 TRAPHANDLER_NOEC(__handler233, 233)
 TRAPHANDLER_NOEC(__handler234, 234)
 TRAPHANDLER_NOEC(__handler235, 235)
 TRAPHANDLER_NOEC(__handler236, 236)
 TRAPHANDLER_NOEC(__handler237, 237)
 TRAPHANDLER_NOEC(__handler238, 238)
 TRAPHANDLER_NOEC(__handler239, 239)
 TRAPHANDLER_NOEC(__handler240, 240)
 TRAPHANDLER_NOEC(__handler241, 241)
 TRAPHANDLER_NOEC(__handler242, 242)
 TRAPHANDLER_NOEC(__handler243, 243)
 TRAPHANDLER_NOEC(__handler244, 244)
 TRAPHANDLER_NOEC(__handler245, 245)
 TRAPHANDLER_NOEC(__handler246, 246)
 TRAPHANDLER_NOEC(__handler247, 247)
 TRAPHANDLER_NOEC(__handler248, 248)
 TRAPHANDLER_NOEC(__handler249, 249)
 TRAPHANDLER_NOEC(__handler250, 250)
 TRAPHANDLER_NOEC(__handler251, 251)
 TRAPHANDLER_NOEC(__handler252, 252)
 TRAPHANDLER_NOEC(__handler253, 253)
 TRAPHANDLER_NOEC(__handler254, 254)
 TRAPHANDLER_NOEC(__handler255, 255)

 .data
 .align 2
 .globl _handler_array
 _handler_array:
 .long __handler0
 .long __handler1
 .long __handler2
 .long __handler3
 .long __handler4
 .long __handler5
 .long __handler6
 .long __handler7
 .long __handler8
 .long __handler9
 .long __handler10
 .long __handler11
 .long __handler12
 .long __handler13
 .long __handler14
 .long __handler15
 .long __handler16
 .long __handler17
 .long __handler18
 .long __handler19
 .long __handler20
 .long __handler21
 .long __handler22
 .long __handler23
 .long __handler24
 .long __handler25
 .long __handler26
 .long __handler27
 .long __handler28
 .long __handler29
 .long __handler30
 .long __handler31
 .long __handler32
 .long __handler33
 .long __handler34
 .long __handler35
 .long __handler36
 .long __handler37
 .long __handler38
 .long __handler39
 .long __handler40
 .long __handler41
 .long __handler42
 .long __handler43
 .long __handler44
 .long __handler45
 .long __handler46
 .long __handler47
 .long __handler48
 .long __handler49
 .long __handler50
 .long __handler51
 .long __handler52
 .long __handler53
 .long __handler54
 .long __handler55
 .long __handler56
 .long __handler57
 .long __handler58
 .long __handler59
 .long __handler60
 .long __handler61
 .long __handler62
 .long __handler63
 .long __handler64
 .long __handler65
 .long __handler66
 .long __handler67
 .long __handler68
 .long __handler69
 .long __handler70
 .long __handler71
 .long __handler72
 .long __handler73
 .long __handler74
 .long __handler75
 .long __handler76
 .long __handler77
 .long __handler78
 .long __handler79
 .long __handler80
 .long __handler81
 .long __handler82
 .long __handler83
 .long __handler84
 .long __handler85
 .long __handler86
 .long __handler87
 .long __handler88
 .long __handler89
 .long __handler90
 .long __handler91
 .long __handler92
 .long __handler93
 .long __handler94
 .long __handler95
 .long __handler96
 .long __handler97
 .long __handler98
 .long __handler99
 .long __handler100
 .long __handler101
 .long __handler102
 .long __handler103
 .long __handler104
 .long __handler105
 .long __handler106
 .long __handler107
 .long __handler108
 .long __handler109
 .long __handler110
 .long __handler111
 .long __handler112
 .long __handler113
 .long __handler114
 .long __handler115
 .long __handler116
 .long __handler117
 .long __handler118
 .long __handler119
 .long __handler120
 .long __handler121
 .long __handler122
 .long __handler123
 .long __handler124
 .long __handler125
 .long __handler126
 .long __handler127
 .long __handler128
 .long __handler129
 .long __handler130
 .long __handler131
 .long __handler132
 .long __handler133
 .long __handler134
 .long __handler135
 .long __handler136
 .long __handler137
 .long __handler138
 .long __handler139
 .long __handler140
 .long __handler141
 .long __handler142
 .long __handler143
 .long __handler144
 .long __handler145
 .long __handler146
 .long __handler147
 .long __handler148
 .long __handler149
 .long __handler150
 .long __handler151
 .long __handler152
 .long __handler153
 .long __handler154
 .long __handler155
 .long __handler156
 .long __handler157
 .long __handler158
 .long __handler159
 .long __handler160
 .long __handler161
 .long __handler162
 .long __handler163
 .long __handler164
 .long __handler165
 .long __handler166
 .long __handler167
 .long __handler168
 .long __handler169
 .long __handler170
 .long __handler171
 .long __handler172
 .long __handler173
 .long __handler174
 .long __handler175
 .long __handler176
 .long __handler177
 .long __handler178
 .long __handler179
 .long __handler180
 .long __handler181
 .long __handler182
 .long __handler183
 .long __handler184
 .long __handler185
 .long __handler186
 .long __handler187
 .long __handler188
 .long __handler189
 .long __handler190
 .long __handler191
 .long __handler192
 .long __handler193
 .long __handler194
 .long __handler195
 .long __handler196
 .long __handler197
 .long __handler198
 .long __handler199
 .long __handler200
 .long __handler201
 .long __handler202
 .long __handler203
 .long __handler204
 .long __handler205
 .long __handler206
 .long __handler207
 .long __handler208
 .long __handler209
 .long __handler210
 .long __handler211
 .long __handler212
 .long __handler213
 .long __handler214
 .long __handler215
 .long __handler216
 .long __handler217
 .long __handler218
 .long __handler219
 .long __handler220
 .long __handler221
 .long __handler222
 .long __handler223
 .long __handler224
 .long __handler225
 .long __handler226
 .long __handler227
 .long __handler228
 .long __handler229
 .long __handler230
 .long __handler231
 .long __handler232
 .long __handler233
 .long __handler234
 .long __handler235
 .long __handler236
 .long __handler237
 .long __handler238
 .long __handler239
 .long __handler240
 .long __handler241
 .long __handler242
 .long __handler243
 .long __handler244
 .long __handler245
 .long __handler246
 .long __handler247
 .long __handler248
 .long __handler249
 .long __handler250
 .long __handler251
 .long __handler252
 .long __handler253
 .long __handler254
 .long __handler255

#endif

/*
 * Lab 3: Your code here for _alltraps
 */

 .text
 .align 2
_alltraps:
	push	%ds
	push	%es
	pushal
	movw	$GD_KD, %ax
	movw	%ax, %ds
	movw	%ax, %es
	pushl	%esp
	call	trap