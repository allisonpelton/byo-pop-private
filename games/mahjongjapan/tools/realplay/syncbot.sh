#!/bin/sh
# China's Bot and TestCommands are Japan's with the namespace changed: run after editing Japan's
cd "$(dirname "$0")/../../.." || exit 1
for f in Bot.cpp Bot.h TestCommands.cpp; do
  sed 's/mahjongjapan/mahjongchina/g; s/MAHJONGJAPAN/MAHJONGCHINA/g' mahjongjapan/src/$f > mahjongchina/src/$f
done
