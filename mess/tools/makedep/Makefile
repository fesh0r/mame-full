CC = @gcc
CFLAGS = -O3 -W -Wall

SRCS = main.c cppsetup.c ifparser.c include.c parse.c pr.c
OBJS = main.o cppsetup.o ifparser.o include.o parse.o pr.o

makedep: $(OBJS)
	@echo Linking $@...
	$(CC) $^ -o $@

$(OBJS): $(SRCS)

%.o:    %.c
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) makedep makedep.exe

depend: $(SRCS)
	include makedep.dep

