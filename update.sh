#!/bin/bash

cd `dirname $0`

git pull

# 
cd datasheet_en
# https://www.wch-ic.com/downloads/CH32V002DS0_PDF.html
curl -z CH32V002DS0.PDF -o CH32V002DS0.PDF https://www.wch-ic.com/downloads/file/397.html
# https://www.wch-ic.com/downloads/CH32V004DS0_PDF.html
curl -z CH32V004DS0.PDF -o CH32V004DS0.PDF https://www.wch-ic.com/downloads/file/398.html
# https://www.wch-ic.com/downloads/CH32V006DS0_PDF.html
curl -z CH32V006DS0.PDF -o CH32V006DS0.PDF https://www.wch-ic.com/downloads/file/396.html
# https://www.wch-ic.com/downloads/CH32V00XRM_PDF.html
curl -z CH32V00XRM.PDF -o CH32V00XRM.PDF https://www.wch-ic.com/downloads/file/399.html
# https://www.wch-ic.com/downloads/CH32V006DS0_PDF.html
curl -z CH32V007DS0.PDF -o CH32V007DS0.PDF https://www.wch-ic.com/downloads/file/405.html
cd ..

# 
cd datasheet_zh
# https://www.wch.cn/downloads/CH32V002DS0_PDF.html
curl -z CH32V002DS0.PDF -o CH32V002DS0.PDF https://www.wch.cn/download/file?id=473
# https://www.wch.cn/downloads/CH32V004DS0_PDF.html
curl -z CH32V004DS0.PDF -o CH32V004DS0.PDF https://www.wch.cn/download/file?id=474
# https://www.wch.cn/downloads/CH32V006DS0_PDF.html
curl -z CH32V006DS0.PDF -o CH32V006DS0.PDF https://www.wch.cn/download/file?id=475
# https://www.wch.cn/downloads/CH32V00XRM_PDF.html
curl -z CH32V00XRM.PDF -o CH32V00XRM.PDF https://www.wch.cn/download/file?id=472
# https://www.wch.cn/downloads/CH32V007DS0_PDF.html
curl -z CH32V007DS0.PDF -o CH32V007DS0.PDF https://www.wch.cn/download/file?id=476
cd ..

# https://www.wch.cn/downloads/CH32V006EVT_ZIP.html
curl -z CH32V006EVT.ZIP -o CH32V006EVT.ZIP https://www.wch.cn/download/file?id=477
rm -rfv EVT
unzip -O GB2312 *.ZIP

git add . --all
git commit -m "update"
git push origin main
