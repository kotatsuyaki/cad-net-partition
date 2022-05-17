.POSIX:
.PHONY: all
all: main.pdf
main.pdf: README.md frontmatter.md
	pandoc frontmatter.md README.md -o main.pdf --data-dir=. --template=eisvogel --pdf-engine=xelatex --listings --lua-filter=./filters/image-size.lua
