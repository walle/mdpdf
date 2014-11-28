#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

/* hoedown */
#include "document.h"
#include "html.h"

/* wkhtmltopdf */
#include <wkhtmltox/pdf.h>

/* mdpdf */
#include "html_data.h"

#define MDPDF_VERSION "1.0.0"

#define HOEDOWN_IUNIT 1024
#define HOEDOWN_OUNIT 64
#define HOEDOWN_MAX_NESTING 16
#define HOEDOWN_EXTENSIONS (HOEDOWN_EXT_AUTOLINK | HOEDOWN_EXT_FENCED_CODE | \
	HOEDOWN_EXT_FOOTNOTES | HOEDOWN_EXT_AUTOLINK)

/* Command line flags */
static bool remove_existing_files;
static bool page_break_between_sources;
static char *stylesheet;
static bool verbose;

static struct option const long_options[] = {
	{"force", no_argument, NULL, 'f'},
	{"page-break", no_argument, NULL, 'p'},
	{"stylesheet", optional_argument, NULL, 's'},
	{"verbose", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'V'},
	{NULL, 0, NULL, 0}
};

/*
 * Print usage information and exit with the supplied status.
 */
void mdpdf_usage (int status) {
	printf ("\
	Usage: %s [OPTIONS] SOURCE... OUTPUT\n\
	",
	"mdpdf");
	fputs ("\n\
	Converts markdown in SOURCE, or multiple SOURCE(s) to PDF in OUTPUT\n\
	Add css rules with with --stylesheet pointing to a file.\n\n\
	", stdout);

	fputs ("\
	-f, --force          overwrite destination files\n\
	", stdout);
	fputs ("\
	-p, --page-break     adds a page break between sources\n\
	", stdout);
			fputs ("\
	-s, --stylesheet     add rules in stylesheet\n\
	", stdout);
			fputs ("\
	-v, --verbose        print generated html\n\
	", stdout);
			 fputs ("\
	-h, --help           display this help and exit\n\
	", stdout);
				fputs ("\
	-V, --version        output version information and exit\n\
", stdout);

	exit(status);
}

/*
 * Create a pdf from html input.
 * Reads the html file in input and writes a pdf to output.
 */
void generate_pdf(const char *input, const char *output) {
	wkhtmltopdf_global_settings * gs;
	wkhtmltopdf_object_settings * os;
	wkhtmltopdf_converter * conv;

	wkhtmltopdf_init(false);

	gs = wkhtmltopdf_create_global_settings();
	wkhtmltopdf_set_global_setting(gs, "out", output);

	os = wkhtmltopdf_create_object_settings();
	wkhtmltopdf_set_object_setting(os, "page", input);
	wkhtmltopdf_set_object_setting(os, "load.loadErrorHandling", "ignore");

	conv = wkhtmltopdf_create_converter(gs);
	wkhtmltopdf_add_object(conv, os, NULL);

	if (!wkhtmltopdf_convert(conv)) {
		fprintf(stderr, "Convertion failed!");
	}

	wkhtmltopdf_destroy_converter(conv);
	wkhtmltopdf_deinit();
}

/*
 * Read the whole file in into the buffer ib.
 * Returns a error code if one occurs.
 */
int read_file_to_buffer(hoedown_buffer *ib, FILE *in) {
	while (!feof(in)) {
		if (ferror(in)) {
			return 5;
		}
		hoedown_buffer_grow(ib, ib->size + HOEDOWN_IUNIT);
		ib->size += fread(ib->data + ib->size, 1, HOEDOWN_IUNIT, in);
	}
	return 0;
}

/*
 * Writes the data in the buffer ob to the file out.
 * If verbose is true the output will be printed on stdout.
 * If stylesheets points to a valid CSS file it's contents will be added to the style of the page.
 */
