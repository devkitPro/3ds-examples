# Usage: ./build_bossdata.sh {additional param(s) if any for bosstool}
# Requires bosstool from: https://github.com/yellows8/3dscrypto-tools
# Builds the bossdata files @ web/boss/ used by this app.

params="--input=bossinput --build --nsdataid=0x57524248"
echo -n "TestHello world from BOSS!" > bossinput
mkdir -p web/boss
bosstool --output=web/boss/JPN_bossdata0.bin --programID=0004003000008202 $params $1
bosstool --output=web/boss/USA_bossdata0.bin --programID=0004003000008f02 $params $1
bosstool --output=web/boss/EUR_bossdata0.bin --programID=0004003000009802 $params $1
bosstool --output=web/boss/CHN_bossdata0.bin --programID=000400300000a102 $params $1
bosstool --output=web/boss/KOR_bossdata0.bin --programID=000400300000a902 $params $1
bosstool --output=web/boss/TWN_bossdata0.bin --programID=000400300000b102 $params $1
