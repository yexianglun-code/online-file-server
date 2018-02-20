DIR  := ./Debug
EXE  := ftpserver
LIBS := -lpthread -I /usr/include/mysql/ -lmysqlclient -lcrypt -lcrypto
SRCS := $(wildcard ./source/*.c)
$(EXE):$(SRCS)
	gcc $^ -o $@ $(LIBS)
clean:
	rm -rf $(EXE)
