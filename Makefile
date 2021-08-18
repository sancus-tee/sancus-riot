.all:

.PHONY: all clean print-versions welcome

all: welcome
	@echo ""
	@exit 1
clean:
	@echo "Cleaning all build products for the current board"
	@for dir in $(APPLICATION_DIRS); do "$(MAKE)" -C$$dir clean; done

welcome:
	@echo "Welcome to RIOT - The friendly OS for IoT!"
	@echo ""
	@echo "You executed 'make' from the base directory."
	@echo "You should run 'make' in your application's directory instead."
	@echo ""
	@echo "Please see our Quick Start Guide at:"
	@echo "    https://doc.riot-os.org/getting-started.html"
	@echo "Or ask questions on our mailing list:"
	@echo "    users@riot-os.org (http://lists.riot-os.org/mailman/listinfo/users)"

print-versions:
	@./dist/tools/ci/print_toolchain_versions.sh

include makefiles/boards.inc.mk
include makefiles/app_dirs.inc.mk

-include makefiles/tests.inc.mk
