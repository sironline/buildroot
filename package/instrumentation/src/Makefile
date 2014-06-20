OBJ=instrumentation.o
LIBS=libinstrumentation.a

all: $(LIBS)

%o: %c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

$(LIBS): $(OBJ)
	$(AR) -r -s $@ $^

clean:
	rm -f *.o
