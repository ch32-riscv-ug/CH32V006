#!/bin/bash

cd `dirname $0`

git pull

# 
cd datasheet_en
# https://www.wch-ic.com/downloads/CH32V003DS0_PDF.html
#wget --continue https://www.wch-ic.com/downloads/file/359.html -O CH32V003DS0.PDF
# https://www.wch-ic.com/downloads/CH32V003RM_PDF.html
#wget --continue https://www.wch-ic.com/downloads/file/358.html -O CH32V003RM.PDF
cd ..

# 
cd datasheet_zh
# https://www.wch.cn/downloads/CH32V002DS0_PDF.html
wget --continue https://www.wch.cn/downloads/file/473.html -O CH32V002DS0.PDF
# https://www.wch.cn/downloads/CH32V004DS0_PDF.html
wget --continue https://www.wch.cn/downloads/file/474.html -O CH32V004DS0.PDF
# https://www.wch.cn/downloads/CH32V006DS0_PDF.html
wget --continue https://www.wch.cn/downloads/file/475.html -O CH32V006DS0.PDF
# https://www.wch.cn/downloads/CH32V002DS0_PDF.html
wget --continue https://www.wch.cn/downloads/file/473.html -O CH32V002DS0.PDF
# https://www.wch.cn/downloads/CH32V00XRM_PDF.html
wget --continue https://www.wch.cn/downloads/file/472.html -O CH32V00XRM.PDF
cd ..

# https://www.wch.cn/downloads/CH32V006EVT_ZIP.html
wget --continue https://www.wch.cn/downloads/file/477.html -O CH32V006EVT.ZIP
rm -rfv EVT
unzip *.ZIP

git add . --all
git commit -m "update"
git push origin main
