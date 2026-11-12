/* NIC-Strelka — the host tests' one helper: a check that names what failed and ends the test */
#ifndef STRELKA_TEST_H
#define STRELKA_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
			       exit(1); } } while (0)
#define OK(what) printf("ok %s\n", what)

#endif
