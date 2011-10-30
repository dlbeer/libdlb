/* libdlb - data structures and utilities library
 * Copyright (C) 2011 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "bint.h"
#include "containers.h"

static int count_alloc;
static int count_free;

struct bint_chunk *bint_chunk_alloc(void)
{
	struct bint_chunk *c = malloc(sizeof(struct bint_chunk));

	if (!c) {
		perror("Memory allocation failure");
		abort();
	}

	memset(c, 0xee, sizeof(*c));
	count_alloc++;
	return c;
}

void bint_chunk_free(struct bint_chunk *c)
{
	free(c);
	count_free++;
}

static void dump_recurse(const struct bint_chunk *c)
{
	int i;

	if (!c)
		return;

	dump_recurse(c->next);
	printf(" |");

	for (i = BINT_CHUNK_SIZE - 1; i >= 0; i--)
		printf(" %08x", c->data[i]);
}

static void dump(const char *label, const struct bint *b)
{
	printf("    %-20s: %c", label, b->negative ? '-' : ' ');
	dump_recurse(&b->digits);
	printf("\n");
}

static void check(const struct bint *b)
{
	/* Make sure that dynamically allocated chains don't have any
	 * zero chunks on the most significant end.
	 */
	if (b->digits.next) {
		const struct bint_chunk *c = b->digits.next;
		int i = 0;

		while (c->next)
			c = c->next;

		while (i < BINT_CHUNK_SIZE && !c->data[i])
			i++;

		assert(i < BINT_CHUNK_SIZE);
	}
}

static void parse_num(const char *num, struct bint *out)
{
	int negative = 0;

	bint_set(out, 0);
	check(out);

	while (*num) {
		if (*num == '-')
			negative = 1;
		else if (isdigit(*num))
			bint_digit_push(out, 10, *num - '0');

		num++;
		check(out);
	}

	bint_set_sign(out, negative ? -1 : 1);
	check(out);
}

static void check_num(const char *num, const struct bint *in)
{
	struct bint out;
	int negative = 0;
	int len = strlen(num);
	int actual_sign = bint_get_sign(in);
	int i;

	check(in);

	bint_init(&out, 0);
	bint_copy(&out, in);

	check(&out);
	for (i = len - 1; i >= 0; i--) {
		if (num[i] == '-') {
			negative = 1;
		} else if (isdigit(num[i])) {
			int d = bint_digit_pop(&out, 10);

			assert(d == num[i] - '0');
			check(&out);
		}
	}

	bint_destroy(&out);

	assert(!bint_get_sign(&out));
	assert((negative && actual_sign < 0) ||
	       (!negative && actual_sign >= 0));
}

static void banner(const char *name)
{
	printf("\n");
	printf("------------------------------------------"
	       "------------------------------\n");
	printf("%s\n", name);
	printf("------------------------------------------"
	       "------------------------------\n");
}

/************************************************************************
 * Table-driven tests
 */

static void test_constants()
{
	BINT_DEF_CONSTANT(a, 57);
	BINT_DEF_CONSTANT(b, -29);

	assert(!bint_get(&bint_zero));
	assert(bint_get(&bint_one) == 1);
	assert(bint_get(&a) == 57);
	assert(bint_get(&b) == -29);
}

static void test_basic()
{
	static const int32_t tests[] = {57, 0, 48, -29, 378, -56};
	struct bint a;
	int i;

	banner("test_basic");
	bint_init(&a, 0);

	for (i = 0; i < lengthof(tests); i++) {
		char buf[64];
		int sign_expect = tests[i] < 0 ? -1 : 1;

		if (!tests[i])
			sign_expect = 0;

		snprintf(buf, sizeof(buf), "%d", tests[i]);
		bint_set(&a, tests[i]);
		dump(buf, &a);
		check(&a);
		assert(bint_get(&a) == tests[i]);
		assert(bint_get_sign(&a) == sign_expect);

		bint_set_sign(&a, -sign_expect);
		check(&a);
		assert(bint_get(&a) == -tests[i]);
		assert(bint_get_sign(&a) == -sign_expect);
	}

	bint_destroy(&a);
}

