
LIB=libhotshadeop

VERSION?=release

GUERILLA_ROOT=/usr/local/guerilla

###############################################################
# The following variables can be overriden in the config.make
# optional file:

# This is where we lookup for guerilla headers
GUERILLA_INCLUDE=$(GUERILLA_ROOT)/devkit/include
# This is where we lookup for libguerilla.a
GUERILLA_LIB=$(GUERILLA_ROOT)/devkit/lib
# This is where we lookup for OpenEXR headers
OPENEXR_INCLUDE=/usr/local/include/OpenEXR
# This is where we lookup for OpenEXR libs
OPENEXR_LIB=/usr/local/lib
# This is where the plugin is to be installed when make install
INSTALL=/usr/local/guerilla/plugins
# These are the initial compilation flags
# Guerilla is compiled with gcc 4.9, which does not support the
# C++11 ABI. gcc 5+ must use this flag
CPPFLAGS=-D_GLIBCXX_USE_CXX11_ABI=0

# Put your overrides here
sinclude config.make
###############################################################

CPPFLAGS+=-DGUERILLA_PLUGIN -DGUERILLA_CONFIG=$(VERSION) -ggdb -Wall -fPIC -MD \
	-std=c++11 -Wno-deprecated -fno-strict-aliasing -fno-omit-frame-pointer \
	-I$(GUERILLA_INCLUDE) \
	-I$(OPENEXR_INCLUDE) \
	-I../3rdparty/include \
	-I../3rdparty/linux/include

ifeq ($(VERSION),debug)
CPPFLAGS?=-O0 -DDEBUG
else ifeq ($(VERSION),dev)
CPPFLAGS?=-O3 -DDEBUG
else ifeq ($(VERSION),release)
CPPFLAGS?=-O3 -DNDEBUG
endif

SRCS := $(shell ls *.cpp)
PWD = $(shell pwd)
OBJS=$(SRCS:%.cpp=$(VERSION)/%.o)

build: $(VERSION) $(VERSION)/$(LIB).so

install: $(INSTALL)/$(LIB).so

$(VERSION):
	mkdir -p $(VERSION)

$(INSTALL):
	mkdir -p $(INSTALL)

$(INSTALL)/$(LIB).so: $(INSTALL) $(VERSION)/$(LIB).so
	cp $(VERSION)/$(LIB).so $(INSTALL)
ifneq ($(VERSION),debug)
	strip --strip-all $(INSTALL)/$(LIB).so
endif

$(OBJS): $(VERSION)/%.o: $(PWD)/%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(VERSION)/$(LIB).so: $(OBJS)
	$(CXX) -shared -Wl,-soname,$(LIB).so -o $(VERSION)/$(LIB).so \
		$(OBJS) \
		-static-libstdc++ -static-libgcc \
		-L$(GUERILLA_LIB) -lguerilla \
		-L$(OPENEXR_LIB) -lIex -lImath \
		-L../3rdparty/linux/lib -lfftw3f

clean:
	rm -f $(VERSION)/*.o $(VERSION)/*.a $(VERSION)/*.d $(VERSION)/$(LIB).so $(INSTALL)/$(LIB).so

DEPS=$(SRCS:%.cpp=$(VERSION)/%.d)
sinclude $(DEPS)
