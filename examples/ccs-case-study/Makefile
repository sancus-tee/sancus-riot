# name of your application
APPLICATION = hello-world

# If no BOARD is found in the environment, use this default:
BOARD = sancus-msp430

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# CFLAGS += -DDEBUG 
CFLAGS += -DLOG_LEVEL=4
CFLAGS += -DTIMERA_CLOCK_DIVIDER=TIMER_CTL_ID_DIV8

# for timer threads
USEMODULE += auto_init
USEMODULE += periph_timer
USEMODULE += secure_mintimer
USEMODULE += log_color

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0
# SCHED_TEST_STACK ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 0

# LOG_LEVEL = LOG_ALL

rebuild:
	$(MAKE) clean
	$(MAKE) all
	sancus-crypto --fill-macs bin/$(BOARD)/$(APPLICATION).elf -o macs.elf

dump:
	$(MAKE) rebuild
	sancus-sim macs.elf --print-progress-at=100000 --dumpfile=sim.vcd --start-dump-at=500000 --stop-after-sm-violation=-1

sim:
	$(MAKE) rebuild
	sancus-sim macs.elf --stop-after-sm-violation=-1


include $(RIOTBASE)/Makefile.include
