/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_CONSTANTS_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_CONSTANTS_H
namespace panda::ecmascript::aarch64 {
enum RegisterId : uint8_t {
    X0, X1, X2, X3, X4, X5, X6, X7,
    X8, X9, X10, X11, X12, X13, X14, X15,
    X16, X17, X18, X19, X20, X21, X22, X23,
    X24, X25, X26, X27, X28, X29, X30, SP,
    Zero = SP,
    FP = X29,
    INVALID_REG = 0xFF,
};

enum RegisterType {
    W = 0,
    X = 1,
};

static const int RegXSize = 64;
static const int RegWSize = 32;

enum VectorRegisterId : uint8_t {
    v0, v1, v2, v3, v4, v5, v6, v7,
    v8, v9, v10, v11, v12, v13, v14, v15,
    v16, v17, v18, v19, v20, v21, v22, v23,
    v24, v25, v26, v27, v28, v29, v30, v31,
    INVALID_VREG = 0xFF,
};

enum Extend : uint8_t {
    NO_EXTEND = 0xFF,
    UXTB = 0,   /* zero extend to byte */
    UXTH = 1,   /* zero extend to half word */
    UXTW = 2,   /* zero extend to word */
    UXTX = 3,   /* zero extend to 64bit */
    SXTB = 4,   /* sign extend to byte */
    SXTH = 5,   /* sign extend to half word */
    SXTW = 6,   /* sign extend to word */
    SXTX = 7,   /* sign extend to 64bit */
};

enum Shift : uint8_t {
    NO_SHIFT = 0xFF,
    LSL = 0x0,
    LSR = 0x1,
    ASR = 0x2,
    ROR = 0x3,
    MSL = 0x4,
};

enum Scale {
    B = 0,
    H = 1,
    S = 2,
    D = 3,
    Q = 4,
};

enum Condition {
    EQ = 0,
    NE = 1,
    HS = 2,
    CS = HS,
    LO = 3,
    CC = LO,
    MI = 4,
    PL = 5,
    VS = 6,
    VC = 7,
    HI = 8,
    LS = 9,
    GE = 10,
    LT = 11,
    GT = 12,
    LE = 13,
    AL = 14,
    NV = 15,
};

enum MoveOpCode {
    MOVN = 0x12800000,
    MOVZ = 0X52800000,
    MOVK = 0x72800000,
};

enum AddSubOpCode {
    ADD_Imm     = 0x11000000,
    ADD_Shift   = 0x0b000000,
    ADD_Extend  = 0x0b200000,
    SUB_Extend  = 0x4b200000,
    SUB_Imm     = 0x51000000,
    SUB_Shift   = 0x4b000000,
};

enum BitwiseOpCode {
    AND_Imm      = 0x12000000,
    AND_Shift    = 0x0a000000,
    ANDS_Imm     = 0x72000000,
    ANDS_Shift   = 0x6a000000,
    ORR_Imm      = 0x32000000,
    ORR_Shift    = 0x2a000000,
};

// branch code
enum BranchOpCode {
    BranchFMask = 0x7C000000,
    BranchCondFMask = 0xFE000000,
    BranchCompareFMask = 0x7E000000,
    BranchTestFMask = 0x7E000000,
    Branch      = 0x14000000,
    BranchCond  = 0x54000000,
    BCCond      = 0x54000010,
    BR          = 0xd61f0000,
    CBNZ        = 0x35000000,
    CBZ         = 0x34000000,
    TBZ         = 0x36000000,
    TBNZ        = 0x37000000,
};

// brk code
enum BrkOpCode {
    BRKImm      = 0xd4200000,
};

// call code
enum CallOpCode {
    BL  = 0x94000000,
    BLR = 0xd63f0000,
};

// compare code
enum CompareCode {
    CMP_Extend  = 0x6d20000f,
    CMP_Imm     = 0x7100000f,
    CMP_Shift   = 0x6d00000f,
    CSEL        = 0x1a800000,
    CSET        = 0x1a9f07e0,
};

// memory code
enum LoadStorePairOpCode {
    LDP_Post     = 0x28c00000,
    LDP_Pre      = 0x29c00000,
    LDP_Offset   = 0x29400000,
    LDP_V_Post   = 0x2cc00000,
    LDP_V_Pre    = 0x2dc00000,
    LDP_V_Offset = 0x2d400000,
    STP_Post     = 0x28800000,
    STP_Pre      = 0x29800000,
    STP_Offset   = 0x29000000,
    STP_V_Post   = 0x2c800000,
    STP_V_Pre    = 0x2d800000,
    STP_V_Offset = 0x2d00000,
};

enum LoadStoreOpCode {
    LDR_Post     = 0xb8400400,
    LDR_Pre      = 0xb8400c00,
    LDR_Offset   = 0xb9400000,
    LDRB_Post    = 0x38400400,
    LDRB_Pre     = 0x38400c00,
    LDRB_Offset  = 0x39400000,
    LDRH_Post    = 0x78400400,
    LDRH_Pre     = 0x78400c00,
    LDRH_Offset  = 0x79400000,
    STR_Post     = 0xb8000400,
    STR_Pre      = 0xb8000c00,
    STR_Offset   = 0xb9000000,
    LDR_Register = 0xb8600800,
    LDRB_Register = 0x38600800,
    LDRH_Register = 0x78600800,
    LDUR_Offset   = 0xb8400000,
    STUR_Offset   = 0xb8000000,
};

enum  AddrMode {
    OFFSET,
    PREINDEX,
    POSTINDEX
};

enum LogicShiftOpCode {
    LSL_Reg = 0x1AC02000,
    LSR_Reg = 0x1AC02400,
    UBFM    = 0x53000000,
};

enum NopOpCode {
    Nop = 0xd503201f,
};

enum RetOpCode {
    Ret = 0xd65f0000,
};

#define COMMON_REGISTER_FIELD_LIST(V)   \
    V(COMMON_REG, Rd, 4, 0)             \
    V(COMMON_REG, Rn, 9, 5)             \
    V(COMMON_REG, Rm, 20, 16)           \
    V(COMMON_REG, Rt, 4, 0)             \
    V(COMMON_REG, Rt2, 14, 10)          \
    V(COMMON_REG, Sf, 31, 31)

/* Aarch64 Instruction MOVZ Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |        |15 14     |11 10 9  |      5 4|       0|
    |sf| 1 0 | 1  0 0  1  0  1|  hw |                    imm16                 |    Rd    |
   Aarch64 Instruction MOVN Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |        |15 14     |11 10 9  |      5 4|       0|
    |sf| 0 0 | 1  0 0  1  0  1|  hw |                    imm16                 |    Rd    |
   Aarch64 Instruction MOVK Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |        |15 14     |11 10 9  |      5 4|       0|
    |sf| 1 1 | 1  0 0  1  0  1|  hw |                    imm16                 |    Rd    |
*/
#define MOV_WIDE_FIELD_LIST(V)   \
    V(MOV_WIDE, Imm16, 20, 5)    \
    V(MOV_WIDE, Hw, 22, 21)

/* Aarch64 Instruction AddImm Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |        |15 14     |11 10 9  |      5 4|       0|
    |sf| 1 S | 1  0  0  0  0  1|sh|             imm12              |    Rn     |    Rd    |
   Aarch64 Instruction AddShift Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |      16|15 14     |11 10 9  |      5 4|       0|
    |sf| 0 S | 0  1  0  1  1|shift| 0|    rm      |     imm6       |    Rn     |    Rd    |
   Aarch64 Instruction AddExtend Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |      16|15     13 12 |11 10 9  |      5 4|       0|
    |sf| 0 S | 0  1  0  1  1|0  0 | 1|    rm      | option  |  imm3   |    Rn     |    Rd    |
*/
#define ADD_SUB_FIELD_LIST(V)           \
    V(ADD_SUB, S, 29, 29)               \
    V(ADD_SUB, Sh, 22, 22)              \
    V(ADD_SUB, Imm12, 21, 10)           \
    V(ADD_SUB, Shift, 23, 22)           \
    V(ADD_SUB, ShiftAmount, 15, 10)     \
    V(ADD_SUB, ExtendOption, 15, 13)    \
    V(ADD_SUB, ExtendShift, 12, 10)

/*
   Aarch64 Instruction OrrImm Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |       16|15 14     |11 10 9  |      5 4|       0|
    |sf| 0 1 | 1  0  0  1  0  0|N|       immr      |      imms      |    Rn     |    Rd    |
   Aarch64 Instruction ORRShift Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |      16|15 14     |11 10 9  |      5 4|       0|
    |sf| 0 1 | 0  1  0  1  0|shift| 0|    rm      |     imm6       |    Rn     |    Rd    |
*/
#define BITWISE_OP_FIELD_LIST(V)            \
    V(BITWISE_OP, N, 22, 22)                \
    V(BITWISE_OP, Immr, 21, 16)             \
    V(BITWISE_OP, Shift, 23, 22)            \
    V(BITWISE_OP, Imms, 15, 10)             \
    V(BITWISE_OP, ShiftAmount, 15, 10)

/*
   Aarch64 Instruction CMP Instruction is aliase of Subs
   Aarch64 Instruction CSEL Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20 |       16|15 14   12 |11 10 9  |      5 4|       0|
    |sf| 0  0| 1  1  0  1  0  1 0  0 |     rm      |    cond   | 0  0|    Rn     |    Rd    |
   Aarch64 Instruction CSET Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20|        16|15 14     |11 10 9  |    5 4|       0|
    |sf| 0  0| 1  1  0  1  0|1  0  0| 1  1  1  1  1|   cond   | 0 1| 1 1 1 1 1|    Rd    |
*/
#define COMPARE_OP_FIELD_LIST(V)   \
    V(CSEL, Cond, 15, 12)          \

/* Aarch64 Instruction LDR Field Defines
    |31 30 29 28|27 26 25 24|23 22 21 20|        |15      12|11 10 9  |      5 4|       0|
    |1  x | 1  0 1 |0 |0  0 | 0 1 | 0|          imm9        | 0 1  |    Rn    |     Rt   |
*/
#define LDR_AND_STR_FIELD_LIST(V)   \
    V(LDR_STR, Size, 31, 30)        \
    V(LDR_STR, Opc, 23, 22)         \
    V(LDR_STR, Imm9, 20, 12)        \
    V(LDR_STR, Imm12, 21, 10)       \
    V(LDR_STR, Extend, 15, 13)      \
    V(LDR_STR, S, 12, 12)


/* Aarch64 Instruction LDP Field Defines
    |31 30 29 28|27 26 25 24|23 22 21   |        |15 14     |11 10 9  |      5 4|       0|
    |x  0 | 1  0 1 |0 |0  0  1| 1|      imm7        |    Rt2      |    Rn     |     Rt   |
*/
#define LDP_AND_STP_FIELD_LIST(V)   \
    V(LDP_STP, Opc, 31, 30)         \
    V(LDP_STP, Imm7, 21, 15)


/* Aarch64 Instruction B Field Defines
    |31 30 29 28|27 26 25 24|23 22 21   |        |15 14     |11 10 9  |      5 4|       0|
    |x  0 | 1  0 1 |0 |0  0  1| 1|      imm7        |    Rt2      |    Rn     |     Rt   |
*/

#define BRANCH_FIELD_LIST(V)        \
    V(BRANCH, Imm26, 25, 0)         \
    V(BRANCH, Imm19, 23, 5)         \
    V(BRANCH, Imm14, 18, 5)         \
    V(BRANCH, B5, 31, 31)           \
    V(BRANCH, B40, 23, 19)

/* Aarch64 Instruction BRK Field Defines
    |31 30 29 28|27 26 25 24|23 22 21   |        |15 14     |11 10 9  |      5 4|       0|
    |1  1 | 0  1  0  0  0  0| 0  0  1|                 imm16                  |0 0 0 0  0|
*/
#define BRK_FIELD_LIST(V)        \
    V(BRK, Imm16, 20, 5)

#define DECL_FIELDS_IN_INSTRUCTION(INSTNAME, FIELD_NAME, HIGHBITS, LOWBITS) \
static const uint32_t INSTNAME##_##FIELD_NAME##_HIGHBITS = HIGHBITS;  \
static const uint32_t INSTNAME##_##FIELD_NAME##_LOWBITS = LOWBITS;    \
static const uint32_t INSTNAME##_##FIELD_NAME##_WIDTH = ((HIGHBITS - LOWBITS) + 1); \
static const uint32_t INSTNAME##_##FIELD_NAME##_MASK = (((1 << INSTNAME##_##FIELD_NAME##_WIDTH) - 1) << LOWBITS);

#define DECL_INSTRUCTION_FIELDS(V)  \
    COMMON_REGISTER_FIELD_LIST(V)   \
    LDP_AND_STP_FIELD_LIST(V)       \
    LDR_AND_STR_FIELD_LIST(V)       \
    MOV_WIDE_FIELD_LIST(V)          \
    BITWISE_OP_FIELD_LIST(V)        \
    ADD_SUB_FIELD_LIST(V)           \
    COMPARE_OP_FIELD_LIST(V)        \
    BRANCH_FIELD_LIST(V)            \
    BRK_FIELD_LIST(V)

DECL_INSTRUCTION_FIELDS(DECL_FIELDS_IN_INSTRUCTION)
#undef DECL_INSTRUCTION_FIELDS
};  // namespace panda::ecmascript::aarch64
#endif