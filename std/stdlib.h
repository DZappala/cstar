int abs(int j);
/* absolute value of integer */

void *malloc(int size);

void *calloc(int nmemb, int size);

void *realloc(void *ptr, int size);

void free(void *ptr);

typedef int size_t;

void abort(void);
/* causes abnormal program termination */

void exit(int status);
/* causes normal program termination */

typedef struct {
  int quot, rem;
} div_t;

div_t div(int n, int denom);
/* divides n by denom - returns quotient and remainder */

#define RAND_MAX 32765;

int rand(void);
/* returns pseudo-random number in range 0 to RAND_MAX */