static void test_bit_ops(void)
{
	static const struct {
		const char	*a;
		const char	*b;
		int		bit;
		int		len_a;
		int		len_b;
	} tests[] = {
		{"57", "121", 6, 6, 7},
		{"8206754987621049676820368",
	         "8206754987629845769842576",
		 43, 83, 83},
		{"0", "1267650600228229401496703205376", 100, 0, 101},
		{"49381759843748798948155328818041681145814478182",
		 "49381759843759183541872398473298742138472918374",
		 113, 156, 156},
		{"43850938609527685247698427598247596875464357349875934"
		 "75983459837254",
		 "27611370481023207918263793291867926569491982324416533"
		 "814126080355034438",
		 234, 222, 235}
	};
	struct bint a;
	int i;

	banner("test_bit_ops");
	bint_init(&a, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s | (1 << %d) == %s\n",
		       tests[i].a, tests[i].bit, tests[i].b);

		parse_num(tests[i].a, &a);
		dump("a", &a);
		assert(bint_bit_length(&a) == tests[i].len_a);
		assert(!bint_bit_get(&a, tests[i].bit));

		bint_bit_set(&a, tests[i].bit);
		dump("set", &a);
		check_num(tests[i].b, &a);
		assert(bint_bit_length(&a) == tests[i].len_b);
		assert(bint_bit_get(&a, tests[i].bit));

		bint_bit_set(&a, tests[i].bit);
		dump("set", &a);
		check_num(tests[i].b, &a);
		assert(bint_bit_length(&a) == tests[i].len_b);
		assert(bint_bit_get(&a, tests[i].bit));

		bint_bit_clear(&a, tests[i].bit);
		dump("clear", &a);
		check_num(tests[i].a, &a);
		assert(bint_bit_length(&a) == tests[i].len_a);
		assert(!bint_bit_get(&a, tests[i].bit));

		bint_bit_clear(&a, tests[i].bit);
		dump("clear", &a);
		check_num(tests[i].a, &a);
		assert(bint_bit_length(&a) == tests[i].len_a);
		assert(!bint_bit_get(&a, tests[i].bit));
	}

	bint_destroy(&a);
}

static void test_bitwise(void)
{
	static const struct {
		const char	*a;
		const char	*b;
		const char	*or;
		const char	*and;
		const char	*xor;
		const char	*bic;
	} tests[] = {
		{"57", "29", "61", "25", "36", "32"},
		{"14329473297",
		 "984175938475918347593814759183745",
		 "984175938475918347593819088387473",
		 "10000269569",
		 "984175938475918347593809088117904",
		 "4329203728"},
		{"43598479824986725498674259876429856",
		 "18923745928373984517",
		 "43598479824986744127251755885780261",
		 "295168432364634112",
		 "43598479824986743832083323521146149",
		 "43598479824986725203505827511795744"},
		{"4395803985403984509384",
		 "48574398574935734570134751938457039475139457913874509"
		 "3847519384750398475",
		 "48574398574935734570134751938457039475139457913874568"
		 "4719898253580467659",
		 "3804931606535154440200",
		 "48574398574935734570134751938457039475139457913874187"
		 "9788291718426027459",
		 "590872378868830069184"}
	};
	struct bint a, b, v;
	int i;

	banner("test_bitwise");
	bint_init(&a, 0);
	bint_init(&b, 0);
	bint_init(&v, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s <x> %s\n", tests[i].a, tests[i].b);

		parse_num(tests[i].a, &a);
		dump("a", &a);

		parse_num(tests[i].b, &b);
		dump("b", &b);

		bint_copy(&v, &a);
		bint_or(&v, &b);
		dump("a|b", &v);
		check_num(tests[i].or, &v);

		bint_copy(&v, &a);
		bint_and(&v, &b);
		dump("a&b", &v);
		check_num(tests[i].and, &v);

		bint_copy(&v, &a);
		bint_xor(&v, &b);
		dump("a^b", &v);
		check_num(tests[i].xor, &v);

		bint_copy(&v, &a);
		bint_bic(&v, &b);
		dump("a&~b", &v);
		check_num(tests[i].bic, &v);
	}

	bint_destroy(&a);
	bint_destroy(&b);
	bint_destroy(&v);
}

