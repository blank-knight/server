
GCC ?= gcc 
# 定义了变量GCC为gcc编译器。如果在命令行中没有指定GCC变量的值，则默认为gcc。
CCMODE = PROGRAM
# 定义了变量GCC为gcc编译器。如果在命令行中没有指定GCC变量的值，则默认为gcc。
INCLUDES =  -I/usr/include/mysql
# 定义了变量INCLUDES，其中包含了需要的头文件路径/usr/include/mysql。
CFLAGS =  -Wall $(MACRO) 
# 定义了变量CFLAGS，其中包含了编译选项-Wall（启用所有警告）和$(MACRO)（预处理宏，可能未在提供的代码中定义）。
TARGET = chatSrv
# 定义了变量TARGET为chatSrv，表示要生成的可执行文件名。
SRCS := $(wildcard *.c)
# 使用通配符将当前目录下的所有.c文件存储到变量SRCS中。
LIBS = -lmysqlclient
# 定义了变量LIBS，其中包含了连接时需要的库-lmysqlclient。


ifeq ($(CCMODE),PROGRAM)
# 条件语句，判断CCMODE是否等于PROGRAM。
$(TARGET): $(LINKS) $(SRCS) #目标规则，指定了生成$(TARGET)所需要的依赖文件。这里使用了变量LINKS（未在提供的代码中定义）和变量SRCS。
	$(GCC) $(CFLAGS) $(INCLUDES) -o $(TARGET)  $(SRCS) $(LIBS) 
# 生成可执行文件的命令。使用变量GCC表示编译器，CFLAGS表示编译选项，INCLUDES表示头文件路径，-o指定输出文件名为$(TARGET)，$(SRCS)表示源文件列表，$(LIBS)表示链接的库。
	@chmod +x $(TARGET)
# 设置生成的可执行文件的执行权限。
	@echo make $(TARGET) ok. 
# 输出编译成功的提示信息。
clean:
	rm -rf $(TARGET)
endif
