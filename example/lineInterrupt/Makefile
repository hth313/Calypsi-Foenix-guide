VPATH = .

FOENIX = ../../module/Calypsi-65816-Foenix

# Common source files
ASM_SRCS =
C_SRCS = main.c

MODEL = --code-model=large --data-model=small
LIB_MODEL = lc-sd

FOENIX_LIB = $(FOENIX)/Foenix-$(LIB_MODEL).a
FOENIX_LINKER_RULES = $(FOENIX)/linker-files/Foenix-U.scm

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

CFLAGS = --core=65816 $(MODEL) -I../../module/Calypsi-65816-Foenix/include

obj/%.o: %.s
	as65816 $(CFLAGS) --target=Foenix --debug --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc65816 $(CFLAGS) --target=Foenix --debug --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%-debug.o: %.s
	as65816 $(CFLAGS) --debug --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%-debug.o: %.c
	cc65816 $(CFLAGS) --debug --list-file=$(@:%.o=%.lst) -o $@ $<

lineInterrupt.elf: $(OBJS_DEBUG)
	ln65816 --debug -o $@ $^ $(FOENIX_LINKER_RULES) clib-$(LIB_MODEL).a --list-file=lineInterrupt.lst --cross-reference --rtattr printf=reduced --semi-hosted

lineInterrupt.pgz:  $(OBJS) $(FOENIX_LIB)
	ln65816 -o $@ $^ $(FOENIX_LINKER_RULES) clib-$(LIB_MODEL)-Foenix.a --output-format=pgz --list-file=lineInterrupt.lst --cross-reference --rtattr printf=reduced --rtattr cstartup=Foenix

$(FOENIX_LIB):
	(cd $(FOENIX) ; make all)

clean:
	-rm $(OBJS) $(OBJS:%.o=%.lst) $(OBJS_DEBUG) $(OBJS_DEBUG:%.o=%.lst) $(FOENIX_LIB)
	-rm lineInterrupt.elf lineInterrupt.pgz lineInterrupt-debug.lst lineInterrupt--Foenix.lst
	-(cd $(FOENIX) ; make clean)
