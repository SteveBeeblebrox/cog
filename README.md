# cog ![GitHub](https://img.shields.io/github/license/SteveBeeblebrox/cog?style=flat-square) ![GitHub last commit](https://img.shields.io/github/last-commit/SteveBeeblebrox/cog?style=flat-square) ![GitHub issues](https://img.shields.io/github/issues-raw/SteveBeeblebrox/cog?style=flat-square) ![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/SteveBeeblebrox/cog?style=flat-square) ![GitHub contributors](https://img.shields.io/github/contributors/SteveBeeblebrox/cog?color=007EC6&style=flat-square) ![GitHub Repo stars](https://img.shields.io/github/stars/SteveBeeblebrox/cog?style=flat-square)
cog (C) Trin Wasinger 2023

Like Rust's Cargo but for C++
## Overview
Rust, an alternative to C++ has a tool called Cargo. Cargo handles packages, building, running, publishing, and even more. It's like g++, make, cmake, gdb, pkg-config, apt-get/dpkg all in one. This project is a minimalist implementation of some of those features for C++ to make development easier.

## Compiling Cog
Build with `make`. Use `make ansif=true` to build cog with support for colored text. Use `make exprfs=true` if you need to support `std::experimental::filesystem` instead of `std::filesystem`.

## `project.cfg` (Or `project.config`)
This file holds settings for your program; it's the heart of a cog project just like `makefile` is for a Unix make project or `Cargo.toml` is for Rust.  The general format is `key=value;` or `#Comment;`. Note that comments must end with a `;` too. Values of `true`, `false`, and `null` are treated specially as are any numeric value. Strings don't have to be quoted unless they are one of the previous special values. Values can be omitted if you want to go with the default (e.g. just `key;`)

The first set of options are the `project.xxx` options:
```R
project.name="MyProject"
project.version="1.0.0"
project.author="Bob Smith"
```
`project.name` is the only required key and must be a string. `project.name` is used as the output executable name (+ `.exe` if applicable). It is also avalible in the `PROJECT_NAME` macro as a const char literal. Similarly, `PROJECT_VERSION` and `PROJECT_AUTHOR` hold the version and author respectively. Currently, these are not used for anything else; however, in the future, they may be used to generate a package index listing.

The next set of options are the `cpp.xxx` options:
```R
cpp.version=11;
cpp.strict=true;
cpp.static=false;
```

`cpp.version` sets the C++ version to use for the project. The default is 11. `cpp.strict` makes all warnings errors that prevent compilation ant also enables additional checks that force you to write valid ISO C++. With this enabled, code you write will be more portable to other compilers. When `cpp.static` is true, dependencies are statically linked (`cpp.static` defaults to false).

Further options are the `which.xxx` options. Unlike the rest, these config cog and not your project:

```R
which.cpp=g++;
which.make=make;
which.pkg-config=pkg-config;
```

These allow you to override the locations of the C++ compiler, make, and pkg-config that cog uses internally. Provide a value that the default command prompt or shell would recognize.

Cog simplifies what it takes to use external libraries in your project. Use the `pkg.xxx` options to require dependencies:

```R
pkg.libpng="2.0";
pkg.libjpeg>="3.0"
pkg?.libR;
```

These entries follow the format `pkg.[name]=[version];`. Version should be a SemVer string, e.g. `"1.0.0"`. If no version is specified, the first one found will be used. Instead of `=`, you may also use `<=` or `>=` to provide maximum or minimum versions acceptable respectively. Note that not all packages are supported; cog can only find those with a `*.pc` entry in the system (You can add these in a `packages` directory next to `project.config` or in the OS specific folders). Use the command `pkg-config --list-all` to see what is avalible on your system. After specifying a package dependency, you may include those files in your program with no further action needed. To add an optional dependency, use `pkg?.[name]` (more on this later).

The final set of values that can be placed in the project config are the `feature.xxx` variables:

```R
feature.LOGGING;
feature.LOGGING.required=feature.OTHER,pkg.formatting;
feature.LOGGING.notes="Enables fancy logging"
```

