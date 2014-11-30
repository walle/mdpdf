#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>

/* hoedown */
#include "hoedown/document.h"
#include "hoedown/html.h"

/* wkhtmltopdf */
#include <wkhtmltox/pdf.h>

/* mdpdf */
#include "html_data.h"

#define MDPDF_VERSION "1.0.1"
#define MDPDF_PAGE_BREAK "\n\n<div class=\"page-break\"></div>\n\n"

#define HOEDOWN_IUNIT 4096
#define HOEDOWN_OUNIT 2048
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
Usage: %s [OPTIONS] SOURCE... OUTPUT\n\n\
Converts markdown in SOURCE, or multiple SOURCE(s) to PDF in OUTPUT\n\
Add css rules with with --stylesheet pointing to a file.\n\n\
-f, --force          overwrite destination files\n\
-p, --page-break     adds a page break between sources\n\
-s, --stylesheet     add rules in stylesheet\n\
-v, --verbose        print generated html\n\
-h, --help           display this help and exit\n\
-V, --version        output version information and exit\n", "mdpdf");

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
 * Prints error and exists if an error occurs.
 */
void read_file_to_buffer(hoedown_buffer *ib, FILE *in) {
	while (!feof(in)) {
		if (ferror(in)) {
			fprintf(stderr, "I/O errors found while reading input.\n");
			hoedown_buffer_free(ib);
			fclose(in);
			exit(EXIT_FAILURE);
		}
		hoedown_buffer_grow(ib, ib->size + HOEDOWN_IUNIT);
		ib->size += fread(ib->data + ib->size, 1, HOEDOWN_IUNIT, in);
	}
}

/*
 * Writes the data in the buffer ob to the file out.
 * If stylesheets points to a valid CSS file it's contents will be added to the style of the page.
 */
void write_html(FILE *out, hoedown_buffer *ob, bool verbose, char *stylesheet) {
	fprintf(out, "%s\n", HTML_HEAD_START);
	if (stylesheet && strlen(stylesheet) > 0) {
		FILE *s = fopen(stylesheet, "r");
		if (s == NULL) {
			fprintf(stderr,
				"Could not open %s for reading. Skipping user supplied styles.\n",
				stylesheet);
		} else {
			const int buffer_size = 4096;
			char buffer[buffer_size];
			int n;
			while ((n = fread(buffer, 1, buffer_size, s)) > 0) {
        fwrite(buffer, 1, n, out);
			}
			fclose(s);
		}
	}
	fprintf(out, "%s\n", HTML_HEAD_END);
	fwrite(ob->data, 1, ob->size, out);
	fprintf(out, "%s\n", HTML_END);
}

/*
 * Get a portable tmp dir, defaults to /tmp if no ENV variable is set.
 * Cody by http://codereview.stackexchange.com/users/27623/syb0rg
 * From http://codereview.stackexchange.com/a/71198/58784
 */
const char* get_tmp_dir(void) {
	char *tmpdir = NULL;
	if ((tmpdir = getenv("TEMP"))) return tmpdir;
	else if ((tmpdir = getenv("TMP"))) return tmpdir;
	else if ((tmpdir = getenv("TMPDIR"))) return tmpdir;
	else return "/tmp/";
}

/*
 * Read all markdown input to buffer.
 * Prints error and exists if an error occurs.
 */
void read_markdown(hoedown_buffer *ib, int n_files, char **file, FILE *in) {
	if (n_files == 1) { /* Reading from stdin */
		read_file_to_buffer(ib, in); /* may exit on error reading input */
		fclose(in);
	} else {
		int i;
		for (i = 0; i < n_files-1; i++) { /* Reading from one or more files */
			FILE *ir = fopen(file[i], "r");
			read_file_to_buffer(ib, ir);
			if (page_break_between_sources) {
				if (i < n_files-1) {
					hoedown_buffer_puts(ib, MDPDF_PAGE_BREAK);
				}
			}
			fclose(ir);
		}
	}
}

/*
 * Render markdown in input buffer ib to output buffer ob.
 */
void render_markdown(hoedown_buffer *ib, hoedown_buffer *ob) {
	hoedown_renderer *renderer = hoedown_html_renderer_new(0, 0);
	hoedown_document *document = hoedown_document_new(renderer, HOEDOWN_EXTENSIONS, HOEDOWN_MAX_NESTING);
	hoedown_document_render(document, ob, ib->data, ib->size);
	hoedown_buffer_free(ib);
	hoedown_document_free(document);
	hoedown_html_renderer_free(renderer);
}

/*
 * Create a tmp file and write html output to it.
 * Sets the filePath used by wkhtml to read the html.
 */
void write_html_to_tmp_file(char *nameBuff, char *filePath, hoedown_buffer *ob, bool verbose, char *stylesheet) {
	int filedesc = -1;
	const char *fileRoot = get_tmp_dir();

	snprintf(nameBuff, FILENAME_MAX, "%smdpdf-XXXXXXX.html", fileRoot);
	filedesc = mkstemps(nameBuff, 5);
	FILE *out = fdopen(filedesc, "w");

	sprintf(filePath, "file://%s", nameBuff);

	write_html(out, ob, verbose, stylesheet);
	fclose(out);

	if (verbose) {
		write_html(stdout, ob, verbose, stylesheet);
	}

	hoedown_buffer_free(ob);
}

int main(int argc, char **argv) {
	int c = 0;
	int n_files = 0;
	char **file = NULL;
	FILE *in = stdin;
	char *outname = NULL;
	hoedown_buffer *ib = hoedown_buffer_new(HOEDOWN_IUNIT);
	hoedown_buffer *ob = hoedown_buffer_new(HOEDOWN_OUNIT);
	char nameBuff[FILENAME_MAX] = "";
	char filePath[FILENAME_MAX] = "";

	/* Parse arguments */
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

	/* Validate arguments */
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

	/* Main logic */
	read_markdown(ib, n_files, file, in);
	render_markdown(ib, ob);
	write_html_to_tmp_file(nameBuff, filePath, ob, verbose, stylesheet);
	generate_pdf(filePath, outname);

	/* Cleanup */
	unlink(nameBuff);
	return EXIT_SUCCESS;
}