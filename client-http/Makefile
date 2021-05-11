TARGET=client
CFLAGS=-Wall

all: $(TARGET)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$< gandalf.teleinf.labinfo.eiaj.ch 80

$(TARGET): $(TARGET).c

archive: Makefile README $(TARGET).c
	tar cvzf RELEASES/client.tar.gz Makefile README $(TARGET).c
