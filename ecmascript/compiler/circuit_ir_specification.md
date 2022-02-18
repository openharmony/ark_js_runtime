# Circuit IR Specification

## General Design

Circuit IR is like a circuit diagram, which is good at representing the intrinsic logic of a computation process. The circuit diagram is a directed graph: [logic gates](https://en.wikipedia.org/wiki/Logic_gate) are nodes, wires are directed edges. For every gate, inputs are ordered, outputs are unordered. Circuit IR describes semantic of programs in a language-neutral and target-neutral way, targeting multi-language and multi-target support. The design of Circuit IR has a great emphasis on compilation speed (for JIT), code optimization (on both high and low levels), and canonicalization (unique representation for same intrinsic logic).

Circuit IR splits a program into two major parts: [sequential logic](https://en.wikipedia.org/wiki/Sequential_logic) part and [combinational logic](https://en.wikipedia.org/wiki/Combinational_logic) part:

* The **sequential logic** part is a subgraph of Circuit IR which is similar to the underlying control flow graph (CFG) of the program, and gates in this part are named **state gates** (since they acted like a [finite state machine](https://en.wikipedia.org/wiki/Finite-state_machine) (FSM)). Wires that connect two state gates represent possible state transitions of the FSM, and they are named **state wires**. Note that every gates that have output state wires are state gates.

* The **combinational logic** part is the other subgraph of Circuit IR which represents all computations in a program using a directed acyclic graph (DAG), and gates in this part are named **computation gates**. A computation gate can do simple things such as adding two integer values, or complex things such as calling a function (and thus make a change to the global memory). Most of **computation gates** will take some values as input and output a value. These values can be transferred by **data wires**. Some computation gates will load from or store to the global memory, so they should be executed non-simultaneously and in some order that will not violate memory dependencies (such as RAW, WAR and WAW). Addressing this issue, **dependency wires** are introduced to constrain the possible execution order of such computations. When a computation gate has multiple dependencies, an auxiliary gate `DEPEND_AND` is used to merge dependencies. Note that dependency wires are treated like data wires during the scheduling phase, since dependency wires can be viewed as special data wires that transfer huge values representing the whole memory.

In traditional [SSA](https://en.wikipedia.org/wiki/Static_single_assignment_form) form IR (e.g. LLVM IR), each instruction is placed inside a node of CFG (basic block), and all instructions in a basic block are [linearly ordered](https://en.wikipedia.org/wiki/Total_order). However, in Circuit IR, computation gates are not tied to state gates, and they are [partially ordered](https://en.wikipedia.org/wiki/Partially_ordered_set) by wires. Sequential logic part and combinational logic part are loosely coupled, they only interact in three ways:

* State gates that have multiple transitions (corresponding to multiple control flow branches) such as `IF_BRANCH` and `SWITCH_BRANCH` will need an input value computed from combinational logic part to select which next state the FSM will transit to.

* Similar to the Φ functions in traditional SSA form IR, there are **[selector](https://en.wikipedia.org/wiki/Multiplexer) gates** such as `VALUE_SELECTOR` and `DEPEND_SELECTOR` that select a data or dependency path based on the state transition action of the FSM. Selector gates are affiliated (many-to-one) to `MERGE` state gates (which have several transition sources), and take several values or dependencies from computation part as input, and will select a value or dependency from input as output, based on which state transition action is taken.

* In some cases, a computation gate with side effect (i.e. may store to the global memory) should not be executed before a specific branch state transition action is taken. Addressing this problem, **[relay](https://en.wikipedia.org/wiki/Relay) gates** `DEPEND_RELAY` are introduced. They take a state gate (the target of action) as input, and reinterpret it as a kind of memory dependency that can be used by computation gates with side effect.

Loose coupling of sequential logic part and combinational logic part can benefit compilation speed, code optimization. Firstly, most of IR passes can focus on only one part and will not be interfered by the other part, so the implementation will be simpler. In addition, some special and important code optimizations such as (register pressure sensitive) loop invariant code motion and code sink can be done in the IR scheduling phrase (without furthermore IR analysis or modification), thus they will not couple with other code optimizations and can be done perfectly. Last but not least, less coupling means more canonicalization, this implies fewer branches in the implementation of IR optimization algorithms.

There are several nodes named **root nodes** in Circuit IR. They are not called "gates" since they do not contribute to the logic of circuit, but they have the same data structure as gates. Root nodes are representing starting/ending vertices of the IR graph, or registering lists of starting/ending vertices of the IR graph. IR Passes usually traverse a part of all gates (forwardly or reversely) starting from root nodes. Explanations of root nodes are listed below:

* `CIRCUIT_ROOT`: A very special gate located at zero offset. It is the only gate that does not have any inputs. Every root nodes listed below take this gate as theirs only input.

* `STATE_ENTRY`: Representing the initial state of the sequential logic part. (for traversing forwardly from beginning to ending)

* `DEPEND_ENTRY`: The origin of dependency flows of the entire circuit.

* `RETURN_LIST`: Registering all terminal states of the function. (for traversing reversely from ending to beginning)
  
* `CONSTANT_LIST` `ARG_LIST`: Registering all value origins such as constants and arguments. (they are special computation gates that do not depend on other values)

The offsets of root nodes are fixed, so they can be accessed instantly via `GateRef Circuit::GetCircuitRoot(OpCode opcode)`.

## Type system

### Levels of types 

There are two levels of types of values in Circuit IR:

* Primary types: This level of types are **low level** (closer to target architectures) and **fundamental** for Circuit IR, and are determined by the opcode of gates. They describe the bit width of values, and which type of registers (integer or float) should such values be put into. Circuit IR can be translated to correct machine code with only primary types. All primary types are `NOVALUE` `I1` `I8` `I16` `INT32` `INT64` `F32` `F64`. Note that pointer type is not included in primary types, since pointers are not different from integers in common ISAs. The concept of pointers should be expressed in secondary types.

* Secondary types: This level of types are **high level** (closer to languages) and **optional**. They can provide information for program analysis, code optimization and garbage collection (generating stack maps). Secondary types can represent categories of possible bit vectors the value can be (e.g. `JS_ANY` `JS_BOOLEAN` `JS_NULL` `JS_UNDEFINED` `JS_NUMBER` `JS_SMI` etc. builtin categories and user defined bit vectors categories), and possible classes of objects the value points to as a pointer (e.g. `JS_HEAP_OBJECT` `JS_STRING` `JS_OBJECT` `JS_ARRAY` `JS_TYPED_ARRAY` `JS_UINT8_ARRAY` etc. builtin classes and user defined classes).

Validation rules of primary type are already builtin for Circuit IR. Validation rules of secondary types need to be additionally provided.

### Layout of type representation bits (GateType)

GateType is represented by 64 bits:

* The 31st bit is used to distinguish between MIR type and TS type, `0` means TS type, `1` means MIR type.

* The 30th and 29th bits are used to indicate whether the output values of the MIR gates are GC related (when within the category of tagged values) as follows:
    * `00` means GC may or may not occur (within the `GC` or `NOGC` tagged value category)
    * `01` means GC will occur (within the `GC` tagged value category)
    * `10` means GC will not occur (within the `NOGC` tagged value category)
    * `11` means not within the category of tagged values (within the C++ world category)

* In the case of MIR type, the 1st bit in GateType is used to indicate whether there are output values. When MachineType is `NOVALUE`, GateType is always `EMPTY`.

### Type inference

Primary types are all set during construction of Circuit IR, so no furthermore type inference is required. Circuit IR verifier will verify consistency of primary types. Secondary types can be not precisely set during construction (e.g. leaving many intermediate values as `JS_ANY`), and the compiler should be able to do type inference and check type consistency following data flow paths (e.g. `c:<JS_ANY>=JS_ADD(a:<JS_NUMBER>, b:<JS_STRING>) -> c:<JS_STRING>=JS_ADD(a:<JS_NUMBER>, b:<JS_STRING>)` and `c:<JS_ANY>=VALUE_SELECTOR(a:<JS_STRING>, b:<JS_ARRAY>) -> c:<JS_HEAP_OBJECT>=JS_ADD(a:<JS_STRING>, b:<JS_ARRAY>)`), thus the secondary types will be more precise than initially set at the end.

## Instructions

There are three levels of instructions in Circuit IR: high-level instructions (HIR), middle-level instructions (MIR) and low-level instructions (LIR). They have the same underlying basic structure, so they can co-occur in the same Circuit IR graphs, and the semantics are still consistent. Instructions are not necessarily computational gates. Some instructions may throw an exception and lead to state transition, so they are state gates doing computations. There are also some "pure" state gates that do not do computation, they are compatible with all levels of instructions.

* HIR instructions represent language-related computational process, which usually correspond to the bytecodes of specific languages.
* MIR instructions are language-independent and target-independent, which are similar to LLVM IR, but having a completely different type system. The type system is very lightweight, designed to better support fast compilation and garbage collection.
* LIR instructions are target-related, which usually correspond to the machine instructions of target architecture.

### Notations

* `ANY_STATE` means any state gates.
* `GENERAL_STATE` means any one of `IF_TRUE`, `IF_FALSE`, `IF_SUCCESS`, `IF_EXCEPTION`,`SWITCH_CASE`, `DEFAULT_CASE`, `MERGE`, `LOOP_BEGIN`, `STATE_ENTRY`, `ORDINARY_BLOCK` gates.
* `ANYVALUE` means any one of `I1` `I8` `I16` `I32` `INT64` `F32` `F64` `ARCH`.
* `ANYINT` means any one of `I1` `I8` `I16` `I32` `I64` `ARCH`.
* `ANYFLOAT` means any one of `F32` `F64`.
* `ARCH` means architecture-related integer type (`INTx` on `x` bits architecture).
* `FLEX` means flexible primary type of output value (could be `ARCH`).
* <<...>> means occurring any times (maybe zero)

### Pure state gates

#### RETURN

Return a value from a non-void function.

|        | state wires       | #dependency wires  | data wires   |
|--------|-------------------|--------------------|--------------|
| input  | [`GENERAL_STATE`] | 1                  | [`ANYVALUE`] |
| output | {}                | 0                  | {}           |

| Root          | Bitfield |
|---------------|----------|
| `RETURN_LIST` | not used |

#### THROW

Throw an exception value out of function scope.

|        | state wires       | #dependency wires | data wires   |
|--------|-------------------|-------------------|--------------|
| input  | [`GENERAL_STATE`] | 1                 | [`ANYVALUE`] |
| output | {}                | 0                 | {}           |

| Root         | Bitfield |
|--------------|----------|
| `THROW_LIST` | not used |

#### ORDINARY_BLOCK

An ordinary state. Usually used as a placeholder. It will be eliminated by IR optimization.

|        | state wires                       | #dependency wires | data wires |
|--------|-----------------------------------|-------------------|------------|
| input  | [`GENERAL_STATE`]                 | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_RELAY`>>} | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### IF_BRANCH

This state has two possible transitions (branches) based on its input value.

|        | state wires             | #dependency wires | data wires |
|--------|-------------------------|-------------------|------------|
| input  | [`GENERAL_STATE`]       | 0                 | [`I1`]   |
| output | {`IF_TRUE`, `IF_FALSE`} | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### SWITCH_BRANCH

This state has multiple possible transitions (branches) based on its input value.

|        | state wires                         | #dependency wires | data wires   |
|--------|-------------------------------------|-------------------|--------------|
| input  | [`GENERAL_STATE`]                   | 0                 | [`ANYVALUE`] |
| output | {<<`SWITCH_CASE`>>, `DEFAULT_CASE`} | 0                 | {}           |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### IF_TRUE | IF_FALSE

Successor states of `IF_BRANCH`.

|        | state wires                       | #dependency wires | data wires |
|--------|-----------------------------------|-------------------|------------|
| input  | [`IF_BRANCH`]                     | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_RELAY`>>} | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### IF_SUCCESS | IF_EXCEPTION

Successor states of instructions (usually HIR) that may throw an exception.

|        | state wires                       | #dependency wires | data wires |
|--------|-----------------------------------|-------------------|------------|
| input  | [`ANY_STATE`]                     | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_RELAY`>>} | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### SWITCH_CASE | DEFAULT_CASE

Successor state of `SWITCH_BRANCH`.

|        | state wires                       | #dependency wires | data wires |
|--------|-----------------------------------|-------------------|------------|
| input  | [`SWITCH_BRANCH`]                 | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_RELAY`>>} | 0                 | {}         |

| Root      | Bitfield                                                               |
|-----------|------------------------------------------------------------------------|
| not used  | represent case value for `SWITCH_CASE` and not used for `DEFAULT_CASE` |

#### MERGE

State that have multiple possible predicate states.

|        | state wires                                                | #dependency wires | data wires |
|--------|------------------------------------------------------------|-------------------|------------|
| input  | [<<`GENERAL_STATE`>>(N)]                                   | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_SELECTOR`>>, <<`VALUE_SELECTOR`>>} | 0                 | {}         |

| Root      | Bitfield                   |
|-----------|----------------------------|
| not used  | number of state inputs (N) |

#### LOOP_BEGIN

Represents loop begin.

|        | state wires                                                | #dependency wires | data wires |
|--------|------------------------------------------------------------|-------------------|------------|
| input  | [`GENERAL_STATE`, `LOOP_BACK`]                             | 0                 | []         |
| output | {`ANY_STATE`, <<`DEPEND_SELECTOR`>>, <<`VALUE_SELECTOR`>>} | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### LOOP_BACK

Represents loop back.

|        | state wires       | #dependency wires | data wires |
|--------|-------------------|-------------------|------------|
| input  | [`GENERAL_STATE`] | 0                 | []         |
| output | {`LOOP_BEGIN`}    | 0                 | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

### Intermediate gates and auxiliary gates

Intermediate gates are gates that connect sequential logic and combinational logic parts (relays and selectors). Currently, there is only one type of auxiliary gates, which is used to represent multiple memory dependencies.

#### VALUE_SELECTOR

Represents value selector.

|        | state wires               | #dependency wires | data wires      |
|--------|---------------------------|-------------------|-----------------|
| input  | [`MERGE` or `LOOP_BEGIN`] | 0                 | [<<`FLEX`>>(N)] |
| output | {}                        | 0                 | {<<`FLEX`>>}    |

| Root     | Bitfield                  |
|----------|---------------------------|
| not used | number of data inputs (N) |

#### DEPEND_SELECTOR

Represents dependency selector.

|        | state wires               | #dependency wires | data wires |
|--------|---------------------------|-------------------|------------|
| input  | [`MERGE` or `LOOP_BEGIN`] | N                 | []         |
| output | {}                        | any               | {}         |

| Root     | Bitfield                        |
|----------|---------------------------------|
| not used | number of dependency inputs (N) |

#### DEPEND_RELAY

Represents dependency relay.

|        | state wires                                                                                                        | #dependency wires | data wires |
|--------|--------------------------------------------------------------------------------------------------------------------|-------------------|------------|
| input  | [`IF_TRUE` or `IF_FALSE` or `IF_SUCCESS` or `IF_EXCEPTION` or `SWITCH_CASE` or `DEFAULT_CASE` or `ORDINARY_BLOCK`] | 1                 | []         |
| output | {}                                                                                                                 | any               | {}         |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### DEPEND_AND

Represents dependency and operator.

|        | state wires | #dependency wires | data wires |
|--------|-------------|-------------------|------------|
| input  | []          | N                 | []         |
| output | {}          | any               | {}         |

| Root     | Bitfield                        |
|----------|---------------------------------|
| not used | number of dependency inputs (N) |

### HIR instructions

#### JS_BYTECODE

Represents JavaScript bytecode.

|        | state wires                    | #dependency wires | data wires       |
|--------|--------------------------------|-------------------|------------------|
| input  | [`GENERAL_STATE`]              | 1                 | [<<`I64`>>(N)] |
| output | {`IF_SUCCESS`, `IF_EXCEPTION`} | any               | {<<`I64`>>}    |

| Root     | Bitfield                   |
|----------|----------------------------|
| not used | number of value inputs (N) |

### MIR instructions

#### Special instructions

##### CALL

Represents a simple function call (would not get an exception).

|        | state wires | #dependency wires | data wires                  |
|--------|-------------|-------------------|-----------------------------|
| input  | []          | 1                 | [`ARCH`, <<`ANYVALUE`>>(N)] |
| output | {}          | any               | {<<`FLEX`>>}                |

| Root      | Bitfield                    |
|-----------|-----------------------------|
| not used  | number of function args (N) |

#### Floating point unary arithmetic operations

* **FNEG**: returns the negation of its floating-point operand.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 0                 | [`FLEX`]     |
| output | {}          | 0                 | {<<`FLEX`>>} |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### Integer binary arithmetic operations

* **ADD**: returns the sum of its two integer operands.
* **SUB**: returns the difference of its two integer operands. It is used to implement the "-" unary operation.
* **MUL**: returns the product of its two integer operands.
* **EXP**: returns the first integer operand raised to the power of the second integer operand.
* **SDIV**: returns the signed quotient of its two integer operands.
* **SREM**: returns the remainder from the signed division of its two integer operands.
* **UDIV**: returns the unsigned quotient of its two integer operands.
* **UREM**: returns the remainder from the unsigned division of its two integer operands.
* **AND**: returns the bitwise logical and of its two operands.
* **XOR**: returns the bitwise logical exclusive or of its two operands. It is used to implement the "~" unary operation.
* **OR**: returns the bitwise logical inclusive or of its two operands.
* **SHL**: returns the first integer operand shifted to the left a specified number of bits.
* **LSHR**: returns the first integer operand shifted to the right a specified number of bits with zero fill.
* **ASHR**: returns the first integer operand shifted to the right a specified number of bits with sign extension.

|        | state wires | #dependency wires | data wires       |
|--------|-------------|-------------------|------------------|
| input  | []          | 0                 | [`FLEX`, `FLEX`] |
| output | {}          | 0                 | {<<`FLEX`>>}     |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### Floating-point binary arithmetic operations

* **FADD**: returns the sum of its two floating-point operands.
* **FSUB**: returns the difference of its two floating-point operands.
* **FMUL**: returns the product of its two floating-point operands.
* **FEXP**: returns the first floating-point operand raised to the power of the second floating-point operand.
* **FDIV**: returns the quotient of its two floating-point operands.
* **FMOD**: returns the remainder from the division of its two floating-point operands.

|        | state wires | #dependency wires | data wires       |
|--------|-------------|-------------------|------------------|
| input  | []          | 0                 | [`FLEX`, `FLEX`] |
| output | {}          | 0                 | {<<`FLEX`>>}     |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### Integer binary compare operations

* **ICMP**: returns a boolean (`I1`) value based on comparison of its two integer operands. Condition code indicating the kind of comparison to perform is stored in the bitfield. The possible condition codes are:
  * `0001` `EQ`: yields true if the operands are equal, false otherwise. No sign interpretation is necessary or performed.
  * `0010` `UGT`: interprets the operands as unsigned values and yields true if op1 is greater than op2.
  * `0011` `UGE`: interprets the operands as unsigned values and yields true if op1 is greater than or equal to op2.
  * `0100` `ULT`: interprets the operands as unsigned values and yields true if op1 is less than op2.
  * `0101` `ULE`: interprets the operands as unsigned values and yields true if op1 is less than or equal to op2.
  * `0110` `NE`: yields true if the operands are unequal, false otherwise. No sign interpretation is necessary or performed.
  * `1010` `SGT`: interprets the operands as signed values and yields true if op1 is greater than op2.
  * `1011` `SGE`: interprets the operands as signed values and yields true if op1 is greater than or equal to op2.
  * `1100` `SLT`: interprets the operands as signed values and yields true if op1 is less than op2.
  * `1101` `SLE`: interprets the operands as signed values and yields true if op1 is less than or equal to op2.

|        | state wires | #dependency wires | data wires               |
|--------|-------------|-------------------|--------------------------|
| input  | []          | 0                 | [`ANYVALUE`, `ANYVALUE`] |
| output | {}          | 0                 | {<<`I1`>>}             |

| Root      | Bitfield       |
|-----------|----------------|
| not used  | condition code |

#### Floating-point binary compare operations

Note: **ordered** means there is no NaN in the operands.

* **FCMP**: returns a boolean (`I1`) value based on comparison of its two floating-point operands. Condition code indicating the kind of comparison to perform is stored in the bitfield. The possible condition codes are:
  * `0000` `FALSE`: always returns false (no comparison)
  * `0001` `OEQ`: ordered and equal
  * `0010` `OGT`: ordered and greater than
  * `0011` `OGE`: ordered and greater than or equal
  * `0100` `OLT`: ordered and less than
  * `0101` `OLE`: ordered and less than or equal
  * `0110` `ONE`: ordered and not equal
  * `0111` `ORD`: ordered (no nans)
  * `1000` `UNO`: unordered (either nans)
  * `1001` `UEQ`: unordered or equal
  * `1010` `UGT`: unordered or greater than
  * `1011` `UGE`: unordered or greater than or equal
  * `1100` `ULT`: unordered or less than
  * `1101` `ULE`: unordered or less than or equal
  * `1110` `UNE`: unordered or not equal
  * `1111` `TRUE`: always returns true (no comparison)


|        | state wires | #dependency wires | data wires               |
|--------|-------------|-------------------|--------------------------|
| input  | []          | 0                 | [`ANYVALUE`, `ANYVALUE`] |
| output | {}          | 0                 | {<<`I1`>>}             |

| Root      | Bitfield       |
|-----------|----------------|
| not used  | condition code |

#### Conversion operations

* **TRUNC**: truncates its integer operand.
* **ZEXT**: zero extends its integer operand.
* **SEXT**: sign extends its integer operand.
* **SITOFP**: regards value as a signed integer and converts that value to floating-point type.
* **UITOFP**: regards value as an unsigned integer and converts that value to floating-point type.
* **FPTOSI**: converts a floating-point value to its signed integer equivalent.
* **FPTOUI**: converts a floating-point value to its unsigned integer equivalent.
* **BITCAST**: converts value to another type without changing any bits.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 0                 | [`ANYVALUE`] |
| output | {}          | 0                 | {<<`FLEX`>>} |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### Memory operations

##### Load

This instruction is used to read from memory.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 1                 | [`ARCH`]     |
| output | {}          | any               | {<<`FLEX`>>} |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

##### Store

This instruction is used to write to memory.

|        | state wires | #dependency wires | data wires           |
|--------|-------------|-------------------|----------------------|
| input  | []          | 1                 | [`ANYVALUE`, `ARCH`] |
| output | {}          | any               | {}                   |

| Root      | Bitfield |
|-----------|----------|
| not used  | not used |

#### Prologue values

##### ALLOCA

This instruction allocates memory on the stack frame of the currently executing function, to be automatically released when this function returns to its caller.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 0                 | []           |
| output | {}          | 0                 | {<<`ARCH`>>} |

| Root          | Bitfield                  |
|---------------|---------------------------|
| `ALLOCA_LIST` | number of allocated bytes |

##### ARG

This instruction is used to represent an argument of the function.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 0                 | []           |
| output | {}          | 0                 | {<<`FLEX`>>} |

| Root        | Bitfield         |
|-------------|------------------|
| `ARG_LIST`  | order of the arg |

##### CONSTANT

This instruction directly returns a specified constant.

|        | state wires | #dependency wires | data wires   |
|--------|-------------|-------------------|--------------|
| input  | []          | 0                 | []           |
| output | {}          | 0                 | {<<`FLEX`>>} |

| Root            | Bitfield                            |
|-----------------|-------------------------------------|
| `CONSTANT_LIST` | bits representation of the constant |

### LIR instructions

Currently, LIR instructions are not implemented. They will be added later for JIT compilation.
