#!/bin/sh

# 1 - неверный вызов скрипта
# 2 - исходный файл не найден или недоступен для чтения
# 3 - не удалось определить тип исходного файла
# 4 - в исходном файле не найден комментарий "Output: <имя>"
# 5 - сборка завершилась с ошибкой

E_USAGE=1
E_NOFILE=2
E_UNKNOWN_TYPE=3
E_NO_OUTPUT=4
E_BUILD=5

if [ $# -ne 1 ]; then
	echo "usage: $0 <source-file>" >&2
	exit "$E_USAGE"
fi

src=$1

if [ ! -f "$src" ] || [ ! -r "$src" ]; then
	echo "$0: cannot read source file: $src" >&2
	exit "$E_NOFILE"
fi

src_dir=$(dirname "$src")
src_base=$(basename "$src")

case "$src_base" in
*.c) kind=c ;;
*.cc | *.cpp | *.cxx | *.C) kind=cxx ;;
*.tex) kind=tex ;;
*)
	echo "$0: unrecognized source type: $src_base" >&2
	exit "$E_UNKNOWN_TYPE"
	;;
esac

output_name=$(awk '
    match($0, /Output:[ \t]*[^ \t*\/]+/) {
        s = substr($0, RSTART, RLENGTH)
        sub(/^Output:[ \t]*/, "", s)
        print s
        exit
    }
' "$src")

if [ -z "$output_name" ]; then
	echo "$0: no 'Output: <name>' comment found in $src" >&2
	exit "$E_NO_OUTPUT"
fi

tmp_dir=$(mktemp -d) || exit "$E_BUILD"

trap 'rm -rf "$tmp_dir"' EXIT
trap 'exit 1' INT TERM HUP

cp "$src" "$tmp_dir/$src_base"

build() {
	case "$kind" in
	c)
		(cd "$tmp_dir" && "${CC:-cc}" -o "$output_name" "$src_base")
		;;
	cxx)
		(cd "$tmp_dir" && "${CXX:-c++}" -o "$output_name" "$src_base")
		;;
	tex)
		(cd "$tmp_dir" && pdflatex -interaction=nonstopmode -halt-on-error "$src_base" >&2) || return 1
		generated="${src_base%.tex}.pdf"
		[ "$generated" = "$output_name" ] || mv "$tmp_dir/$generated" "$tmp_dir/$output_name"
		;;
	esac
}

if ! build || [ ! -f "$tmp_dir/$output_name" ]; then
	echo "$0: build failed for $src" >&2
	exit "$E_BUILD"
fi

cp "$tmp_dir/$output_name" "$src_dir/$output_name"
