# Circuit IR specification

## General design

Circuit IR is like a circuit diagram, which is good at representing the intrinsic logic of a computation process. The circuit diagram is a directed graph: [logic gates](https://en.wikipedia.org/wiki/Logic_gate) are nodes, wires are directed edges. Circuit IR describes semantic of programs in a language-neutral and target-neutral way, targeting multi-language and multi-target support. The design of Circuit IR has a great emphasis on compilation speed (for JIT), code optimization (on both high and low levels), and canonicalization (unique representation for same intrinsic logic).

Circuit IR splits a program into two major parts: [sequential logic](https://en.wikipedia.org/wiki/Sequential_logic) part and [combinational logic](https://en.wikipedia.org/wiki/Combinational_logic) part:

* The **sequential logic** part is a subgraph of Circuit IR which is similar to the underlying control flow graph (CFG) of the program, and gates in this part are named **state gates** (since they acted like a [finite state machine](https://en.wikipedia.org/wiki/Finite-state_machine) (FSM)). Wires that connect two state gates represent possible state transitions of the FSM, and they are named **state wires**.

* The **combinational logic** part is the other subgraph of Circuit IR which represents all computations in a program using a directed acyclic graph (DAG), and gates in this part are named **computation gates**. A computation gate can do simple things such as adding two integer values, or complex things such as calling a function (and thus make a change to the global memory). Most of **computation gates** will take some values as input and output a value. These values can be transferred by **data wires**. Some computation gates will load from or store to the global memory, so they should be executed non-simultaneously and in some order that will not violate memory dependencies (such as RAW, WAR and WAW). Addressing this issue, **dependency wires** are introduced to constrain the possible execution order of such computations. When a computation gate has multiple dependencies, an auxiliary gate `DEPEND_AND` is used to merge dependencies.

In traditional [SSA](https://en.wikipedia.org/wiki/Static_single_assignment_form) form IR (e.g. LLVM IR), each instruction is placed inside a node of CFG (basic block), and all instructions in a basic block are [linearly ordered](https://en.wikipedia.org/wiki/Total_order). However, in Circuit IR, computation gates are not tied to state gates, and they are [partially ordered](https://en.wikipedia.org/wiki/Partially_ordered_set) by wires. Sequential logic part and combinational logic part are loosely coupled, they only interact in three ways:

* State gates that have multiple transitions (corresponding to multiple control flow branches) such as `IF_BRANCH` and `SWITCH_BRANCH` will need an input value computed from combinational logic part to select which next state the FSM will transit to.

* Similar to the Φ functions in traditional SSA form IR, there are **[selector](https://en.wikipedia.org/wiki/Multiplexer) gates** such as `VALUE_SELECTOR_INT64` and `DEPEND_SELECTOR` that select a data or dependency path based on the state transition action of the FSM. Selector gates are affiliated (many-to-one) to `MERGE` state gates (which have several transition sources), and take several values or dependencies from computation part as input, and will select a value or dependency from input as output, based on which state transition action is taken.

* In some cases, a computation gate with side effect (i.e. may store to the global memory) should not be executed before a specific branch state transition action is taken. Addressing this problem, **[relay](https://en.wikipedia.org/wiki/Relay) gates** `DEPEND_RELAY` are introduced. They take a state gate (the target of action) as input, and reinterpret it as a kind of memory dependency that can be used by computation gates with side effect.

Loose coupling of sequential logic part and combinational logic part can benefit compilation speed, code optimization. Firstly, most of IR passes can focus on only one part and will not be interfered by the other part, so the implementation will be simpler. In addition, some special and important code optimizations such as (register pressure sensitive) loop invariant code motion and code sink can be done in the IR scheduling phrase (without furthermore IR analysis or modification), thus they will not couple with other code optimizations and can be done perfectly. Last but not least, less coupling means more canonicalization, this implies fewer branches in the implementation of IR optimization algorithms.

There are several gates named **root gates** in Circuit IR. Root gates are representing starting/ending vertices of the IR graph, or registering lists of starting/ending vertices of the IR graph. IR Passes usually traverse a part of all gates (forwardly or reversely) starting from root gates. Explanations of Root gates are listed below:

* `STATE_ENTRY`: representing the initial state of the sequential logic part (for traversing forwardly from beginning to ending)

* `DEPEND_ENTRY`: the origin of dependency flows of the entire circuit

* `RETURN_LIST`: registering all terminal states (for traversing reversely from ending to beginning)
  
* `CONSTANT_LIST` `ARG_LIST`: registering all value origins such as constants and arguments (they are special computation gates that do not depend on other values)

## Type system

### Levels of types 

There are two levels of types of values in Circuit IR:

* Primary types: This level of types are **low level** (closer to ISA) and **fundamental** for Circuit IR, and are determined by the opcode of gates. They describe the bit width of values, and which type of registers (integer or float) should such values be put into. Circuit IR can be translated to correct machine code with only primary types. All primary types are `INT1` `INT8` `INT16` `INT32` `INT64` `FLOAT32` `FLOAT64`. Note that pointer type is not included in primary types, since pointers are not different from integers in common ISAs. The concept of pointers should be expressed in secondary types.

* Secondary types: This level of types are **high level** (closer to language) and **optional**. They can provide information for program analysis, code optimization and garbage collection (generating stack maps). Secondary types can represent categories of possible bit vectors the value can be (e.g. `JS_ANY` `JS_BOOLEAN` `JS_NULL` `JS_UNDEFINED` `JS_NUMBER` `JS_SMI` etc. builtin categories and user defined bit vectors categories), and possible classes of objects the value points to as a pointer (e.g. `JS_HEAP_OBJECT` `JS_STRING` `JS_OBJECT` `JS_ARRAY` `JS_TYPED_ARRAY` `JS_UINT8_ARRAY` etc. builtin classes and user defined classes).

### Type inference

Primary types are all set during construction of Circuit IR, so no furthermore type inference is required. Circuit IR verifier will verify consistency of primary types. Secondary types can be not precisely set during construction (e.g. leaving many intermediate values as `JS_ANY`), and the compiler should be able to do type inference (and check consistency) following data flow paths (e.g. `c:<JS_ANY>=JS_ADD(a:<JS_NUMBER>, b:<JS_STRING>) -> c:<JS_STRING>=JS_ADD(a:<JS_NUMBER>, b:<JS_STRING>)` and `c:<JS_ANY>=VALUE_SELECTOR(a:<JS_STRING>, b:<JS_ARRAY>) -> c:<JS_HEAP_OBJECT>=JS_ADD(a:<JS_STRING>, b:<JS_ARRAY>)`), thus the secondary types will be more precise than initially set at the end.
