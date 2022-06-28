# Sistema-Arquivos-EX3
Trabalho final para disciplina Organização de Sistemas Operacionais do curso de Engenharia de Computação da Universidade Federal de Santa Catarina (UFSC).

Para compilar: g++ -std=c++17 fs.cpp main.cpp sha256.cpp -o teste -lgtest -lcrypto -lpthread

### How to use fopen:

> FILE *fopen(const char *filename, const char *mode)

* filename − This is the C string containing the name of the file to be opened.

* mode − This is the C string containing a file access mode. It includes −


1. "r" - Opens a file for reading. The file must exist.

2. "w" - Creates an empty file for writing. If a file with the same name already exists, its content is erased and the file is considered as a new empty file.

3. "a" - Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.

4. "r+" - Opens a file to update both reading and writing. The file must exist.

5. "w+" - Creates an empty file for both reading and writing.

6. "a+" - Opens a file for reading and appending.


### How to use fseek:

>int fseek(FILE *stream, long int offset, int whence)

* stream − This is the pointer to a FILE object that identifies the stream.

* offset − This is the number of bytes to offset from whence.

* whence − This is the position from where offset is added. It is specified by one of the following constants −

  * SEEK_SET: Beginning of file
  
  * SEEK_CUR: Current position of the file pointer

  * SEEK_END: End of file

### How to use fwrite:

> size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)

* ptr − This is the pointer to the array of elements to be written.

* size − This is the size in bytes of each element to be written.

* nmemb − This is the number of elements, each one with a size of size bytes.

* stream − This is the pointer to a FILE object that specifies an output stream.


### How to use fread:

> size_t fread(void * buffer, size_t size, size_t count, FILE * stream);

* buffer: Pointer to the buffer where data will be stored. A buffer is a region of memory used to temporarily store data

* size: The size of each element to be read in bytes

* count: Number of elements to be read

* stream: Pointer to the FILE object from where data is to be read