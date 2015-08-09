
#mips-openwrt-linux-gcc -static -o gprint $(FLAGS) gprint.c $(LIBS) -liconv 
#mips-openwrt-linux-gcc -static -o gstatus gstatus.c 
#mips-openwrt-linux-strip gstatus gprint

FLAGS= -I/home/zentao/guo/tplink/include --static
LIBS= -L/home/zentao/guo/tplink/lib -L/home/zentao/waj/trunk_tplink/staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/lib 
CROSS=mips-openwrt-linux-

gprint:
	$(CROSS)g++ -o gprint $(FLAGS) gprint.cpp $(LIBS) -liconv 
	$(CROSS)g++ -o gstatus gstatus.cpp 
	$(CROSS)g++ -o printbmp $(FLAGS) bmp.cpp $(LIBS) 
	$(CROSS)strip gstatus gprint printbmp


clean:
	rm gprint gstatus printbmp
