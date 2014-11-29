# mdpfd

A small utility for converting markdown to PDF.
The application can convert from STDIN, one markdown file or multiple
markdown files that get concatinated in the output PDF.

Similar to [gimli](https://github.com/walle/gimli) but tries to be a better *nix citizen.
Should also be a bit faster, but is not benchmarked. Not all features of gimli are implemented.

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

If you have a build environment ready it's easy to install the application.
First make sure you have the dependencies wkhtmltopdf needs, you can see
them for your platform in the [build.py file](https://github.com/wkhtmltopdf/wkhtmltopdf/blob/a902777385100610ec3d0d30a9c1e904d318d1a5/scripts/build.py#L200) in the wkhtmltopdf repository. The example linked to is for Ubuntu.

When you have the dependencies, clone the repository and cd the repository directory.
Use the command `make` to build the application. If everything works, you can run
the tests with `make test`. If the tests pass use `make install` to install, may require sudo.

### Install compiled versions

For now only compiled releases exists for linux-86_64. You can download the binary from the [releases page on github](https://github.com/walle/mdpdf/releases). Put it somewhere on your path to make it usable.

## License

Licensed under MIT license. See [LICENSE](LICENSE) for more information.

## Authors

* Fredrik Wallgren