rm -r ./dist
rm 20190084.zip
mkdir dist
chmod +x bomb
cp docs/20190084.pdf ./dist
cp ./* ./dist
cd dist

rm dist.sh
zip ../20190084.zip ./*

