echo "---------------"
echo "scankey"
(cd scankey && bash run.sh) || exit 1
echo
echo "---------------"
echo "scanvalue"
(cd scanvalue && bash run.sh) || exit 1
echo
echo "---------------"
echo "parser"
(cd parser && bash run.sh) || exit 1
echo
echo "---------------"
echo "stdtest"
(cd stdtest && bash run.sh) || exit 1