static void test_shift(void)
{
	static const struct {
		const char	*a;
		const char	*b;
		int		shift;
	} tests[] = {
		{"57", "244813135872", 32},
		{"57", "114", 1},
		{"12354721398579231745290387497123979872134",
		 "775518431312406552307103593558519000943965358772"
	         "69857866439031036972104638902353398382609054040064",
		 192},
		{"90483509348",
		 "26052960536601309522805577444679584600038057978739208"
		 "67567928979713832310439336171718921789846231949261418"
		 "24867052070730261851362165241313709513904780424297224"
		 "7258591483920384",
		 543},
		{"0", "0", 2000}
	};
	int i;
	struct bint a;

	banner("test_shift");
	bint_init(&a, 0x57);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s << %d == %s\n",
		       tests[i].a, tests[i].shift, tests[i].b);

		parse_num(tests[i].a, &a);
		dump("a", &a);
		bint_shift_left(&a, tests[i].shift);
		dump("<<", &a);
		check_num(tests[i].b, &a);
		bint_shift_right(&a, tests[i].shift);
		dump(">>", &a);
		check_num(tests[i].a, &a);
	}

	bint_destroy(&a);
}

static void test_add_sub_cmp(void)
{
	static const struct {
		const char	*a;
		const char	*b;
		const char	*sum;
	} tests[] = {
		{"29", "57", "86"},
		{"-57", "29", "-28"},
		{"-29", "57", "28"},
		{"-57", "-29", "-86"},
		{"5285986754962795428769428756982475698754296875426",
		 "43598374698713945871394857139457193875497139857990",
		 "48884361453676741300164285896439669574251436733416"},
		{"-3875938479138475918374591834759183754987135431987",
		 "438759834795813745981734598314598173459878713",
		 "-3875499719303680104628610100160869156813675553274"}
	};
	int i;
	struct bint a;
	struct bint b;

	banner("test_add_sub_cmp");
	bint_init(&a, 0);
	bint_init(&b, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s + %s == %s\n",
		       tests[i].a, tests[i].b, tests[i].sum);

		parse_num(tests[i].a, &a);
		dump("a", &a);
		parse_num(tests[i].b, &b);
		dump("b", &b);

		assert(bint_compare(&a, &b) < 0);
		assert(bint_compare(&b, &a) > 0);
		assert(bint_compare(&a, &a) == 0);
		assert(bint_compare(&b, &b) == 0);

		bint_add(&a, &b);
		dump("sum", &a);
		check_num(tests[i].sum, &a);

		bint_sub(&a, &b);
		dump("subtract", &a);
		check_num(tests[i].a, &a);

		bint_add(&b, &a);
		dump("sum", &b);
		check_num(tests[i].sum, &b);

		bint_sub(&b, &a);
		dump("subtract", &b);
		check_num(tests[i].b, &b);
	}

	bint_destroy(&a);
	bint_destroy(&b);
}

static void test_digits(void)
{
	static const char *const tests[] = {
		"57",
		"984179584711234",
		"43174295712",
		"-5140012340",
		"41309570238412834098120348",
		"31429483208982134",
		"-32194898432",
		"342173294871293847128934719283471237594238531451245",
		"-7324451734958713495713495719475139847589134759183475913745"
		"1340951245789345719347598174591274621934751934679314123475"
		"-134675810031475601348751647580000137456813451834561045000"
	};
	int i;
	struct bint b;
	struct bint c;

	printf("test_digits:\n");
	bint_init(&b, 0);
	bint_init(&c, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s\n", tests[i]);
		parse_num(tests[i], &b);
		dump("result", &b);
		check_num(tests[i], &b);
		bint_copy(&c, &b);
		check_num(tests[i], &c);
	}

	bint_destroy(&b);
	bint_destroy(&c);
}