void write_html_to_file(FILE *out, hoedown_buffer *ob, bool verbose, char *stylesheet) {
	fprintf(out, "%s\n", HTML_HEAD_START);
	if (verbose) {
		fprintf(stdout, "%s\n", HTML_HEAD_START);
	}
	if (stylesheet && strlen(stylesheet) > 0) {
		FILE *s = fopen(stylesheet, "r");
		if (s == NULL) {
			fprintf(stderr, "Could not open %s for reading. Skipping user supplied styles.\n", stylesheet);
		} else {
			char c;
			while((c = fgetc(s)) != EOF) {
				fputc(c, out);
			}
			fclose(s);
		}
	}
	fprintf(out, "%s\n", HTML_HEAD_END);
	if (verbose) {
		fprintf(stdout, "%s\n", HTML_HEAD_END);
	}

	(void)fwrite(ob->data, 1, ob->size, out);
	if (verbose) {
		(void)fwrite(ob->data, 1, ob->size, stdout);
	}

	fprintf(out, "%s\n", HTML_END);
	if (verbose) {
		fprintf(stdout, "%s\n", HTML_END);
	}
}

int main(int argc, char **argv) {
	int c;
	int n_files;
	char **file;
	FILE *in = stdin;
	char *outname = NULL;

	while ((c = getopt_long (argc, argv, "fpvhVs:", long_options, NULL)) != -1) {
		switch (c) {
			case 'f':
				remove_existing_files = true;
			break;
			case 'p':
				page_break_between_sources = true;
			break;
			case 's':
				if (optarg)
					stylesheet = optarg;
			break;
			case 'v':
				verbose = true;
			break;
			case 'h':
				mdpdf_usage(EXIT_SUCCESS);
			break;
			case 'V':
				printf("mdpdf - %s\n", MDPDF_VERSION);
				exit (EXIT_SUCCESS);
			break;
			default:
				 mdpdf_usage(EXIT_FAILURE);
			break;
		}
	}

	n_files = argc - optind;
	file = argv + optind;

	if (n_files == 0) {
		fprintf(stderr, "No OUTPUT supplied\n");
		mdpdf_usage(EXIT_FAILURE);
	}

	outname = file[n_files-1];

	FILE *t = fopen(outname, "r");
	if (t) {
		fclose(t);
		if (!remove_existing_files) {
			fprintf(stderr, "File %s does already exist. Use -f to overwrite it.\n", outname);
			mdpdf_usage(EXIT_FAILURE);
		}
	}

	/* Read SOURCE(s) */

	hoedown_buffer *ib = hoedown_buffer_new(HOEDOWN_IUNIT);
	if (n_files == 1) {
		int error_code = read_file_to_buffer(ib, in);
		if (error_code > 0) {
			fprintf(stderr, "I/O errors found while reading input.\n");
			hoedown_buffer_free(ib);
			fclose(in);
			exit(error_code);
		}
	} else {
		int i;
		for (i = 0; i < n_files-1; i++) {
			FILE *ir = fopen(file[i], "r");
			int error_code = read_file_to_buffer(ib, ir);
			if (error_code > 0) {
				fprintf(stderr, "I/O errors found while reading input.\n");
				hoedown_buffer_free(ib);
				fclose(ir);
				exit(error_code);
			}
			if (page_break_between_sources) {
				if (i < n_files-1) {
					hoedown_buffer_puts(ib, "\n\n<div class=\"page-break\"></div>\n\n");
				}
			}
			fclose(ir);
		}
	}

	hoedown_renderer *renderer = hoedown_html_renderer_new(0, 0);
	hoedown_document *document = hoedown_document_new(renderer, HOEDOWN_EXTENSIONS, HOEDOWN_MAX_NESTING);

	hoedown_buffer *ob = hoedown_buffer_new(HOEDOWN_OUNIT);
	hoedown_document_render(document, ob, ib->data, ib->size);
	hoedown_buffer_free(ib);
	fclose(in);

	/* Generate a tmpfile for html output */
	char nameBuff[23];
	char filePath[30];
	int filedesc = -1;
	memset(nameBuff, 0, sizeof(nameBuff));
	memset(filePath, 0, sizeof(filePath));

	strncpy(nameBuff, "/tmp/mdpdf-XXXXXX.html", 22);
	/* Use tmp file to build a file:// path for wkhtml */
	sprintf(filePath, "file://%s", nameBuff);

	filedesc = mkstemps(nameBuff, 5);
	FILE *out = fdopen(filedesc, "w");

	/* Write tmp html output to file */
	write_html_to_file(out, ob, verbose, stylesheet);
	fclose(out);

	/* Cleanup hoedown */
	hoedown_buffer_free(ob);
	hoedown_document_free(document);
	hoedown_html_renderer_free(renderer);

/* start pdf generating */
	generate_pdf(filePath, outname);

	return EXIT_SUCCESS;
}