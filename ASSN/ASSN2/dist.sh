rm -r ./dist
rm bomb84.zip
rm bomb84.tar
mkdir dist
chmod +x bomb
cp ./* ./dist

cd dist
rm dist.sh
tar -cvf bomb84.tar ./*
mv bomb84.tar ../
cd ../
zip bomb84.zip bomb84.tar