static void test_mul(void)
{
	static const struct {
		const char	*a;
		const char	*b;
		const char	*result;
	} tests[] = {
		{"57", "29", "1653"},
		{"10384102385093475091845", "134529485713894761304",
		 "1396967953467053174528839038245555751965880"},
		{"-9134875912847512479847138971394876",
		 "123948120345810349760137509712509832",
		 "-1132250698989667652990844465245621387"
			"463927763498435672193099904420832"},
		{"834069823406813049861309486913486109346",
		 "-13940983109458",
		 "-11627753320222997542757610726518846139705089674794468"},
		{"-431583409683506813458901345819345",
		 "-1345834098130495813049581034860193486013486",
		 "5808396689394866857136743992157220404738792"
			"38569155541579470645336389686670"},
		{"984014598457031450917459834750138754198374509317"
		 "461938745190487593475193754913745109387549348751"
		 "93754904715348",
		 "394871390647138457130948610532984751304587139845"
		 "719354871394875139487513984751938475190375491834"
		 "75103984579183754",
		 "388559212909813553337307398576274973645488926430"
		 "825133097538653022481170927648774569323213663501"
		 "274472372111503249615771742821677268687963594650"
		 "666000879816477107609506015302133193853311749868"
		 "3159159150726663815379556056392"}
	};
	int i;
	struct bint a, b, r;

	banner("test_mul");

	bint_init(&a, 0);
	bint_init(&b, 0);
	bint_init(&r, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  Test %d: %s * %s == %s\n",
		       i, tests[i].a, tests[i].b, tests[i].result);

		parse_num(tests[i].a, &a);
		parse_num(tests[i].b, &b);
		bint_mul(&r, &a, &b);
		dump("r", &r);
		check_num(tests[i].result, &r);
	}

	bint_destroy(&a);
	bint_destroy(&b);
	bint_destroy(&r);
}

static void test_div(void)
{
	const static struct {
		const char	*a;
		const char	*b;
		const char	*q;
		const char	*r;
	} tests[] = {
		{"574", "29", "19", "23"},
		{"574", "-29", "-20", "-6"},
		{"-574", "29", "-20", "6"},
		{"-574", "-29", "19", "-23"},
		{"23987", "324", "74", "11"},
		{"852385018235091283403451782938572937451293875928374",
		 "1",
		 "852385018235091283403451782938572937451293875928374",
		 ""},
		{"57",
		 "852385018235091283403451782938572937451293875928374",
		 "0",
		 "57"},
		{"852385018235091283403451782938572937451293875928374"
		 "90283749182374918273491237498127349812734912734",
		 "12354721398579231745290387497123979872134",
		 "689926539608666413116540624784145833207433041490442"
		 "7810790",
		 "11600016778119280671482306907634989386874"}
	};
	int i;
	struct bint a, b, q, r;

	banner("test_div");
	bint_init(&a, 0);
	bint_init(&b, 0);
	bint_init(&q, 0);
	bint_init(&r, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  Test %d: %s / %s == %s (rem %s)\n",
		       i, tests[i].a, tests[i].b, tests[i].q, tests[i].r);

		parse_num(tests[i].a, &a);
		dump("a", &a);
		parse_num(tests[i].b, &b);
		dump("b", &b);
		bint_div(&q, &a, &b, &r);
		dump("q", &q);
		check_num(tests[i].q, &q);
		dump("r", &r);
		check_num(tests[i].r, &r);
	}

	bint_destroy(&a);
	bint_destroy(&b);
	bint_destroy(&q);
	bint_destroy(&r);
}

static void test_expt(void)
{
	static const struct {
		const char	*base;
		const char	*expt;
		const char	*result;
	} tests[] = {
		{"3", "11", "177147"},
		{"3214324", "21",
		 "44553586608112367399350788249837008870941825958147120"
		 "06216353991615809566804614109594852028019757639219636"
		 "1554155388806120518907355725824"},
		{"2084912384091238409", "3",
		 "9062821514809233503099592413152090958174836005408051929"}
	};
	struct bint b, e, r;
	int i;

	banner("test_expt");

	bint_init(&b, 0);
	bint_init(&e, 0);
	bint_init(&r, 0);

	for (i = 0; i < lengthof(tests); i++) {
		printf("  %s ** %s == %s\n",
		       tests[i].base, tests[i].expt, tests[i].result);

		parse_num(tests[i].base, &b);
		dump("b", &b);

		parse_num(tests[i].expt, &e);
		dump("e", &e);

		bint_expt(&r, &b, &e);
		dump("r", &r);
		check_num(tests[i].result, &r);
	}

	bint_destroy(&b);
	bint_destroy(&e);
	bint_destroy(&r);
}

int main(void)
{
	test_constants();
	test_basic();
	assert(count_alloc == 0);

	test_digits();
	test_bit_ops();
	test_bitwise();
	test_shift();
	test_add_sub_cmp();
	test_mul();
	test_div();
	test_expt();

	printf("\n");
	printf("alloc: %d, free: %d\n", count_alloc, count_free);
	assert(count_alloc > 0);
	assert(count_alloc == count_free);
	return 0;
}