Features are optional parts of your program that can be enabled when compiling. You could use these for conditional compilation or to switch out libraries for different platforms. To add a feature, write `feature.NAME;` where `NAME` is made of the characters `[A-Z_]`. If given a boolean value of `false`, the feature is disabled by default; otherwise, it is enabled. Use `feature.NAME.notes="text"` to provide an optional description of each feature. This can be viewed later using the command `cog features`. `feature.NAME.required` can be given a comma separated string where each item is in the form `feature.OTHER` or `pkg.name`. When the feature `NAME` is enabled, it will cause all other features in the key to be enabled (recursively) and will make any packages listed required even if they were previously optional (Note that you must still specify the package beforehand as mentioned previously). If enabled, a feature `NAME` will cause the macro `FEATURE_NAME` to be defined in your source files.

## The CLI
The `cog` command itself will print out a help message if no other options are given. Arguments of `-h`, `--help`, and `?` will also cause cog to print the same message. `-v` or `--version` will print out the current version of cog.

The first subcommand is `cog new [Name]`. It will create a new directory called `Name` with a starter project and template `project.cfg` inside. If not give, `Name` defaults to `UntitledProject`. This could overwrite files if used without care.

The next command is `cog features` it prints out a table of the features defined in `project.cfg`, if they are enabled by default, and any notes specified.

The next two commands are similar `cog run` and `cog build`. Both compile any changes in your project if needed (e.g. if `project.cfg` changes or if individual source files change). Source files should be located in a `src` directory next to `project.cfg`. `run` and `build` can be given the options `-x` or `--no-default-features` to disable features that `project.cfg` has enabled by default. By default, cog defines the `DEBUG` macro in your program, but the options `-r` or `--release` disable this behavior and force a complete rebuild. The options `-F NAME` or `--feature NAME` will enabled feature `NAME` even if the `-x` option disabled it. Use this to enable features via the command line. If built with different features than last time, a complete rebuild will be done. That is the extent of what build does. Run first calls build and then executes your program. The output of compilation can be found in the `build` directory. In order to pass command line arguments to your program and not to `cog run`, use the form `cog run <...cog options> -- <...your program options>`. Many different macros are also defined to help you conditionally compile for different targets.

The command `cog test` injects minimal testing dependencies into your project allowing you to write tests embedded in your source files. When building normally, these tests are stripped out. `--feature` and `--no-default-features` may be used just like a normal build. Testing is done via macros. `TEST(id, body)` defines a test named `id` where `body` can be a single expression or a `{...}` block. If the form `cog test -- [ids...]` is used, only tests with a matching `id` are run. Each test is considered valid if no exception is thrown. Use `TASSERT(expression)` to throw an error if `expression` is false. `TPRINTS(body, message)` asserts that evaluating `body` writes exactly `message` to `std::cout` while `TPRINTMATCHES(body, pattern)` checks that the output matches the regex string `pattern`. A simple test could be written like so:
```cpp
int add(int a, int b) {
	return a + b;
}

TEST(commutative_add, TASSERT(add(1,2) == add(2,1)));
```
Note that when running `cog test`, all `std::cout` is captured; however, `std::err` works like usual. Additionally, `main()` is NOT called. `cog test` is still in development and has only ben tested with `which.cpp=g++`.

Additionally, cog supports a subcommand for super cat powers.

## Overall Process
1. Run `cog new <ProjectName>`
2. Edit `<ProjectName>/project.cfg` as needed
3. Edit `<ProjectName>/src/main.cpp` to write your program. Place all other source files in the `src` folder.
4. Run `cog run` to test

## Planned Features
1. Add a cppfront pass to the build step to avoid common pitfalls in code.
2. Some sort of package installer (Maybe from a central index, this would be a big task)
3. Alternatively, ability to pull dependencies from GitHub/GitLab/etc... and compile them before compiling target projects
4. Project publishing and doc generation (Integrate existing tool or scrape doc comments)
5. Option to compile to libraries instead of executables
6. Additional CLI options for finer control
7. `arch.xxx` and `platform.xxx` options in `project.cfg` that can be required from features to further improve conditional compilation and targeting different systems
8. Testing on other systems (Currently only Debian and MinGW on Windows)
9. A `build.cpp` file for compile time scripting (And thus `dev.pkg.xxx` dependencies to go with it)
10. Detect and lock on dependency versions (Expanding `build/project.log`)
11. Profiling (part of `cog test`?) and debugging (`lldb` or `gdb`) subcommands
12. Add more super cat powers...
