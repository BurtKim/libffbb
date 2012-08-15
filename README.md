Camera API add-on with FFmpeg support.

Support forum post:  
http://supportforums.blackberry.com/t5/Native-Development/Camera-API-NV12-frame-to-AVFrame-FFmpeg/td-p/1842089

Download FFmpeg:

	git clone git://source.ffmpeg.org/ffmpeg.git ffmpeg
	OR
	[http://ffmpeg.org](http://ffmpeg.org)

Building FFmpeg:

	$ # build (shared) libs for armle-v7
	$ ./configure --enable-cross-compile --cross-prefix=arm-unknown-nto-qnx8.0.0eabi- --arch=armv7 --disable-debug --enable-optimizations --enable-asm --disable-static --enable-shared --target-os=qnx --disable-ffplay --disable-ffserver --disable-ffprobe --prefix=`pwd`/target  
	$ make install 

	$ # build (static) libs for x86 (this config won't work)
	$ ./configure --enable-cross-compile --cross-prefix=i486-pc-nto-qnx8.0.0- --arch=x86 --disable-debug --enable-static --disable-shared --target-os=qnx --disable-ffplay --disable-ffserver --disable-ffprobe --disable-yasm --prefix=`pwd`/target  
	$ make install   

Compiling against this library:

	device {
		ARCH = armle-v7
	}
	
	simulator {
		ARCH = x86
		LIBS += -lsocket -lz -lbz2
	}
	
	LIBS += -lcamapi -L../ffmpeg/lib/$${ARCH} -lavformat -lavcodec -lavutil

Building BAR:

	<!-- include libs for armle-v7 -->
	<asset path="ffmpeg/lib/armle-v7/libavcodec.so.54">lib/libavcodec.so.54</asset>
	<asset path="ffmpeg/lib/armle-v7/libavformat.so.54">lib/libavformat.so.54</asset>
	<asset path="ffmpeg/lib/armle-v7/libavutil.so.51">lib/libavutil.so.51</asset>
	
	<!-- include libs for x86 -->
	<asset path="ffmpeg/lib/x86/libavcodec.a">lib/libavcodec.a</asset>
	<asset path="ffmpeg/lib/x86/libavformat.a">lib/libavformat.a</asset>
	<asset path="ffmpeg/lib/x86/libavutil.a">lib/libavutil.a</asset>