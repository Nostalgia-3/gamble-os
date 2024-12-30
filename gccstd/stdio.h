#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h> // this is a required header :/

typedef unsigned int size_t;
typedef unsigned int FILE;
typedef unsigned int fpos_t;

#define NULL 0

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 512
#define EOF -1

#define FOPEN_MAX 64 // max # of files open

#define L_tmpnam 8 // max length of a char array

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

// these are supposed to be pointers to FILEs

#define stdin 0
#define stdout 1
#define stderr 2

// Closes the stream. All buffers are flushed
int fclose(FILE *stream);
// Clears the end-of-file and error indicators for the given stream.
void clearerr(FILE *stream);
// Tests the end-of-file indicator for the given stream.
int feof(FILE *stream);
// Tests the error indicator for the given stream.
int ferror(FILE *stream);
// Flushes the output buffer for a stream
int fflush(FILE *stream);
// Gets the current file position of the stream and writes it to pos.
int fgetpos(FILE *stream, fpos_t *pos);
// Opens the filename pointed to by filename using the given mode.
FILE *fopen(const char *filename, const char *mode);
// Reads data from the given stream into the array pointed to by ptr.
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
// Associates a new filename with the given open stream and same time closing the old file in stream.
FILE *freopen(const char *filename, const char *mode, FILE *stream);
// Sets the file position of the stream to the given offset. The argument offset signifies the number of bytes to seek from the given whence position.
int fseek(FILE *stream, long int offset, int whence);
// Sets the file position of the given stream to the given position. The argument pos is a position given by the function fgetpos.
int fsetpos(FILE *stream, const fpos_t *pos);
// Returns the current file position of the given stream.
long int ftell(FILE *stream);
// Writes data from the array pointed to by ptr to the given stream.
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
// Deletes the given filename so that it is no longer accessible.
int remove(const char *filename);
// Causes the filename referred to, by old_filename to be changed to new_filename.
int rename(const char *old_filename, const char *new_filename);
// Sets the file position to the beginning of the file of the given stream.
void rewind(FILE *stream);
// Defines how a stream should be buffered.
void setbuf(FILE *stream, char *buffer);
// Another function to define how a stream should be buffered.
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
// Creates a temporary file in binary update mode (wb+).
FILE *tmpfile(void);
// Generates and returns a valid temporary filename which does not exist.
char *tmpnam(char *str);
// Sends formatted output to a stream.
int fprintf(FILE *stream, const char *format, ...);
// Sends formatted output to stdout.
int printf(const char *format, ...);
// Sends formatted output to a string.
int sprintf(char *str, const char *format, ...);
// Sends formatted output to a stream using an argument list.
int vfprintf(FILE *stream, const char *format, va_list arg);
// Sends formatted output to stdout using an argument list.
int vprintf(const char *format, va_list arg);
// Sends formatted output to a string using an argument list.
int vsprintf(char *str, const char *format, va_list arg);
// Reads formatted input from a stream.
int fscanf(FILE *stream, const char *format, ...);
// Reads formatted input from stdin.
int scanf(const char *format, ...);
// Reads formatted input from a string.
int sscanf(const char *str, const char *format, ...);
// Gets the next character (an unsigned char) from the specified stream and advances the position indicator for the stream.
int fgetc(FILE *stream);
// Reads a line from the specified stream and stores it into the string pointed to by str. It stops when either (n-1) characters are read, the newline character is read, or the end-of-file is reached, whichever comes first.
char *fgets(char *str, int n, FILE *stream);
// Writes a character (an unsigned char) specified by the argument char to the specified stream and advances the position indicator for the stream.
int fputc(int ch, FILE *stream);
// Writes a string to the specified stream up to but not including the null character.
int fputs(const char *str, FILE *stream);
// Gets the next character (an unsigned char) from the specified stream and advances the position indicator for the stream.
int getc(FILE *stream);
// Gets a character (an unsigned char) from stdin.
int getchar(void);
// Reads a line from stdin and stores it into the string pointed to by, str. It stops when either the newline character is read or when the end-of-file is reached, whichever comes first.
char *gets(char *str);
// Writes a character (an unsigned char) specified by the argument char to the specified stream and advances the position indicator for the stream.
int putc(int ch, FILE *stream);
// Writes a character (an unsigned char) specified by the argument char to stdout.
int putchar(int ch);
// Writes a string to stdout up to but not including the null character. A newline character is appended to the output.
int puts(const char *str);
// Pushes the character char (an unsigned char) onto the specified stream so that the next character is read.
int ungetc(int ch, FILE *stream);
// Prints a descriptive error message to stderr. First the string str is printed followed by a colon and then a space.
void perror(const char *str);

#endif