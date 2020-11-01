rm -r "dist"
mkdir "dist"

tar -cf 20190084.tar sol1.hex sol2.hex sol3.hex sol4.hex sol5.hex

mv 20190084.tar ./dist
cp docs/20190084.pdf ./dist