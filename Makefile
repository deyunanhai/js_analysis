
SRC_DIR = src
RELEASE_DIR = release 
OBJ_DIR = obj
TOOL_DIR = tools
CC = gcc
APPNAME = js_analysis
THIRD_PARTY_DIR = third-party
THIRD_PARTY_DIST = $(THIRD_PARTY_DIR)/dist
OTHER_LIBS = $(THIRD_PARTY_DIST)/lib/libjs.a
CFILES =  	   \
	analysis.c \
	jsfile.c   \
	main.c
INCLUDES = -I$(THIRD_PARTY_DIST)/include -I$(THIRD_PARTY_DIST)/include/nspr
CFLAGS =  -Wall -Wno-format -O -fPIC 
CFLAGS += -DXP_UNIX -DSVR4 -DSYSV -D_BSD_SOURCE -DPOSIX_SOURCE -DHAVE_LOCALTIME_R
CFLAGS += -DHAVE_VA_COPY -DVA_COPY=va_copy -DPIC -DJS_HAS_FILE_OBJECT 
CFLAGS += $(INCLUDES)
LDFLAGS = 
PROG_LIBS = -lm -lnspr4 -lpthread -ldl
TOOL_LIBS = $(PROG_LIBS)
PROG = $(OBJ_DIR)/$(APPNAME) 

PROG_OBJS  = $(addprefix $(OBJ_DIR)/, $(CFILES:.c=.o))
define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir -p $(@D); fi
endef

JS_FILE_DIR = lib
JS_LIB_DIR = $(OBJ_DIR)/lib
JS_FILES = 	        \
	common.js       \
	constants.js    \
	make_public.js  \
	console.js      \
	test_main.js
JS_OBJS  = $(addprefix $(JS_LIB_DIR)/, $(JS_FILES))
JS_CREATOR  = $(OBJ_DIR)/js_creator

$(PROG): $(THIRD_PARTY_DIST) $(PROG_OBJS) $(JS_OBJS)
	$(CC) -o $@ $(CFLAGS) $(PROG_OBJS) $(LDFLAGS) $(OTHER_LIBS) \
	$(PROG_LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $(SRC_DIR)/$*.c

$(JS_CREATOR): $(TOOL_DIR)/js_creator.c
	@$(MAKE_OBJDIR)
	$(CC) -o $(JS_CREATOR) $(CFLAGS) $(TOOL_DIR)/js_creator.c $(LDFLAGS) $(OTHER_LIBS) $(TOOL_LIBS) 

$(JS_LIB_DIR)/%.js: $(JS_FILE_DIR)/%.js
	@if test ! -d $(JS_LIB_DIR); then rm -rf $(JS_LIB_DIR); mkdir -p $(JS_LIB_DIR); fi
	cp $(JS_FILE_DIR)/$*.js $@

$(JS_LIB_DIR)/constants.js: $(JS_CREATOR)
	cat $(JS_FILE_DIR)/constants.js.in > $(JS_LIB_DIR)/constants.js
	$(JS_CREATOR) >> $(JS_LIB_DIR)/constants.js
 
$(THIRD_PARTY_DIST): 
	@cd $(THIRD_PARTY_DIR); ./build.sh

all: $(PROG)

js_lib: $(JS_OBJS)

clean: 
	rm -rf $(OBJ_DIR)
	rm -rf $(RELEASE_DIR)
	@cd $(THIRD_PARTY_DIR); ./clean.sh

release: $(PROG)
	@if test ! -d $(RELEASE_DIR); then rm -rf $(RELEASE_DIR); mkdir -p $(RELEASE_DIR); fi
	cp $(PROG) $(RELEASE_DIR) && cp -r $(JS_LIB_DIR) $(RELEASE_DIR)
	@cd $(RELEASE_DIR); strip -s $(APPNAME)


