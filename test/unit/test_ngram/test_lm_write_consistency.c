#include "lm/ngram_model.h"
#include <pocketsphinx/logmath.h>
#include <pocketsphinx/err.h>

#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Count the n-gram lines per order in an ARPA file and compare them with
 * the counts declared in the \data\ section.  A well-formed writer must
 * emit exactly as many lines in each \k-grams: section as it announced. */
static int
check_arpa_consistency(const char *path)
{
	FILE *fp = fopen(path, "r");
	char line[4096];
	int header[16];
	int actual[16];
	int max_order = 0;
	int order = 0;
	int i, k, n;

	TEST_ASSERT(fp);
	for (i = 0; i < 16; i++)
		header[i] = actual[i] = 0;

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "ngram %d=%d", &k, &n) == 2) {
			if (k > 0 && k < 16) {
				header[k] = n;
				if (k > max_order)
					max_order = k;
			}
			continue;
		}
		if (sscanf(line, "\\%d-grams:", &k) == 1) {
			order = (k > 0 && k < 16) ? k : 0;
			continue;
		}
		if (strncmp(line, "\\end\\", 5) == 0) {
			order = 0;
			continue;
		}
		if (order > 0 && line[0] != '\n' && line[0] != '\\')
			actual[order]++;
	}
	fclose(fp);

	TEST_ASSERT(max_order > 0);
	for (i = 1; i <= max_order; i++) {
		E_INFO("order %d: header %d actual %d\n", i, header[i], actual[i]);
		TEST_EQUAL(header[i], actual[i]);
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	(void)argc;
	(void)argv;
	err_set_loglevel(ERR_INFO);

	lmath = logmath_init(1.0001, 0, 0);

	/* A consistent binary model must convert to an internally consistent
	 * ARPA file and round-trip back without error. */
	E_INFO("Converting consistent BIN to ARPA\n");
	model = ngram_model_read(NULL, LMDIR "/100.lm.bin", NGRAM_BIN, lmath);
	TEST_ASSERT(model);
	TEST_EQUAL(0, ngram_model_write(model, "100.consistency.tmp.lm",
					NGRAM_ARPA));
	ngram_model_free(model);

	E_INFO("Verifying \\data\\ counts match section contents\n");
	check_arpa_consistency("100.consistency.tmp.lm");

	E_INFO("Re-reading converted ARPA\n");
	model = ngram_model_read(NULL, "100.consistency.tmp.lm", NGRAM_ARPA,
				 lmath);
	TEST_ASSERT(model);
	ngram_model_free(model);

	logmath_free(lmath);
	return 0;
}
