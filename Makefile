# SPDX-License-Identifier: GPL-3.0-only
PREFIX:=/usr/local
CFLAGS:=-std=c99 -O2 -pthread

# "-Wl,--nxcompat" specified to enable Windows DEP for the executable code
ifeq ($(OS),Windows_NT)
CFLAGS += -Wl,--nxcompat
else
CFLAGS += -I /usr/local/cuda/include
endif

RETPOLINE_CFLAGS:=


# check C compiler repoline using version information
ifeq ($(shell $(CC) --version 2>&1 | grep -o clang),clang)
	# check for retpoline support (clang)
	ifeq ($(shell $(CC) -Werror \
			-mretpoline -mretpoline-external-thunk \
			-E -x c /dev/null -o /dev/null >/dev/null 2>&1 && echo $$? || echo $$?),0)
RETPOLINE_CFLAGS := -mretpoline -mretpoline-external-thunk -fcf-protection=full
	else
$(error Error: $(CC) (clang) does not support retpoline!)
	endif
else
	# check for RedHat/Ubuntu GCC version string
	ifeq ($(if $(shell $(CC) --version 2>&1 | grep -o GCC),gcc,$(if $(shell $(CC) -v 2>&1 | grep -o gcc),gcc,$(shell $(CC) --version 2>&1 | grep -o gcc))),gcc)
		# check for retpoline support (gcc)
		ifeq ($(shell $(CC) -Werror \
				-mindirect-branch=thunk-extern -mindirect-branch-register \
				-E -x c /dev/null -o /dev/null >/dev/null 2>&1 && echo $$? || echo $$?),0)
RETPOLINE_CFLAGS := -mindirect-branch=thunk-extern -mindirect-branch-register -fcf-protection=full
			# check for Intel Skylake+ from model 78 (cpu family 6 extended model 4 and model e)
			ifeq ($(shell [ -e "/proc/cpuinfo" ] && echo cpuinfo),cpuinfo)
				ifeq ($(shell cat /proc/cpuinfo | grep vendor_id | head -n 1 | grep GenuineIntel > /dev/null && echo "GenuineIntel"),GenuineIntel)
					ifeq ($(shell [ "$$(cat /proc/cpuinfo | grep 'cpu family[[:space:]]*: 6' > /dev/null && \
						cat /proc/cpuinfo | grep '^model[[:space:]]*:' | head -n 1 | awk '{ print $$3; }')" -ge "78" ] && \
						echo "skylake+"),skylake+)
RETPOLINE_CFLAGS += -mfunction-return=thunk-extern
					endif
				endif
			endif
		else
$(error Error: $(CC) (gcc) does not support retpoline!)
		endif
	# found unexpected C compiler
	else
$(error Error: Detected unexpected C compiler (unknown retpoline support)!)
	endif
endif

.PHONY: nvidia-temp clean install uninstall

nvidia-temp:
	$(CC) $(RETPOLINE_CFLAGS) $(CFLAGS) -o nvidia-temp ./nvidia-temp.c -l nvidia-ml

clean:
	-rm nvidia-temp

install:
	install nvidia-temp $(PREFIX)/bin

uninstall:
	rm $(PREFIX)/bin/nvidia-temp
