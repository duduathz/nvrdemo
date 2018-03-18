AUTOMAKE_OPTIONS=foreign
INCLUDES=-I$(top_srcdir)/include
bin_PROGRAMS=nvrh3c
nvrh3c_SOURCES=nvr.cpp mongoose.c http_manger.cpp Units.cpp tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp sip_manger.cpp sql_manger.cpp data_reader.cpp data_writer.cpp data_transfer.cpp device_manger.cpp block_queue.cpp CppSQLite3.cpp video_stream_manger.cpp video_data_manger.cpp video_manger.cpp nvr_device_mgr.cpp ./*.h ./conf/* ./include/*
nvrh3c_LDFLAGS=-D_GNU_SOURCE
DEFS+=-D_GNU_SOURCE
LIBS +=-ljson -lpthread -ldl -losip2 -losipparser2 -leXosip2 -ljrtp -ljthread -lglog -lsqlite3 -lzmq -lmsgpack
