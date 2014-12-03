#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <getopt.h>
#include <unistd.h>

/* hoedown */
#include "hoedown/document.h"
#include "hoedown/html.h"

/* wkhtmltopdf */
#include <wkhtmltox/pdf.h>

/* mdpdf */
#include "html_data.h"

#define MDPDF_VERSION "1.1.0"
#define MDPDF_PAGE_BREAK "\n\n<div class=\"page-break\"></div>\n\n"

#define HOEDOWN_IUNIT 4096
#define HOEDOWN_OUNIT 2048
#define HOEDOWN_MAX_NESTING 16
#define HOEDOWN_EXTENSIONS (HOEDOWN_EXT_AUTOLINK | HOEDOWN_EXT_FENCED_CODE | \
	HOEDOWN_EXT_FOOTNOTES | HOEDOWN_EXT_AUTOLINK)

/* Command line flags */
static bool remove_existing_files;
static bool page_break_between_sources;
static char *stylesheet_file_path;
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
 * Function with behaviour like `mkdir -p'
 * http://niallohiggins.com/2009/01/08/mkpath-mkdir-p-alike-in-c-for-unix/
 */
int mkpath(const char *s, mode_t mode){
	char *q, *r = NULL, *path = NULL, *up = NULL;
	int rv;

	rv = -1;
	if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0) {
		return (0);
	}

	if ((path = strdup(s)) == NULL) {
		exit(1);
	}

	if ((q = strdup(s)) == NULL) {
		exit(1);
	}

	if ((r = dirname(q)) == NULL) {
		goto out;
	}

	if ((up = strdup(r)) == NULL) {
		exit(1);
	}

	if ((mkpath(up, mode) == -1) && (errno != EEXIST)) {
		goto out;
	}

	if ((mkdir(path, mode) == -1) && (errno != EEXIST)) {
		rv = -1;
	} else {
		rv = 0;
	}

	out:
		if (up != NULL) {
			free(up);
		}
		free(q);
		free(path);
		return (rv);
}

/*
 * Create a directory for the file in file_path if it does not already exist.
 * Returns error code if unsuccessful.
 */
int create_output_directory(const char *file_path) {
	char *path = strdup(file_path);
	char *dir = dirname(path);
	return mkpath(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/*
 * Create a pdf from html.
 * Reads the html file in input_html_file_path and writes a pdf to output_pdf_path.
 * Tries to create a directory for the pdf file, returns without creating pdf if it can't create one.
 */
void generate_pdf(const char *input_html_file_path, const char *output_pdf_path) {
	wkhtmltopdf_global_settings * gs;
	wkhtmltopdf_object_settings * os;
	wkhtmltopdf_converter * conv;

	int err = create_output_directory(output_pdf_path);
	if (err != 0 && err != EEXIST) {
		fprintf(stderr, "Could not create directory for %s\n", output_pdf_path);
		return;
	}

	wkhtmltopdf_init(false);

	gs = wkhtmltopdf_create_global_settings();
	wkhtmltopdf_set_global_setting(gs, "out", output_pdf_path);

	os = wkhtmltopdf_create_object_settings();
	wkhtmltopdf_set_object_setting(os, "page", input_html_file_path);
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
void write_html(FILE *out, hoedown_buffer *ob, bool verbose, char *stylesheet_file_path) {
	fprintf(out, "%s\n", HTML_HEAD_START);
	if (stylesheet_file_path && strlen(stylesheet_file_path) > 0) {
		FILE *s = fopen(stylesheet_file_path, "r");
		if (s == NULL) {
			fprintf(stderr,
				"Could not open %s for reading. Skipping user supplied styles.\n",
				stylesheet_file_path);
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
void read_markdown(hoedown_buffer *ib, int number_of_sources, char **file, FILE *in) {
	if (number_of_sources == 1) { /* Reading from stdin */
		read_file_to_buffer(ib, in); /* may exit on error reading input */
		fclose(in);
	} else {
		int i;
		for (i = 0; i < number_of_sources-1; i++) { /* Reading from one or more files */
			FILE *ir = fopen(file[i], "r");
			read_file_to_buffer(ib, ir);
			if (page_break_between_sources) {
				if (i < number_of_sources-1) {
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
 * Sets the tmp_html_file_protocol_path used by wkhtml to read the html.
 */
void write_html_to_tmp_file(char *tmp_html_file_path, char *tmp_html_file_protocol_path, hoedown_buffer *ob, bool verbose, char *stylesheet) {
	int filedesc = -1;
	const char *fileRoot = get_tmp_dir();

	snprintf(tmp_html_file_path, FILENAME_MAX, "%smdpdf-XXXXXXX.html", fileRoot);
	filedesc = mkstemps(tmp_html_file_path, 5);
	FILE *out = fdopen(filedesc, "w");

	sprintf(tmp_html_file_protocol_path, "file://%s", tmp_html_file_path);

	write_html(out, ob, verbose, stylesheet);
	fclose(out);

	if (verbose) {
		write_html(stdout, ob, verbose, stylesheet);
	}

	hoedown_buffer_free(ob);
}

int main(int argc, char **argv) {
	int c = -1;
	int number_of_sources = 0;
	char **file = NULL;
	FILE *in = stdin;
	char *output_pdf_file_path = NULL;
	hoedown_buffer *ib = hoedown_buffer_new(HOEDOWN_IUNIT);
	hoedown_buffer *ob = hoedown_buffer_new(HOEDOWN_OUNIT);
	char tmp_html_file_path[FILENAME_MAX] = "";
	char tmp_html_file_protocol_path[FILENAME_MAX] = "";

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
					stylesheet_file_path = optarg;
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
	number_of_sources = argc - optind;
	file = argv + optind;

	if (number_of_sources == 0) {
		fprintf(stderr, "No OUTPUT supplied\n");
		mdpdf_usage(EXIT_FAILURE);
	}

	output_pdf_file_path = file[number_of_sources-1];

	FILE *t = fopen(output_pdf_file_path, "r");
	if (t) {
		fclose(t);
		if (!remove_existing_files) {
			fprintf(stderr, "File %s does already exist. Use -f to overwrite it.\n", output_pdf_file_path);
			mdpdf_usage(EXIT_FAILURE);
		}
	}

	/* Main logic */
	read_markdown(ib, number_of_sources, file, in);
	render_markdown(ib, ob);
	write_html_to_tmp_file(tmp_html_file_path, tmp_html_file_protocol_path, ob, verbose, stylesheet_file_path);
	generate_pdf(tmp_html_file_protocol_path, output_pdf_file_path);

	/* Cleanup */
	unlink(tmp_html_file_path);
	return EXIT_SUCCESS;
}