#CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -Wno-format-security
CXXFLAGS =	 -g3  -fmessage-length=0 -Wno-format-security -std=c++11

OBJDIR = 	obj
_OBJS =		TranslateServer.o Session.o SessionManager.o User.o Langs.o Call.o \
ServiceEmail.o ServiceAsync.o ServiceSMSTwilio.o ServiceIOSVerify.o ServicePayPal.o \
 base64.o logging.o 
OBJS =	$(patsubst %, $(OBJDIR)/%, $(_OBJS))

HEADERS =	TranslateServer.h protocol.h Session.h Call.h Langs.h SessionManager.h User.h \
ServiceEmail.h ServiceAsync.h ServiceSMSTwilio.h ServiceIOSVerify.h ServicePayPal.h base64.h

LIBS =		-lssl -lcrypto -lpthread -lmysqlcppconn -lxerces-c -ljsoncpp -lcurlcpp -lcurl

INCPATH =	-Icurlcpp-master/include/  -I/usr/include/jsoncpp/
LIBPATH =	-Lcurlcpp-master/build/src

TARGET =	TranslateServer

$(OBJDIR)/%.o: %.cpp $(HEADERS)
	$(CXX) $(INCPATH) $(CXXFLAGS) -o $@  -c $<

$(TARGET):	$(OBJS) $(HEADERS)
	$(CXX) $(LIBPATH) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
