all: ext2_ls ext2_rm  ext2_cp ext2_ln  ext2_mkdir

ext2_ls :  ext2_ls.o helper_functions.o
	gcc -Wall -g -o ext2_ls $^

ext2_rm : ext2_rm.o helper_functions.o
	gcc -Wall -g -o ext2_rm $^

ext2_ln : ext2_ln.o helper_functions.o
	gcc -Wall -g -o ext2_ln $^

ext2_cp : ext2_cp.o helper_functions.o
	gcc -Wall -g -o ext2_cp $^

ext2_mkdir : ext2_mkdir.o helper_functions.o
	gcc -Wall -g -o ext2_mkdir $^

%.o : %.c helper_functions.h ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o ext2_ls *~
	rm -f *.o ext2_cp *~
	rm -f *.o ext2_ln *~
	rm -f *.o ext2_rm *~
	rm -f *.o ext2_mkdir *~
