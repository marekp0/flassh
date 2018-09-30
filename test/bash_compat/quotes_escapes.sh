# test quoting and escaping rules
echo ''\''\'''  '\\"\"' \a'\a' 'a a'
echo ""\""\""\  "\\'\'" \a"\a" "a a"

echo "hello
world"

echo hello\
world
