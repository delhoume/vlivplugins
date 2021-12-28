pushd avi
nmake -f avi.mak 
copy /y avi.dll ..
popd
pushd debug
nmake -f debug.mak 
copy /y debug.dll ..
popd
pushd internal
nmake -f internal.mak
copy /y vliv.dll ..
popd
pushd lyapunov
nmake -f lyapunov.mak
copy /y lyapunov.dll ..
popd
pushd newton
nmake -f newton.mak
copy /y newton.dll ..
popd
pushd wic
nmake -f wichandler.mak
copy /y wichandler.dll ..
popd
