# Computer Architecture Projects
Computer Architecture course's projects repository.
These projects focus on various aspects of computer architecture, including branch prediction, memory caching, instruction-level dependencies, and multithreading.
Each project includes a simulator designed to model these architectural components and analyze their behavior.

## Projects Overview

1. **`Branch-Predictor Simulator:`**
In this exercise, we implemented a simulator for a branch predictor with two levels as implemented in most modern CPUs.
The predictor configuration is flexible and will be defined at the start of the simulator run using parameters.
The simulator environment will receive a run trace of a program describing only the branching events:
including the instruction address, the jump decision, and the calculated target address.
The simulator will be required to make a prediction for each branching event based solely on the instruction address and then
update its state based on the actual instruction identification and the actual branch decision, as detailed in the trace.


2. **`Memory Cache Simulator:`**
In this exercise, we implemented a cache memory simulator. The cache memory configuration is flexible and defined at the start of the run using parameters.
The simulator receives an input file detailing memory accesses in the following format:
w 0x1ff91ca8
r 0x20000cdc
..
The simulator calculates the Hit/Miss rate and the average memory access time for the specified configuration and the given input file.


3. **`Instruction-Level Dependencies Simulator:`**
In this exercise, we implemented a dependency analyzer for the instructions of a given program.
The program interface accepts the processor characteristics and שמ execution trace of a program, and then performד queries regarding the data
dependencies of the instructions in the program.


4. **`Multithreading Simulator:`**
In this exercise, we implemented a simulator that mimics the execution of a multithreaded processor in two configurations:
Block Multithreading and Fine-grained Multithreading.
