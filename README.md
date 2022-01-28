# RLE-ZIP-Compression

Please use these flags while compiling :
CFLAGS=-w -pthread
LDFLAGS=-lpthread

USAGE :

For Single thread option :

./RLE-Compression-MultiThreaded  file1toEncode file2toEncode .....   > EncodedFile


For Multi thread option :

./RLE-Compression-MultiThreaded  -j NumberofThreads file1toEncode file2toEncode .....   > EncodedFile

