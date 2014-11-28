#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>

#include "document.h"
#include "html.h"

#include "html_data.h"

#include <wkhtmltox/pdf.h>

#define DEF_IUNIT 1024
#define DEF_OUNIT 64
#define DEF_MAX_NESTING 16

enum renderer_type {
  RENDERER_HTML,
  RENDERER_HTML_TOC
};

struct option_data {
  char *basename;
  int done;

  /* time reporting */
  int show_time;

  /* I/O */
  size_t iunit;
  size_t ounit;
  const char *filename;

  /* renderer */
  enum renderer_type renderer;
  int toc_level;
  hoedown_html_flags html_flags;

  /* parsing */
  hoedown_extensions extensions;
  size_t max_nesting;
};

/* If true, remove existing files unconditionally.*/
static bool remove_existing_files;

static bool page_break_between_sources;

static char *stylesheet;

/* If true, print generated html.*/
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

int main(int argc, char **argv) {
  int c;
  int n_files;
  char **file;
  FILE *in = stdin;
  FILE *out = stdout;
  int filedesc = -1;
  char *outname = "test.pdf"; /*TODO: No default*/
  char filePath[30];

  struct option_data data;
  hoedown_buffer *ib, *ob;
  hoedown_renderer *renderer = NULL;
  void (*renderer_free)(hoedown_renderer *) = NULL;
  hoedown_document *document;

  wkhtmltopdf_global_settings * gs;
  wkhtmltopdf_object_settings * os;
  wkhtmltopdf_converter * conv;

  char nameBuff[23];
  memset(nameBuff, 0, sizeof(nameBuff));
  memset(filePath, 0, sizeof(filePath));

  strncpy(nameBuff, "/tmp/mdpdf-XXXXXX.html", 22);

  filedesc = mkstemps(nameBuff, 5);
  out = fdopen(filedesc, "w");

  sprintf(filePath, "file://%s", nameBuff);

  /* Parse options */
  data.basename = argv[0];
  data.done = 0;
  data.show_time = 0;
  data.iunit = DEF_IUNIT;
  data.ounit = DEF_OUNIT;
  data.filename = NULL;
  data.renderer = RENDERER_HTML;
  data.toc_level = 0;
  data.html_flags = 0;
  data.extensions = HOEDOWN_EXT_AUTOLINK |
    HOEDOWN_EXT_FENCED_CODE |
    HOEDOWN_EXT_FOOTNOTES |
    HOEDOWN_EXT_AUTOLINK;
  data.max_nesting = DEF_MAX_NESTING;

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
          printf("0.1.0\n");
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

  if (n_files == 1) {
    /* Read everything */
    ib = hoedown_buffer_new(data.iunit);
    while (!feof(in)) {
      if (ferror(in)) {
        fprintf(stderr, "I/O errors found while reading input.\n");
        return 5;
      }
      hoedown_buffer_grow(ib, ib->size + data.iunit);
      ib->size += fread(ib->data + ib->size, 1, data.iunit, in);
    }
  } else {
    ib = hoedown_buffer_new(data.iunit);
    int i;
    for (i = 0; i < n_files-1; i++) {
      FILE *ir = fopen(file[i], "r");
      while (!feof(ir)) {
        if (ferror(ir)) {
          fprintf(stderr, "I/O errors found while reading input.\n");
          return 5;
        }
        hoedown_buffer_grow(ib, ib->size + data.iunit);
        ib->size += fread(ib->data + ib->size, 1, data.iunit, ir);
      }
      if (page_break_between_sources) {
        if (i < n_files-1) {
          hoedown_buffer_puts(ib, "\n\n<div class=\"page-break\"></div>\n\n");
        }
      }
      fclose(ir);
    }
  }

  /* Create the renderer */
  switch (data.renderer) {
    case RENDERER_HTML:
      renderer = hoedown_html_renderer_new(data.html_flags, data.toc_level);
      renderer_free = hoedown_html_renderer_free;
      break;
    case RENDERER_HTML_TOC:
      renderer = hoedown_html_toc_renderer_new(data.toc_level);
      renderer_free = hoedown_html_renderer_free;
      break;
  };

  /* Perform Markdown rendering */
  ob = hoedown_buffer_new(data.ounit);
  document = hoedown_document_new(renderer, data.extensions, data.max_nesting);

  /*clock_gettime(CLOCK_MONOTONIC, &start);*/
  hoedown_document_render(document, ob, ib->data, ib->size);
  /*clock_gettime(CLOCK_MONOTONIC, &end);*/

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

  /* Write the result to out */
  (void)fwrite(ob->data, 1, ob->size, out);
  if (verbose) {
    (void)fwrite(ob->data, 1, ob->size, stdout);
  }

  fprintf(out, "%s\n", HTML_END);
  if (verbose) {
    fprintf(stdout, "%s\n", HTML_END);
  }

  /* Cleanup */
  hoedown_buffer_free(ib);
  hoedown_buffer_free(ob);

  hoedown_document_free(document);
  renderer_free(renderer);

  fflush(out);

  /* Init wkhtmltopdf in graphics less mode */
  wkhtmltopdf_init(false);

  /*
   * Create a global settings object used to store options that are not
   * related to input objects, note that control of this object is parsed to
   * the converter later, which is then responsible for freeing it
   */
  gs = wkhtmltopdf_create_global_settings();
  /* We want the result to be storred in the file called test.pdf */
  wkhtmltopdf_set_global_setting(gs, "out", outname);

  /*
   * Create a input object settings object that is used to store settings
   * related to a input object, note again that control of this object is parsed to
   * the converter later, which is then responsible for freeing it
   */
  os = wkhtmltopdf_create_object_settings();

  wkhtmltopdf_set_object_setting(os, "page", filePath);
  wkhtmltopdf_set_object_setting(os, "load.loadErrorHandling", "ignore");

  /* Create the actual converter object used to convert the pages */
  conv = wkhtmltopdf_create_converter(gs);

  /*
   * Add the the settings object describing the qstring documentation page
   * to the list of pages to convert. Objects are converted in the order in which
   * they are added
   */
  wkhtmltopdf_add_object(conv, os, NULL);

  /* Perform the actual convertion */
  if (!wkhtmltopdf_convert(conv)) {
    fprintf(stderr, "Convertion failed!");
  }

  /* Destroy the converter object since we are done with it */
  wkhtmltopdf_destroy_converter(conv);

  /* We will no longer be needing wkhtmltopdf funcionality */
  wkhtmltopdf_deinit();

  unlink(nameBuff);

  fclose(in);
  fclose(out);

  return EXIT_SUCCESS;
}