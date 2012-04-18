#makefile for standby.bin

INCLUDE   = -I. \
	-I$(KDIR)/include \
	-I$(KDIR)/arch/arm/mach-sun5i/include

STANDBY_OBJ = common.o standby.o pm_debug.o stack.o standby_clock.o dram/dram.o dram/dram_init.o   \
      standby_int.o standby_ir.o standby_key.o standby_power.o  \
      standby_tmr.o standby_twi.o standby_usb.o standby_delay.o

SUSPEND_OBJ = suspend.o pm_debug.o common.o stack.o standby_clock.o standby_twi.o standby_delay.o \
	standby_int.o standby_tmr.o mem_power.o dram/dram.o dram/dram_init.o mem_mmu_pc_asm.o mem_cpu.o \
	mem_mmu_pc.o

RESUME1_OBJ = resume1.o mem_power.o pm_debug.o stack.o mem_mmu_pc.o mem_mmu_pc_asm.o standby_twi.o standby_delay.o \
	standby_int.o standby_tmr.o common.o standby_clock.o mem_cpu.o standby_power.o dram/dram.o dram/dram_init.o \
	mem_tmr.o

RESUME0_OBJ = resume0.o mem_power.o pm_debug.o stack.o standby_twi.o standby_delay.o \
	common.o standby_clock.o resume1_asm.o mem_mmu_pc_asm.o mem_cpu.o
	
STANDBY_OUPUTFILE = standby
SUSPEND_OUPUTFILE = suspend
RESUME1_OUPUTFILE = resume1
RESUME0_OUPUTFILE = resume0


STANDBY_LD_FILE = standby.xn
SUSPEND_LD_FILE = suspend.xn
RESUME1_LD_FILE = resume1.xn
RESUME0_LD_FILE = resume0.xn

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Os -g -march=armv7-a -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian  -c

all: standby suspend resume1 resume0

standby:
	$(CC) $(INCLUDE) $(CFLAGS) common.c           -o common.o
	$(CC) $(INCLUDE) $(CFLAGS) standby.c          -o standby.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c    -o pm_debug.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_clock.c    -o standby_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_int.c      -o standby_int.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_ir.c       -o standby_ir.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_key.c      -o standby_key.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_power.c    -o standby_power.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_tmr.c      -o standby_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_twi.c      -o standby_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_usb.c      -o standby_usb.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_delay.S    -o standby_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) dram/dram.c        -o dram/dram.o
	$(CC) $(INCLUDE) $(CFLAGS) dram/dram_init.c   -o dram/dram_init.o
	$(CC) $(INCLUDE) $(CFLAGS) stack.S            -o stack.o

	$(CROSS_COMPILE)ld -T $(STANDBY_LD_FILE)  -EL  $(STANDBY_OBJ) -o $(STANDBY_OUPUTFILE).elf  -Map $(STANDBY_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(STANDBY_OUPUTFILE).elf > $(STANDBY_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(STANDBY_OUPUTFILE).elf $(STANDBY_OUPUTFILE).bin

	rm -rf *.o ../*.o

	cp standby.bin standby.code

	@echo ----------------------------------------
	@echo well done!
	
suspend:
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/suspend/suspend.c                    -o suspend.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c                            -o pm_debug.o
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/mem_power.c                          -o mem_power.o
	$(CC) $(INCLUDE) $(CFLAGS) ./common.c                                 -o common.o
	$(CC) $(INCLUDE) $(CFLAGS) ./stack.S                                  -o stack.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_int.c                              -o standby_int.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_tmr.c                              -o standby_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_clock.c                          -o standby_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_twi.c                            -o standby_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_delay.S                          -o standby_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) ./dram/dram.c                              -o dram/dram.o
	$(CC) $(INCLUDE) $(CFLAGS) ./dram/dram_init.c                         -o dram/dram_init.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc_asm.S                      -o mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc.c                          -o mem_mmu_pc.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_cpu.c			      -o mem_cpu.o


	$(CROSS_COMPILE)ld -T $(SUSPEND_LD_FILE)  -EL  $(SUSPEND_OBJ) -o $(SUSPEND_OUPUTFILE).elf  -Map $(SUSPEND_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(SUSPEND_OUPUTFILE).elf > $(SUSPEND_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(SUSPEND_OUPUTFILE).elf $(SUSPEND_OUPUTFILE).bin

	rm -rf *.o ../*.o

	cp suspend.bin ./mem/suspend/suspend.code

	@echo ----------------------------------------
	@echo well done!
	
resume1:
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/resume/resume1.c                     -o resume1.o
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/mem_power.c                          -o mem_power.o
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/mem_tmr.c                            -o mem_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c                            -o pm_debug.o
	$(CC) $(INCLUDE) $(CFLAGS) ./common.c                                 -o common.o
	$(CC) $(INCLUDE) $(CFLAGS) ./stack.S                                  -o stack.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_twi.c                            -o standby_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_delay.S                          -o standby_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_clock.c                          -o standby_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_power.c                          -o standby_power.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_int.c                              -o standby_int.o
	$(CC) $(INCLUDE) $(CFLAGS) standby_tmr.c                              -o standby_tmr.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc.c                          -o mem_mmu_pc.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc_asm.S                      -o mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_cpu.c				-o mem_cpu.o
	$(CC) $(INCLUDE) $(CFLAGS) ./dram/dram.c                              -o dram/dram.o
	$(CC) $(INCLUDE) $(CFLAGS) ./dram/dram_init.c                         -o dram/dram_init.o
	
	
	$(CROSS_COMPILE)ld -T $(RESUME1_LD_FILE)  -EL  $(RESUME1_OBJ) -o $(RESUME1_OUPUTFILE).elf  -Map $(RESUME1_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(RESUME1_OUPUTFILE).elf > $(RESUME1_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(RESUME1_OUPUTFILE).elf $(RESUME1_OUPUTFILE).bin

	rm -rf *.o ../*.o

	cp resume1.bin ./mem/resume/resume1.code

	@echo ----------------------------------------
	@echo well done!

resume0:
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/resume/resume0.c           	      -o resume0.o
	$(CC) $(INCLUDE) $(CFLAGS) ./mem/mem_power.c                          -o mem_power.o
	$(CC) $(INCLUDE) $(CFLAGS) ./common.c                                 -o common.o
	$(CC) $(INCLUDE) $(CFLAGS) ./stack.S                                  -o stack.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../pm_debug.c                            -o pm_debug.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_twi.c                            -o standby_twi.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_delay.S                          -o standby_delay.o
	$(CC) $(INCLUDE) $(CFLAGS) ./standby_clock.c                          -o standby_clock.o
	$(CC) $(INCLUDE) $(CFLAGS) ./resume1_asm.S                            -o resume1_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_mmu_pc_asm.S                      -o mem_mmu_pc_asm.o
	$(CC) $(INCLUDE) $(CFLAGS) ./../mem_cpu.c				-o mem_cpu.o
	
	$(CROSS_COMPILE)ld -T $(RESUME0_LD_FILE)  -EL  $(RESUME0_OBJ) -o $(RESUME0_OUPUTFILE).elf  -Map $(RESUME0_OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(RESUME0_OUPUTFILE).elf > $(RESUME0_OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(RESUME0_OUPUTFILE).elf $(RESUME0_OUPUTFILE).bin

	rm -rf *.o ../*.o

	cp resume0.bin ./mem/resume/resume0.code

	@echo ----------------------------------------
	@echo well done!
	
clean:
	-rm *.o *.map *.lst *.bin *.elf *.exe dram/*.o

