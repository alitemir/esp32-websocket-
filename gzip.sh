cp ../ican-web/index.html ./data/index.html
cp ../ican-web/data/* ./data/.
mkdir -p ./data/lang
cp ../ican-web/lang/* ./data/lang/.
gzip -9 data/*
gzip -9 data/lang/*