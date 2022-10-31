# Imbed

Imbed is a tool to embed & extract files inside PNG images,
and to view what chunks a PNG file contains.

# How to use

You first need to build the executable using make:

```sh
$ make
```

Then, you can run it against a file to embed or extract files, or probe its content

```sh
$ imbed embed image.png my_file out.png
$ imbed probe out.png
$ imbed extract out.png
```
