# mdpfd

A small utility for converting markdown to PDF.
The application can convert from STDIN, one markdown file or multiple
markdown files that get concatinated in the output PDF.

Similar to [gimli](https://github.com/walle/gimli) but tries to be a better *nix citizen.
Should also be a bit faster, but is not benchmarked. Not all features of gimli are implemented.

## Dependencies

mdpdf requires [hoedown](https://github.com/hoedown/hoedown) and [wkhtmltopdf](https://github.com/wkhtmltopdf/wkhtmltopdf/) shared libraries to be installed.

## Usage

```
Usage: mdpdf [OPTIONS] SOURCE... OUTPUT

Converts markdown in SOURCE, or multiple SOURCE(s) to PDF in OUTPUT
Add css rules with with --stylesheet pointing to a file.

-f, --force          overwrite destination files
-p, --page-break     adds a page break between sources
-s, --stylesheet     add rules in stylesheet
-v, --verbose        print generated html
-h, --help           display this help and exit
-V, --version        output version information and exit
```

## Installation

### Install from source

mdpdf requires the shared libraries for [hoedown](https://github.com/hoedown/hoedown) and [wkhtmltopdf](https://github.com/wkhtmltopdf/wkhtmltopdf/) to be installed.
Follow the build instructions in the libraries to install them.
*NOTICE* the hoedown library installs to `/usr/local/lib/` as default and this might not be on your `LD_LIBRARY_PATH` if not you can add it with `LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib` before running the application.

When you have the dependencies, clone the repository and cd the repository directory.
Use the command `make` to build the application. If everything works, you can run
the tests with `make test`. If the tests pass use `make install` to install, may require sudo.

## License

Licensed under MIT license. See [LICENSE](LICENSE) for more information.

## Authors

* Fredrik Wallgren