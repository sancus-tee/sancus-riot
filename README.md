# Sancus-enabled RIOT
This is a fork of [RIOT](https://github.com/RIOT-OS) that allows to run RIOT on [Sancus](https://distrinet.cs.kuleuven.be/software/sancus/) nodes.

The core reason for this project is twofold: First, to enable the use of an embedded OS on Sancus processors. Second, to move certain OS features into an enclave in order to secure parts of the OS and shield them from the attacker.

This project was used in parts in our publication [Aion: Enabling Open Systems through Strong Availability
Guarantees for Enclaves](https://falder.org/files/paper/2021_aion.pdf) where we modify the Sancus architecture and this version of RIOT in order to provide availability guarantees for mutually distrusting enclaves. Aion has been accepted at CCS '21.

## Aion
Aion is a security architecture that consists of multiple parts:

* Hardware modifications to an embedded TEE architecture that allow to place restrictions on atomicity (bounded atomicity) and allow to control interrupt behavior.
* Compiler support for these modifications.
* A real-time operating system that uses these hardware changes by implementing a protected scheduler. This protected scheduler controls all interrupts and can provide enclaves with certain availability guarantees, enforced through the hardware changes and the scheduler's policies.

The full research paper of Aion can be read here: [Aion: Enabling Open Systems through Strong Availability
Guarantees for Enclaves](https://falder.org/files/paper/2021_aion.pdf). 

The individual elements of Aion are split into the separate Sancus subsystems as follows:

* Sancus core: Sancus core adds an atomicity monitor. Most changes were discussed in [this pull request](https://github.com/sancus-tee/sancus-core/pull/24) and are intended to be upstream in Sancus core.
* Sancus compiler: For Aion, the Sancus compiler provides a configuration system that allows to give enclaves control over the interrupt vector table or some MMIO region. These changes are mostly contained in [this pull request](https://github.com/sancus-tee/sancus-compiler/pull/33) and are also intended to be upstream with the Sancus compiler.
* This repository: A modified Riot system that places the scheduler and the timer inside an enclave and protect them from an attacker. The most interesting files are as follows:

| File | Path | Description |
| :--- | :--- | :---------- |
| [sched.c](core/sched.c) | core/ | The scheduler. |
| [secure_mintimer_core.c](sys/secure_mintimer/secure_mintimer_core.c) | sys/secure_mintimer | The secure version of the timer that lives inside the scheduler enclave. |
| [cpu.c](cpu/msp430-sancus/cpu.c)  | cpu/msp430-sancus/ | Entry routines into the scheduler. |
| [cpu.h](cpu/msp430-sancus/include/cpu.h)  | cpu/msp430-sancus/include | Main assembly codes for entering and exiting the scheduler. | 
| [Assembly stubs](cpu/msp430-sancus/stubs) | cpu/msp430-sancus/stubs | Assembly stubs when entering and exiting enclaves such as the scheduler or application enclaves. |


## FEATURES
This version of RIOT is severely reduced in its capabilities. Mostly because the broad range of RIOT features was never necessary for the small set of features required for Aion. However, we believe most features can be reintroduced with little developer overhead. The core features of this version are:

* a preemptive, tickless scheduler with priorities
* * This scheduler resides inside an enclave and has exclusive control over the timer. The secure_mintimer module runs together with the scheduler and can provide trusted time.
* high resolution, long-term timers
* Enclaves can receive periodic scheduling from the scheduler which gives them some availability guarantees even in the presence of a strong software adversary.

## Useful compiler defines
There are some compiler defines that can be useful during debugging and testing with the simulator. Ideally, these will be given in the Makefile as an append to the `CFLAGS` variable (see the evaluation example as a good example for this).

| Variable | Options | Description |
| :------- | :------ | :---------- |
| LOG_LEVEL| 1,2,3,4 | Log level, 4 is highest logging. |
| DEBUG_TIMER | None (ifdef) | Disables the security of the timer. Useful for debugging and evaluation but breaks security. |
| MANUAL_SCHEDULER_BOOT | None (ifdef) | Enables a manual boot of the scheduler, aka does not automatically enable the timer. This is useful if some enclaves wish to attest the scheduler at boot time.|
| EVALUATION_ENABLED |None (ifdef)| Enables evaluation metric taking. Breaks security. |
| TIMERA_CLOCK_DIVIDER| TIMER_CTL_ID_DIV1, TIMER_CTL_ID_DIV2, TIMER_CTL_ID_DIV4, TIMER_CTL_ID_DIV8| Timer divider controls how often the timer ticks. Either each cycle (Div1), each 2nd cycle (Div2), 4th (Div4), or 8th cycle (Div8). Hardware is usually fine with a Div4 or Div8 by simulation may want to use a Div1 to speed things up. |

## GETTING STARTED
Check the CI for more information. Install the [Sancus toolchain](https://distrinet.cs.kuleuven.be/software/sancus/install.php) either locally or via one of the [Docker containers](https://github.com/orgs/sancus-tee/packages). Then, run one of the examples under the examples folder which each contain a simple to use run script.

## LICENSE
* Most of the code developed by the RIOT community is licensed under the GNU
  Lesser General Public License (LGPL) version 2.1 as published by the Free
  Software Foundation.
* Some external sources, especially files developed by SICS are published under
  a separate license.
* All code added and modified by KU Leuven as parts of the Sancus modifications and
  Aion additions is licensed under the same LGPL version 2.1 as the original RIOT code.

All code files contain licensing information.

For more information on RIOT, see the RIOT website:
https://www.riot-os.org
